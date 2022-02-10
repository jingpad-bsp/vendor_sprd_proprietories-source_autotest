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

#include "sensor_bbat_test.h"

#include "sprd_fts_type.h"
#include "sprd_fts_diag.h"
#include "sprd_fts_log.h"

#include <pthread.h>
#include <unistd.h>

static void thread_exit_handler(int sig)
{
	LOGD("receive signal %d , eng_receive_data_thread exit\n", sig);
	pthread_exit(0);
}

static void *get_sensor_data_thread(void *sensor_event_num)
{
	struct sensor_event_and_num *sensor_event_with_num = (sensor_event_and_num *)sensor_event_num;
	const size_t EVENTS_NUM = 16;
	sensors_event_t sensorbuffer[EVENTS_NUM];
	int events_received_num = 0, valid_events_num = 0, i =0;

	struct sigaction actions;
	memset(&actions, 0, sizeof(actions));
	sigemptyset(&actions.sa_mask);
	actions.sa_flags = 0;
	actions.sa_handler = thread_exit_handler;
	sigaction(SIGUSR1, &actions, NULL);

	memset(sensorbuffer, 0 , sizeof(sensorbuffer));

	for(; valid_events_num <sensor_event_with_num->least_events_num_needed;) {
		events_received_num = sensor_poll(sensorbuffer, EVENTS_NUM);
		for (i = 0; i < events_received_num; i++)
			if(sensor_event_with_num->sensor_event.type == sensorbuffer[i].type) {
				sensor_event_with_num->sensor_event = sensorbuffer[i];
				valid_events_num++;
			}
	}

	sensor_event_with_num->events_num_received = valid_events_num;

	return NULL;
}

int bbat_sensor_test_start(struct sensor_event_and_num *event_and_num)
{
	int ret = -1, i = 0;
	pthread_t thread;

	if (sensor_load()) {
		LOGE("sensor load failed!\n");
		return BBAT_TEST_FAILED;
	}

	ret = sensor_enable(event_and_num->sensor_event.type);
	if (ret == -1) {
		LOGE("sensor enable failed!\n");
		return BBAT_TEST_FAILED;
	}
	if (event_and_num->sensor_event.type == SENSOR_TYPE_LIGHT ||
	    event_and_num->sensor_event.type == SENSOR_TYPE_PROXIMITY) {
		// 0.5 seconds is enough for light/proximity sensor have valid data
		usleep(500 * 1000);
	}

	event_and_num->events_num_received= 0;
	pthread_create(&thread, NULL, get_sensor_data_thread, event_and_num);
	for(i = 0; i < 40 ; i++) { // 2 seconds is enough for sensor test get data, if no data in 2s, it's abnormal
		usleep(50 * 1000);
		if(event_and_num->events_num_received)
			break;
	}

	if (event_and_num->events_num_received) {
		pthread_join(thread, NULL);
	} else {
		LOGE("get_sensor_data_thread haven't completed after 2 seconds, there may be something unexpected happened in get sensor data\n");
		if (pthread_kill(thread, SIGUSR1) != 0) {
			LOGE("pthread_kill get_sensor_data thread error\n");
		}
		pthread_join(thread, NULL);
	}

	sensor_disable(event_and_num->sensor_event.type);

	if(event_and_num->events_num_received < event_and_num->least_events_num_needed)
		return BBAT_TEST_FAILED;

	return BBAT_TEST_SUCCESS;
}

int bbat_accelerometer_single_test(struct sensor_event_and_num *event_and_num,
										char *data, unsigned int *data_length)
{
	int ret = BBAT_TEST_FAILED, symbol_x, symbol_y, symbol_z;
	unsigned int x_value = 0, y_value = 0, z_value = 0;

	ret = bbat_sensor_test_start(event_and_num);

	if (ret == BBAT_TEST_SUCCESS) {
		LOGD("%s: x=%f, y=%f, z=%f", __func__,
							event_and_num->sensor_event.acceleration.x,
							event_and_num->sensor_event.acceleration.y,
							event_and_num->sensor_event.acceleration.z);
		symbol_x = event_and_num->sensor_event.acceleration.x >= 0 ? 0 : 1;
		x_value = fabs(event_and_num->sensor_event.acceleration.x) * 1000000;
		symbol_y = event_and_num->sensor_event.acceleration.y >= 0 ? 0 : 1;
		y_value = fabs(event_and_num->sensor_event.acceleration.y) * 1000000;
		symbol_z = event_and_num->sensor_event.acceleration.z >= 0 ? 0 : 1;
		z_value = fabs(event_and_num->sensor_event.acceleration.z) * 1000000;
		data[0] = symbol_x;
		data[1] = (unsigned char)(x_value>>16);
		data[2] = (unsigned char)(x_value>>8);
		data[3] = (unsigned char)(x_value);
		data[4] = symbol_y;
		data[5] = (unsigned char)(y_value>>16);
		data[6] = (unsigned char)(y_value>>8);
		data[7] = (unsigned char)(y_value);
		data[8] = symbol_z;
		data[9] = (unsigned char)(z_value>>16);
		data[10] = (unsigned char)(z_value>>8);
		data[11] = (unsigned char)(z_value);
		*data_length = 12;
	} else {
		LOGE("accelerometer sensor single test failed!\n");
		*data_length = 0;
	}

	return ret;
}

