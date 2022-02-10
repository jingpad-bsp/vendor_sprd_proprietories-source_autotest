/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "sensor.h"
#include "sensor_calibration.h"

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <hardware/sensors.h>

#include "sprd_fts_type.h"

#define ENG_STREND "\r\n"

#define PRODUCTINFO_FILE_NODE "/mnt/vendor/productinfo"
#define ACC_CALIBRATION_FILE_NODE "/mnt/vendor/productinfo/acc_sensor_calibration"
#define ACC_CALIBRATION_COUNT_FILE "/mnt/vendor/productinfo/acc_sensor_calibration/calibration_status"
#define ACC_SENSOR_X_AXIS_OFFSET "/mnt/vendor/productinfo/acc_sensor_calibration/x_axis_offset"
#define ACC_SENSOR_Y_AXIS_OFFSET "/mnt/vendor/productinfo/acc_sensor_calibration/y_axis_offset"
#define ACC_SENSOR_Z_AXIS_OFFSET "/mnt/vendor/productinfo/acc_sensor_calibration/z_axis_offset"

//static int eng_linuxcmd_accsensor_calibration(char *req, char *rsp);

static unsigned int calibration_status;
static float x_offset, y_offset, z_offset;
static const float MAX_X_Y_OFFSET_VALUE = 2;
static const float MAX_Z_OFFSET_VALUE = 2.5;

static int check_calibration_status()
{
	int err = -1;
	FILE *fd = NULL;

	if (access(ACC_CALIBRATION_COUNT_FILE, 0)) {
		LOGD("acc_sensor_calibration directory not exist, creat it\n");
		if (mkdir(PRODUCTINFO_FILE_NODE, 0770))
			LOGE("creat PRODUCTINFO_FILE_NODE failed!(%s)\n", strerror(errno));
		if (mkdir(ACC_CALIBRATION_FILE_NODE, 0770))
			LOGE("creat ACC_CALIBRATION_FILE_NODE failed!(%s)\n", strerror(errno));
		fd = fopen(ACC_CALIBRATION_COUNT_FILE, "w+");
		if (fd == NULL) {
			LOGE("creat calibration_status file failed!(%s)\n", strerror(errno));
			return -1;
		}
		fclose(fd);
		calibration_status = 0;
	} else {
		fd = fopen(ACC_CALIBRATION_COUNT_FILE, "r");
		if (fd == NULL) {
			LOGE("open calibration_status file failed!(%s)\n", strerror(errno));
			return -1;
		}
		err = fscanf(fd, "%d", &calibration_status);
		if (err < 0) {
			LOGE("read calibration_status failed!(%s)\n", strerror(errno));
			return -1;
		}
		LOGD("check_calibration_status: calibration_status=%d\n", calibration_status);
		fclose(fd);
	}

	return 0;
}

static int remove_calibration_files()
{
	int err = -1;
	FILE *calibration_status_fd = NULL;

	err = remove(ACC_SENSOR_X_AXIS_OFFSET);
	err = remove(ACC_SENSOR_Y_AXIS_OFFSET);
	err = remove(ACC_SENSOR_Z_AXIS_OFFSET);

	if (!err)	{
		calibration_status = 0;
		calibration_status_fd = fopen(ACC_CALIBRATION_COUNT_FILE, "w");
		if (calibration_status_fd == NULL) {
		LOGE("open calibration_status file failed!(%s)\n", strerror(errno));
		return -1;
		}
		fprintf(calibration_status_fd, "%d", calibration_status);
		fclose(calibration_status_fd);
	} else {
		LOGE("remove calibration files failed!(%s)\n", strerror(errno));
	}

	return err;
}

static void thread_exit_handler(int sig)
{
	LOGD("receive signal %d , eng_receive_data_thread exit\n", sig);
	pthread_exit(0);
}

static void *accsensorcali_thread(void *data)
{
	int *realdata = (int *)data;
	const size_t EVENTS_NUM = 16;
	int events_received_count = 0, acc_events_count = 0, reject_number = 6;
	sensors_event_t sensorbuffer[EVENTS_NUM], acc_events[EVENTS_NUM];
	float x_sum = 0, y_sum = 0, z_sum = 0;


	struct sigaction actions;
	memset(&actions, 0, sizeof(actions));
	sigemptyset(&actions.sa_mask);
	actions.sa_flags = 0;
	actions.sa_handler = thread_exit_handler;
	sigaction(SIGUSR1, &actions, NULL);

	*realdata = sensor_load();
	if (*realdata == -1) {
		LOGE("sensor load failed!\n");
		return realdata;
	}
	LOGD("sensor_load successed!\n");

	*realdata = sensor_enable(SENSOR_TYPE_ACCELEROMETER);
	if (*realdata == -1) {
		LOGE("sensor enable failed!\n");
		return realdata;
	}
	LOGD("sensor enabled, accsensor ID is %d\n", *realdata);

	events_received_count = sensor_poll(sensorbuffer, EVENTS_NUM);
	memset(sensorbuffer, 0 , sizeof(sensorbuffer));
	memset(acc_events, 0 , sizeof(acc_events));

	for (; acc_events_count <EVENTS_NUM;) {
		events_received_count = sensor_poll(sensorbuffer, EVENTS_NUM);
		if (events_received_count < 0) {
			LOGE("sensor_poll get data failed!\n");
			return realdata;
		}
		for (int i = 0; i < events_received_count; i++) {
			if(sensorbuffer[i].type == SENSOR_TYPE_ACCELEROMETER) {
				memcpy(acc_events + acc_events_count, sensorbuffer + i, sizeof(sensorbuffer[i]));
				acc_events_count ++;
				if (acc_events_count == EVENTS_NUM)
					break;
			}
		}
	}

	*realdata = 0;

	for (int j = 0; j < acc_events_count; j++) {
		if (j < reject_number)
			continue;
		LOGD("acc sensor value: X=%f, Y=%f, Z=%f\n", acc_events[j].acceleration.x, acc_events[j].acceleration.y, acc_events[j].acceleration.z);
		x_sum += acc_events[j].acceleration.x;
		y_sum += acc_events[j].acceleration.y;
		z_sum += acc_events[j].acceleration.z;
	}
	x_offset = 0 - x_sum / (acc_events_count - reject_number);
	y_offset = 0 - y_sum / (acc_events_count - reject_number);
	z_offset = GRAVITY_EARTH - z_sum / (acc_events_count - reject_number);
	LOGD("caculating offset value done,  x_offset=%f, y_offset=%f, z_offset=%f\n", x_offset, y_offset, z_offset);
	sensor_disable(SENSOR_TYPE_ACCELEROMETER);

	return realdata;
}

