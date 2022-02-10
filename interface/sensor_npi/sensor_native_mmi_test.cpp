
#include "sensor_native_mmi_test.h"
#include "eng_attok.h"
#include "sensor.h"
#include "sprd_fts_type.h"
#include "sprd_fts_diag.h"
#include "sprd_fts_log.h"
#include "sprd_fts_cb.h"


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hardware/sensors.h>


// when get AT CMD to start test sensor, set thread_run to 1,
// when get AT CMD to close sensor, set thread_run to 0
static int thread_run;

// when get AT CMD to start calibration sensor, set thread_run to 1
static int cali_thread_run;

extern "C" {
typedef int (*NOTIFYSENSORDATA)(sensors_event_t *pEvent);

QUERYINTERFACE Sensor_ptfQueryInterface = NULL;
NOTIFYSENSORDATA ptrNotify = NULL;

void register_fw_function(struct fw_callback *reg)
{
    ALOGD("so_sensor: register_fw_function");

    Sensor_ptfQueryInterface = reg->ptfQueryInterface;

    if (Sensor_ptfQueryInterface("get sensor data", (const void**)&ptrNotify) != 0)
    {
        ALOGE("so_sensor:query interface(send at command) fail!");
    }
    ALOGD("so_sensor: register_fw_function: ptrNotify = %x", ptrNotify);
}
}

void *mmi_get_sensor_data_thread(void *sensor_type)
{
	const size_t EVENTS_NUM = 16;
	sensors_event_t sensor_event_data, sensor_events[EVENTS_NUM];
	int n = 0;
	int *type = (int*)sensor_type;
	int ret;

	LOGD("%s: begin!\n", __func__);

	if (sensor_load()) {
		LOGE("sensor load failed!\n");
		return NULL;
	}

	ret = sensor_enable(*type);
	if (ret == -1) {
		LOGE("sensor enable failed!\n");
		return NULL;
	}

	memset(sensor_events, 0 , sizeof(sensor_events));

	while(thread_run) {
		n = sensor_poll(sensor_events, EVENTS_NUM);

		if (n < 0) {
			LOGE("%s: error! sensor_poll get data failed!\n", __func__);
			break;
		}

    LOGD("so_sensor: notify: n = %d", n);
		for(int i = 0; i < n; i++) {
			if (sensor_events[i].type == *type) {
				sensor_event_data = sensor_events[i];
				//LOGD("%s: sensor_event got!\n", __func__);
				LOGD("%s: sensor_event got!value=<%5.1f,%5.1f,%5.1f>\n", __func__, sensor_events[i].data[0], sensor_events[i].data[1], sensor_events[i].data[2]);
				if (ptrNotify != NULL) {
					LOGD("so_sensor: notify");
					ptrNotify(&sensor_event_data);
				}
			}
		}

		usleep(5*1000);
    }

	sensor_disable(*type);
	LOGD("%s: end!\n", __func__);

	return NULL;
}


int sensor_test(int cmd, int type)
{
	LOGD("%s: cmd = %d, type = %d\n", __func__, cmd, type);

	if (cmd == 1) {
		pthread_t thread;
		thread_run = 1;
		pthread_create(&thread, NULL, mmi_get_sensor_data_thread, &type);
		LOGD("%s: mmi_get_sensor_data_thread running!\n", __func__);
		pthread_join(thread, NULL);
	}else if (cmd == 0){
		thread_run = 0;
		LOGD("%s: mmi_get_sensor_data_thread ended!\n", __func__);
	}else{
		LOGE("%s: unknown cmd, cmd = %d, type = %d\n", __func__, cmd, type);
	}
	return 0;
}