int bbat_light_sensor_single_test(struct sensor_event_and_num *event_and_num,
										char *data, unsigned int *data_length)
{
	int ret = BBAT_TEST_FAILED;
	unsigned int light_value;

	ret = bbat_sensor_test_start(event_and_num);

	if (ret == BBAT_TEST_SUCCESS) {
		light_value= event_and_num->sensor_event.light;
		data[0] = (unsigned char)(light_value>>8);
		data[1] = (unsigned char)(light_value);
		*data_length = 2;
	} else {
		LOGE("light sensor single test failed!\n");
		*data_length = 0;
	}

	return ret;
}

int bbat_proximity_sensor_single_test(struct sensor_event_and_num *event_and_num,
										char *data, unsigned int *data_length)
{
	int ret = BBAT_TEST_FAILED;
	unsigned int proximity_value;

	ret = bbat_sensor_test_start(event_and_num);

	if (ret == BBAT_TEST_SUCCESS) {
		proximity_value= event_and_num->sensor_event.distance;
		data[0] = (unsigned char)proximity_value;
		*data_length = 1;
	} else {
		LOGE("proximity sensor single test failed!\n");
		*data_length = 0;
	}

	return ret;
}

int bbat_gyroscope_single_test(struct sensor_event_and_num *event_and_num,
										char *data, unsigned int *data_length)
{
	int ret = BBAT_TEST_FAILED, symbol_x, symbol_y, symbol_z;
	unsigned int x_value = 0, y_value = 0, z_value = 0;

	ret = bbat_sensor_test_start(event_and_num);

	if (ret == BBAT_TEST_SUCCESS) {
		symbol_x = event_and_num->sensor_event.gyro.x >= 0 ? 0 : 1;
		x_value = fabs(event_and_num->sensor_event.gyro.x) * 1000;
		symbol_y = event_and_num->sensor_event.gyro.y >= 0 ? 0 : 1;
		y_value = fabs(event_and_num->sensor_event.gyro.y) * 1000;
		symbol_z = event_and_num->sensor_event.gyro.z >= 0 ? 0 : 1;
		z_value = fabs(event_and_num->sensor_event.gyro.z) * 1000;
		data[0] = symbol_x;
		data[1] = (unsigned char)(x_value>>8);
		data[2] = (unsigned char)(x_value);
		data[3] = symbol_y;
		data[4] = (unsigned char)(y_value>>8);
		data[5] = (unsigned char)(y_value);
		data[6] = symbol_z;
		data[7] = (unsigned char)(z_value>>8);
		data[8] = (unsigned char)(z_value);
		*data_length = 9;
	} else {
		LOGE("gyroscope sensor single test failed!\n");
		*data_length = 0;
	}

	return ret;
}

int bbat_magnetic_single_test(struct sensor_event_and_num *event_and_num,
										char *data, unsigned int *data_length)
{
	int ret = BBAT_TEST_FAILED, symbol_x, symbol_y, symbol_z;
	unsigned int x_value = 0, y_value = 0, z_value = 0;

	ret = bbat_sensor_test_start(event_and_num);

	if (ret == BBAT_TEST_SUCCESS) {
		symbol_x = event_and_num->sensor_event.magnetic.x >= 0 ? 0 : 1;
		x_value = fabs(event_and_num->sensor_event.magnetic.x) * 1000;
		symbol_y = event_and_num->sensor_event.magnetic.y >= 0 ? 0 : 1;
		y_value = fabs(event_and_num->sensor_event.magnetic.y) * 1000;
		symbol_z = event_and_num->sensor_event.magnetic.z >= 0 ? 0 : 1;
		z_value = fabs(event_and_num->sensor_event.magnetic.z) * 1000;
		data[0] = symbol_x;
		data[1] = (unsigned char)(x_value>>8);
		data[2] = (unsigned char)(x_value);
		data[3] = symbol_y;
		data[4] = (unsigned char)(y_value>>8);
		data[5] = (unsigned char)(y_value);
		data[6] = symbol_z;
		data[7] = (unsigned char)(z_value>>8);
		data[8] = (unsigned char)(z_value);
		*data_length = 9;
	} else {
		LOGE("gyroscope sensor single test failed!\n");
		*data_length = 0;
	}

	return ret;
}