static int check_offset_value()
{
	if((fabs(x_offset) >= MAX_X_Y_OFFSET_VALUE) || (fabs(y_offset) >= MAX_X_Y_OFFSET_VALUE) || (fabs(z_offset) >= MAX_Z_OFFSET_VALUE))
		return -1;

	return 0;
}

static int store_offset_value()
{
	FILE *x_offset_fd = NULL;
	FILE *y_offset_fd = NULL;
	FILE *z_offset_fd = NULL;
	FILE *calibration_status_fd = NULL;
	float pre_x_offset = 0, pre_y_offset = 0, pre_z_offset = 0;


	x_offset_fd = fopen(ACC_SENSOR_X_AXIS_OFFSET, "w");
	if (x_offset_fd == NULL) {
		LOGE("creat x_axis_offset file failed!(%s)\n", strerror(errno));
		return -1;
	}
	fprintf(x_offset_fd, "%f", x_offset);
	fflush(x_offset_fd);
	fsync(fileno(x_offset_fd));
	fclose(x_offset_fd);

	y_offset_fd = fopen(ACC_SENSOR_Y_AXIS_OFFSET, "w");
	if (y_offset_fd == NULL) {
		LOGE("creat y_axis_offset file failed!(%s)\n", strerror(errno));
		return -1;
	}
	fprintf(y_offset_fd, "%f", y_offset);
	fflush(y_offset_fd);
	fsync(fileno(y_offset_fd));
	fclose(y_offset_fd);

	z_offset_fd = fopen(ACC_SENSOR_Z_AXIS_OFFSET, "w");
	if (z_offset_fd == NULL) {
		LOGE("creat z_axis_offset file failed!(%s)\n", strerror(errno));
		return -1;
	}
	fprintf(z_offset_fd, "%f", z_offset);
	fflush(z_offset_fd);
	fsync(fileno(z_offset_fd));
	fclose(z_offset_fd);

	calibration_status_fd = fopen(ACC_CALIBRATION_COUNT_FILE, "w");
	if (calibration_status_fd == NULL) {
		LOGE("open calibration_status file failed!(%s)\n", strerror(errno));
		return -1;
	}
	fprintf(calibration_status_fd, "%d", 1);
	fflush(calibration_status_fd);
	fsync(fileno(calibration_status_fd));
	fclose(calibration_status_fd);

	return 0;
}

int eng_linuxcmd_accsensor_calibration(char *req, char *rsp)
{
	int ret;
	pthread_t thread = -1;
	int thread_status = -1;

	ret = check_calibration_status();
	if (ret == -1) {
		LOGE("acc sensor check_calibration_status failed!\n");
		return -1;
	}

	if (calibration_status) {
		if(remove_calibration_files()) {
			LOGE("acc sensor remove_calibration_files failed!\n");
			return -1;
		}
	}
	pthread_create(&thread, NULL, accsensorcali_thread, &thread_status);
	sleep(4); //use 4 second to calibration acc sensor.
	if (thread_status == 0) {
		pthread_join(thread, NULL);
	} else {
		LOGE("accsensorcali_thread haven't completed after 5 seconds, there may be something unexpected happened in get acc sensor data\n");
		if (pthread_kill(thread, SIGUSR1) != 0) {
			LOGE("pthread_kill get_sensor_data thread error\n");
			return -1;
		}
		pthread_join(thread, NULL);
	}

	ret = check_offset_value();
	if (ret == -1) {
		LOGE("acc sensor calibration offset value exceeded limit!\n");
		return -1;
	}

	ret = store_offset_value();
	if (ret == -1) {
		LOGE("acc sensor store calibration value failed!\n");
		return -1;
	}

	LOGD("acc sensor calibration successed!\n");
	return 0;
}
/*
extern "C" {
void register_this_module_ext(struct eng_callback *reg, int *num)
{
	unsigned int moudles_num = 0;

	(reg + moudles_num)->type = 0x38;
    LOGD("file:%s, func:%s\n", __FILE__, __func__);
    sprintf(reg->at_cmd, "%s", "AT+SENSORCALI");
    reg->eng_linuxcmd_func = eng_linuxcmd_accsensor_calibration;
	LOGD("module cmd:%s\n", reg->at_cmd);
	moudles_num ++;

	*num = moudles_num;
}
}
*/