int eng_linuxcmd_sensortest(char *req, char *rsp)
{
	int cmd_type = 0, sensor_type = 0;
	char *ptr = NULL, *ptr_bak = NULL;
	int ret = -1;

	ALOGD("so_sensor: eng_linuxcmd_sensortest");

	if (NULL == req)
	{
		ALOGD("so_sensor:%s,null pointer", __FUNCTION__);
		goto ERR_CMD;
	}

	if (req[0] == 0x7e)
	{
		ptr = req + 1 + sizeof(MSG_HEAD_T);
	}
	else
	{
		ptr = strdup(req) ;
		if (!ptr) {
			ALOGD("ptr allocate mem error!");
			goto ERR_CMD;
		}
		ptr_bak = ptr;
	}

	ALOGD("so_sensor:sensor test:%s", ptr);
	ret = at_tok_equel_start(&ptr);
	ret |= at_tok_nextint(&ptr, &cmd_type);
	if (ret!=0) {
		ALOGD("so_sensor:sensor test line :%d error cmd_type:%s",__LINE__,ptr);
		goto FREE_PTR;
	}
	ret |= at_tok_nextint(&ptr, &sensor_type);
	if (ret!=0) {
		ALOGD("so_sensor:sensor test line :%d error sensor_type:%s",__LINE__,ptr);
		goto FREE_PTR;
	}

	return sensor_test(cmd_type, sensor_type);

FREE_PTR:
	if (ptr_bak) {
		free(ptr_bak);
		ptr_bak = NULL;
	}
ERR_CMD:
	if(rsp != NULL) {
		sprintf(rsp, "\r\nERROR\r\n");
		return strlen(rsp);
	} else {
		return 0;
	}
}


extern "C" {
/*
void register_this_module(struct eng_callback * reg)
{

    AT+SENSORTEST=p1,p2

    p1: 1---open sensor
        0---close sensor

    p2: sensor type, defined in sensors-base.h
        enum {
            SENSOR_TYPE_META_DATA = 0,
            SENSOR_TYPE_ACCELEROMETER = 1,
            SENSOR_TYPE_MAGNETIC_FIELD = 2,
            SENSOR_TYPE_ORIENTATION = 3,
            SENSOR_TYPE_GYROSCOPE = 4,
            SENSOR_TYPE_LIGHT = 5,
            SENSOR_TYPE_PRESSURE = 6,
            SENSOR_TYPE_TEMPERATURE = 7,
            SENSOR_TYPE_PROXIMITY = 8,
            SENSOR_TYPE_GRAVITY = 9
            ...
        }
/

    ALOGD("so_sensor:file:%s, func:%s\n", __FILE__, __func__);
    sprintf(reg->at_cmd, "%s", "AT+SENSORTEST");
    reg->eng_linuxcmd_func = eng_linuxcmd_sensortest;
}
*/
}




#ifdef SENSORHUB

int SenAlsCaliCmd(const char * cmd)
{
    int fd = -1;
    int ret = -1;
    int bytes_to_write = 0;

    if (cmd == NULL) {
        return -1;
    }

    bytes_to_write = strlen(cmd);
    if (fd < 0) {
        fd = open(LIGHT_SENSOR_CALIBRATOR, O_WRONLY | O_NONBLOCK);
    }

    if (fd < 0) {
        return -1;
    } else {
        ret = write(fd, cmd, bytes_to_write);
        if( ret < 0) {
            LOGE("als cali_write_cmd ret is %d, byte is %d", ret, bytes_to_write);
            return -1;
        }
    }

    if(fd >= 0) {
        close(fd);
    }

    return 0;
}

int light_sensor_calibration(int sensor_type)
{
	int ret;
	char write_buf[256] = {0};

	ret = sensor_enable(sensor_type);
	if (ret == -1) {
		LOGE("light sensor enable failed!\n");
		return -1;
	}
	usleep(500*1000);

	snprintf(write_buf, sizeof(write_buf) - 1,"%d %d", CALIB_DATA_WRITE, sensor_type);
	ret = SenAlsCaliCmd((const char *)write_buf);
	LOGD("als sensor cali cmd write:%s, ret = %d", write_buf, ret);
	sensor_disable(sensor_type);

	if (ret < 0) {
		LOGE("light sensor calibration failed!\n");
		return -1;
	}
	LOGD("light sensor calibration success!\n");

	return 0;
}