int bbat_pressure_single_test(struct sensor_event_and_num *event_and_num,
										char *data, unsigned int *data_length)
{
	int ret = BBAT_TEST_FAILED;
	unsigned int pressure_value = 0;

	ret = bbat_sensor_test_start(event_and_num);

	if (ret == BBAT_TEST_SUCCESS) {
		pressure_value = event_and_num->sensor_event.pressure  * 1000;
		data[1] = (unsigned char)(pressure_value>>16);
		data[2] = (unsigned char)(pressure_value>>8);
		data[3] = (unsigned char)pressure_value;
		*data_length = 3;
	} else {
		LOGE("pressure sensor single test failed!\n");
		*data_length = 0;
	}

	return ret;
}

int package_test_result_to_rsp(char *buf, char *rsp, char ret, char *data_to_pc, int data_to_pc_len)
{
	struct msg_head_tag *msg_head_ptr;
	// string rsp is composed by 5 parts: 0x7e, struct msg_head_tag without subtype member, ret, data_to_pc, 0x7e.
	int rsplen = 1 + (sizeof(msg_head_tag) - 1) + 1 + data_to_pc_len + 1;

	memcpy(rsp, buf, 1+ sizeof(msg_head_tag) - 1); // -1 means not contain subtype(char) member.
	rsp[1 + (sizeof(msg_head_tag) - 1)] = ret;
	memcpy(rsp+ 1 + (sizeof(msg_head_tag) - 1) + 1, data_to_pc, data_to_pc_len); // copy data_to_pc to rsp
	rsp[rsplen - 1] = 0x7e;
	msg_head_ptr = (struct msg_head_tag *)(rsp + 1);
	msg_head_ptr->len = rsplen - 2; // rsp length without 2 char: head(0x7e) and tail(0x7e)

	return rsplen;
}

/*
 * diag_sensor_test_interface() - function for bbat sensor one key test and single item test
 * @buf: pc cmd
 * @buf_len: length of buf
 * @rsp: diag cmd return to pc
 * @rsplen: length of rsp
 */
