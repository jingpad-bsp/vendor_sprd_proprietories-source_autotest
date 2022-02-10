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

#ifdef NONSENSORHUB
#include "sensor_calibration.h"
#endif

#ifdef SENSORHUB
//#include "sensor_native_mmi_cali.h"
#endif

#include "sensor_bbat_test.h"
#include "sensor_native_mmi_test.h"

#include "sprd_fts_type.h"
#include "sprd_fts_diag.h"
#include "sprd_fts_log.h"

extern "C" {
 void register_this_module_ext(struct eng_callback *reg, int *num)
{
	unsigned int moudles_num = 0;

	(reg + moudles_num)->type = BBAT_TEST;
	(reg + moudles_num)->subtype = BBAT_SENSOR_TEST;
	(reg + moudles_num)->eng_diag_func = diag_sensor_test_interface;
	LOGD("register module for Diag cmd diag_sensor_test_interface : [%p] \n", diag_sensor_test_interface);
	moudles_num ++;

/*
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
*/

	ALOGD("so_sensor:file:%s, func:%s\n", __FILE__, __func__);
	sprintf((reg + moudles_num)->at_cmd, "%s", "AT+SENSORTEST");
	(reg + moudles_num)->eng_linuxcmd_func = eng_linuxcmd_sensortest;
	moudles_num ++;


#ifdef NONSENSORHUB
	(reg + moudles_num)->type = 0x39;
	LOGD("file:%s, func:%s\n", __FILE__, __func__);
	sprintf((reg + moudles_num)->at_cmd, "%s", "AT+SENSORCALI");
	(reg + moudles_num)->eng_linuxcmd_func = eng_linuxcmd_accsensor_calibration;
	LOGD("module cmd:%s\n", (reg + moudles_num)->at_cmd);
	moudles_num ++;
#endif

#ifdef SENSORHUB
	(reg + moudles_num)->type = 0x39;
	LOGD("file:%s, func:%s\n", __FILE__, __func__);
	sprintf((reg + moudles_num)->at_cmd, "%s", "AT+SENSORCALI");
	(reg + moudles_num)->eng_linuxcmd_func = eng_linuxcmd_sensorcali;
	LOGD("module cmd:%s\n", (reg + moudles_num)->at_cmd);
	moudles_num ++;
#endif


	*num = moudles_num;
	return;
}
}
