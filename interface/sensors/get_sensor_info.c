/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
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

#include "sprd_fts_type.h"
#include "sprd_fts_diag.h"
#include "sprd_fts_log.h"

#define ENG_STREND "\r\n"

#if defined(ANDROID)
#include <cutils/log.h>

#undef LOG_TAG
#define LOG_TAG "sprd_sensors_hal"

#define MIN(a, b) ((a) <= (b) ? (a) : (b))

#define LOGI(fmt, args...) \
  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args)
#define LOGD(fmt, args...) \
  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) \
  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)
#endif


static int eng_linuxcmd_getsensorinfo(char *req, char *rsp);


static struct sensors_module_t      * sSensorModule = NULL;
static struct sensors_poll_device_t * sSensorDevice = NULL;
static struct sensor_t const        * sSensorList   = NULL;
static int                            sSensorCount  = 0;


static int get_all_sensors_name(char *sensor_name)
{
    unsigned int len = 0, count = 1;
    int err = -1;
    for(int i =0; i < sSensorCount; i++ ) {
        switch((sSensorList + i)->type) {
            case SENSOR_TYPE_ACCELEROMETER:
            case SENSOR_TYPE_MAGNETIC_FIELD:
            case SENSOR_TYPE_GYROSCOPE:
            case SENSOR_TYPE_LIGHT:
            case SENSOR_TYPE_PRESSURE:
            case SENSOR_TYPE_TEMPERATURE:
            case SENSOR_TYPE_PROXIMITY:
            case SENSOR_TYPE_RELATIVE_HUMIDITY:
            case SENSOR_TYPE_AMBIENT_TEMPERATURE:
            case SENSOR_TYPE_HEART_RATE:
                   err = sprintf(sensor_name + len, "%s\n", (sSensorList + i)->name);
                   ALOGD("%d sensor is %s, sensor_name=%s, strlen((sSensorList + i)->name=%d\n",
                                      count++, (sSensorList + i)->name, sensor_name, strlen((sSensorList + i)->name));
                   len += (strlen((sSensorList + i)->name) + 1);
                   break;
            default:
                   break;
        }

    }

    return err;
}

static int init_sensorOpen( void )
{
    int err = -1;
    if( NULL != sSensorModule ) {
        ALOGD("already init!\n");
        return 0;
    }
    err = hw_get_module(SENSORS_HARDWARE_MODULE_ID,(hw_module_t const**)&sSensorModule);
    if( 0 == err ) {
    err = sensors_open(&sSensorModule->common, &sSensorDevice);
        if( err ) {
            ALOGE("sensors_open error: %d\n", err);
            return -1;
        }
    sSensorCount = sSensorModule->get_sensors_list(sSensorModule, &sSensorList);
    ALOGD("sensor count = %d\n", sSensorCount);
    } else {
        ALOGD("hw_get_module('%s') error: %d\n", SENSORS_HARDWARE_MODULE_ID, err);
    return -2;
    }

    return 0;
}

static int eng_linuxcmd_getsensorinfo(char *req, char *rsp)
{
      int fd = -1, err = -1;
      int is_loader = -1;
      int len;
      char buffer[512];

    is_loader = init_sensorOpen();
    if (is_loader < 0) {
        ALOGE("open sensor failed!\n");
    } else {
        ALOGD("sensor has init!\n ");
    }

      fd = open("/sys/class/sprd_sensorhub/sensor_hub/sensor_info", O_RDONLY);

      if (fd < 0) {
          err = get_all_sensors_name(rsp);
          ALOGD("eng_linuxcmd_getsensorinfo from sSensorList");
      }else{
          memset(buffer,0,sizeof(buffer));
          len = read(fd, buffer, sizeof(buffer));
          err = sprintf(rsp, "%s%s", buffer, ENG_STREND);
      }
      close(fd);

      return 0;
}

void register_this_module(struct eng_callback * reg)
{
    ALOGD("file:%s, func:%s\n", __FILE__, __func__);
    sprintf(reg->at_cmd, "%s", "AT+GETSENSORINFO");
    reg->eng_linuxcmd_func = eng_linuxcmd_getsensorinfo;
    ALOGD("module cmd:%s\n", reg->at_cmd);
}

