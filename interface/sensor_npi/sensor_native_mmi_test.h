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

#ifndef _SENSOR_NATIVEMMI_TEST_H__
#define _SENSOR_NATIVEMMI_TEST_H__

static void *mmi_get_sensor_data_thread(void *sensor_type);
static int sensor_test(int cmd, int type);
int eng_linuxcmd_sensortest(char *req, char *rsp);

#ifdef SENSORHUB
#define LIGHT_SENSOR_CALIBRATOR "/sys/class/sprd_sensorhub/sensor_hub/light_sensor_calibrator"
#define SENSOR_CALI_CMD "/sys/class/sprd_sensorhub/sensor_hub/calibrator_cmd"
#define SENSOR_CALI_DATA "/sys/class/sprd_sensorhub/sensor_hub/calibrator_data"

typedef enum {
    CALIB_EN,
    CALIB_CHECK_STATUS,
    CALIB_DATA_WRITE,
    CALIB_DATA_READ,
    CALIB_FLASH_WRITE,
    CALIB_FLASH_READ,
} CALIBRATOR_CMD;

static void *mmi_get_sensor_data_thread(void *sensor_type);
static int sensor_calibration(int enable, int cali_type, int type);
int eng_linuxcmd_sensorcali(char *req, char *rsp);
#endif //SENSORHUB

#endif //_SENSOR_NATIVEMMI_TEST_H__