int SenCaliCmd(const char * cmd)
{
    int fd = -1;
    int ret = -1;
    int bytes_to_write = 0;

    if(cmd == NULL){
        return -1;
    }

    bytes_to_write = strlen(cmd);
    if(fd < 0) {
        fd = open(SENSOR_CALI_CMD, O_WRONLY | O_NONBLOCK);
    }

    if(fd < 0) {
        return -1;
    }else{
        ret = write(fd, cmd, bytes_to_write);
        if( ret != bytes_to_write) {
                        LOGD("cali_write_cmd ret is %d, byte is %d", ret, bytes_to_write);
                        return -1;
        }
    }

    if(fd >= 0) {
        close(fd);
    }

    return 0;
}

void *mmi_cali_get_sensor_data_thread(void *sensor_type)
{
	const size_t EVENTS_NUM = 16;
	sensors_event_t sensor_event_data, sensor_events[EVENTS_NUM];
	int n = 0;
	int *type = (int*)sensor_type;

	LOGD("%s: begin!\n", __func__);

	memset(sensor_events, 0 , sizeof(sensor_events));

	while(cali_thread_run) {
		n = sensor_poll(sensor_events, EVENTS_NUM);

		if (n < 0) {
			LOGE("%s: error! sensor_poll get data failed!\n", __func__);
			break;
		}

    LOGD("so_sensor: notify: n = %d", n);
		for(int i = 0; i < n; i++) {
			if (sensor_events[i].type == *type) {
				sensor_event_data = sensor_events[i];
				//LOGD("%s: sensor_event got!\n", __func__);
				LOGD("%s: sensor_event got!value=<%5.1f,%5.1f,%5.1f>\n", __func__, sensor_events[i].data[0], sensor_events[i].data[1], sensor_events[i].data[2]);
				if (ptrNotify != NULL) {
					LOGD("so_sensor: notify");
					ptrNotify(&sensor_event_data);
				}
			}
		}

		usleep(50*1000);
    }

	return NULL;
}


static int sensor_calibration(int enable, int cali_type, int sensor_type)
{
	int ret;
	char write_buf[256] = {0};
	char calibuf[128] = {0};
	int cali_data_fd;

	LOGD("%s: enable = %d, cali_type = %d, sensor_type = %d\n", __func__, enable, cali_type, sensor_type);

	if (sensor_load()) {
		LOGE("sensor load failed!\n");
		return -1;
	}

	if (sensor_type == SENSOR_TYPE_LIGHT) {

		ret = light_sensor_calibration(sensor_type);
		if (ret < 0)
		    return -1;
		else
		    return 0;
	}

	snprintf(write_buf, sizeof(write_buf) - 1,"%d %d 1", CALIB_EN, sensor_type);
	ret = SenCaliCmd((const char *)write_buf);
	LOGD("sensor cali cmd write:%s, ret = %d", write_buf, ret);

	ret = sensor_enable(sensor_type);
	if (ret == -1) {
		LOGE("sensor enable failed!\n");
		return -1;
	}

	pthread_t thread;
	cali_thread_run = 1;
	pthread_create(&thread, NULL, mmi_cali_get_sensor_data_thread, &sensor_type);
	LOGD("%s: mmi_cali_get_sensor_data_thread running!\n", __func__);

	sleep(4);

	snprintf(write_buf, sizeof(write_buf) - 1,"%d %d 1", CALIB_CHECK_STATUS, sensor_type);
	ret = SenCaliCmd((const char *)write_buf);
	LOGD("sensor cali cmd write:%s, ret = %d", write_buf, ret);

	cali_data_fd = open(SENSOR_CALI_DATA, O_RDWR);
	if(cali_data_fd < 0){
		LOGE("open sensor cali data: %s faild", SENSOR_CALI_DATA);
		ret = -1;
		goto END;
	} else {
		memset(calibuf, 0, sizeof(calibuf));
		ret = read(cali_data_fd, calibuf, sizeof(calibuf));
		if (ret <= 0) {
			LOGE("mmitest [fp:%d] read calibrator_data length error", cali_data_fd);
			ret = -1;
			goto END;
		}
		LOGD("calibrator_data = %s; %d", calibuf, atoi(calibuf));
	}

	if (2 == atoi(calibuf)) {
		snprintf(write_buf, sizeof(write_buf) - 1,"%d %d 1", CALIB_DATA_READ, sensor_type);
		ret = SenCaliCmd((const char *)write_buf);
		LOGD("sensor cali cmd write:%s, ret = %d", write_buf, ret);
		cali_data_fd = open(SENSOR_CALI_DATA,O_RDWR);
		if(cali_data_fd < 0){
			LOGE("open sensor cali data: %s faild",SENSOR_CALI_DATA);
			ret = -1;
			goto END;
		} else {
			memset(calibuf, 0, sizeof(calibuf));
			ret = read(cali_data_fd, calibuf, sizeof(calibuf));
			if (ret <= 0) {
				LOGE("mmitest [fp:%d] read calibrator_data length error", cali_data_fd);
				ret = -1;
				goto END;
			} else {
				if (!(atoi(calibuf)))
					ret = 0;
				else
					ret = -1;
			}
			close(cali_data_fd);
			LOGD("calibrator_data = %s; %d", calibuf, atoi(calibuf));
		}
	} else {
		LOGD("calibrator_data = %s; %d, calibration failed", calibuf, atoi(calibuf));
		ret = -1;
		goto END;
	}

END:
	cali_thread_run = 0;
	sensor_disable(sensor_type);

	if(is_onchange_reporting_mode(sensor_type) == 1)	{
	/*
	 * on change reprting mode sensor may not report any events in
	 * mmi_cali_get_sensor_data_thread loop before we disable it.
	 * which may cause the thread can't end. so we enable it once
	 * again to end the loop. we don't use pthread_kill to end the thread,
	 * because it may cause poll re-entry next time we enter sensor cali.
	 */
		usleep(50*1000);
		sensor_enable(sensor_type);
		usleep(50*1000);
		sensor_disable(sensor_type);
	}
	usleep(100*1000);
	pthread_join(thread, NULL);

	LOGD("%s: end!\n", __func__);

	if (ret < 0)
		return -1;
	else
		return 0;

}