int diag_sensor_test_interface(char *buf, int buf_len, char *rsp, int rsplen)
{
	/* diag cmd use 3 character to identify different sensor,
	 * buf[9]: main cmd, whether is specific sensor available test or skd specifical sensor test,
	 * buf[10]: main sensor type,
	 * buf[11]: sub sensor type, 0x1: proximity sensor, 0x2 light sensor.
	 */
	char main_cmd = buf[9], sensor_type = buf[10], sub_sensor_type = buf[11];
	struct sensor_event_and_num event_and_num;
	//accelerometor need 12 char to store acceleration value, it is the max.
	char data_to_pc[12];
	unsigned int data_to_pc_len = 0;
	char ret = BBAT_TEST_FAILED;

	LOGD("%s: test start, main_cmd=0x%x", __func__, main_cmd);
	switch (main_cmd) {
	case SENSOR_TYPE_ACCELEROMETER:
	case SENSOR_TYPE_MAGNETIC_FIELD:
	case SENSOR_TYPE_ORIENTATION:
	case SENSOR_TYPE_GYROSCOPE:
	case SENSOR_TYPE_LIGHT:
	case SENSOR_TYPE_PRESSURE:
	case SENSOR_TYPE_TEMPERATURE:
	case SENSOR_TYPE_PROXIMITY:
	case SENSOR_TYPE_GRAVITY:
	case SENSOR_TYPE_LINEAR_ACCELERATION:
	case SENSOR_TYPE_ROTATION_VECTOR:
		event_and_num.sensor_event.type = main_cmd;
		event_and_num.least_events_num_needed = 1;
		ret = bbat_sensor_test_start(&event_and_num);
		if (ret == BBAT_TEST_FAILED)
			LOGE("main_cmd=0x%x sensor test failed!\n", main_cmd);
		data_to_pc[0] = ret; //bbat autotester tool use test result as return data
		data_to_pc_len = 1;
		ret = 0; //bbat autotester tool need test result always return 0;
		break;
	case SPECIFICAL_SENSOR_TEST:
		switch (sensor_type) {
		case ACCELEROMETER_SKD_SINGLE_TEST:
			event_and_num.sensor_event.type = SENSOR_TYPE_ACCELEROMETER;
			event_and_num.least_events_num_needed = 10;
			ret = bbat_accelerometer_single_test(&event_and_num, data_to_pc, &data_to_pc_len);
			break;
		case LIGHT_AND_PROXIMITY_SKD_SINGLE_TEST:
			if (sub_sensor_type == LIGHT) {
				//test light sensor
				event_and_num.sensor_event.type = SENSOR_TYPE_LIGHT;
				event_and_num.least_events_num_needed = 1;
				ret = bbat_light_sensor_single_test(&event_and_num, data_to_pc, &data_to_pc_len);
			} else {
				//test proximity sensor
				event_and_num.sensor_event.type = SENSOR_TYPE_PROXIMITY;
				event_and_num.least_events_num_needed = 1;
				ret = bbat_proximity_sensor_single_test(&event_and_num, data_to_pc, &data_to_pc_len);
			}
			break;
		case GYROSCOPE_SKD_SINGLE_TEST:
			event_and_num.sensor_event.type = SENSOR_TYPE_GYROSCOPE;
			event_and_num.least_events_num_needed = 10;
			ret = bbat_gyroscope_single_test(&event_and_num, data_to_pc, &data_to_pc_len);
			break;
		case MAGNETIC_FIELD_SKD_SINGLE_TEST:
			event_and_num.sensor_event.type = SENSOR_TYPE_MAGNETIC_FIELD;
			event_and_num.least_events_num_needed = 10;
			ret = bbat_magnetic_single_test(&event_and_num, data_to_pc, &data_to_pc_len);
			break;
		case PRESSURE_SKD_SINGLE_TEST:
			event_and_num.sensor_event.type = SENSOR_TYPE_PRESSURE;
			event_and_num.least_events_num_needed = 10;
			ret = bbat_pressure_single_test(&event_and_num, data_to_pc, &data_to_pc_len);
			break;
		default:
			LOGE("sensor_type=0x%x is an unsupported sensor or have no test code for it, please check sensors_bbat_test file!\n", sensor_type);
			ret = BBAT_TEST_FAILED;
			break;
		}
		break;
	default:
		LOGE("main_cmd=0x%x is unsupported!\n", main_cmd);
		return ENG_DIAG_RET_UNSUPPORT;
		break;
	}

	LOGD("pc->engpc:%d number--> %x %x %x %x %x %x %x %x %x %x %x ",buf_len,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10]);

	rsplen = package_test_result_to_rsp(buf, rsp, ret, data_to_pc, data_to_pc_len);
	if (main_cmd == 0x20)
		LOGD("engpc->pc:%d number--> %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
										rsplen,rsp[0],rsp[1],rsp[2],rsp[3],rsp[4],rsp[5],rsp[6],rsp[7],rsp[8],rsp[9],rsp[10],rsp[11],rsp[12],rsp[13],rsp[14],rsp[15],rsp[16],rsp[17],rsp[18],rsp[19],rsp[20],rsp[21]);
	else
		LOGD("engpc->pc:%d number--> %x %x %x %x %x %x %x %x %x %x %x ",rsplen,rsp[0],rsp[1],rsp[2],rsp[3],rsp[4],rsp[5],rsp[6],rsp[7],rsp[8],rsp[9],rsp[10]);
	return rsplen;
}
/*
extern "C" {
void register_this_module_ext(struct eng_callback *reg, int *num)
{
	unsigned int moudles_num = 0;

	(reg + moudles_num)->type = BBAT_TEST;
	(reg + moudles_num)->subtype = BBAT_SENSOR_TEST;
	(reg + moudles_num)->eng_diag_func = diag_sensor_test_interface;
	LOGD("register module for Diag cmd diag_sensor_test_interface : [%p] \n", diag_sensor_test_interface);
	moudles_num ++;

	*num = moudles_num;
	return;
}
}
*/