int eng_linuxcmd_sensorcali(char *req, char *rsp)
{
    int enable = 0, calibration_type = 0, sensor_type = 0;
    char *ptr = NULL, *ptr_bak = NULL;
    int ret = -1;

    ALOGD("so_sensor: eng_linuxcmd_sensorcali");

	if (NULL == req)
	{
		ALOGD("so_sensor:%s,null pointer", __FUNCTION__);
		goto ERR_CMD;
	}

	if (req[0] == 0x7e)
	{
		ptr = req + 1 + sizeof(MSG_HEAD_T);
	}
	else
	{
		ptr = strdup(req) ;
		if (!ptr) {
			ALOGD("ptr allocate mem error!");
			goto ERR_CMD;
		}
		ptr_bak = ptr;
	}

	ALOGD("so_sensor:sensor test:%s", ptr);

	ret = at_tok_equel_start(&ptr);

	ret |=at_tok_nextint(&ptr, &enable);
	if(ret!=0) {
		ALOGD("so_sensor:sensor test line :%d error enable:%s",__LINE__,ptr);
		goto FREE_PTR;
	}
	ret |= at_tok_nextint(&ptr, &calibration_type);
	if(ret!=0) {
		ALOGD("so_sensor:sensor test line :%d error calibration_type:%s",__LINE__,ptr);
		goto FREE_PTR;
	}
	ret |= at_tok_nextint(&ptr, &sensor_type);
	if(ret!=0) {
		ALOGD("so_sensor:sensor test line :%d error sensor_type:%s",__LINE__,ptr);
		goto FREE_PTR;
	}

	return sensor_calibration(enable, calibration_type, sensor_type);

FREE_PTR:
	if (ptr_bak) {
		free(ptr_bak);
		ptr_bak = NULL;
	}
ERR_CMD:
	if (rsp != NULL) {
		sprintf(rsp, "\r\nERROR\r\n");
		return strlen(rsp);
	} else {
		return 0;
	}
}


#endif //SENSORHUB

