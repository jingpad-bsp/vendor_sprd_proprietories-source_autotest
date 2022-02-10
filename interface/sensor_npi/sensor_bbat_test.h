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

#ifndef _SENSOR_BBAT_TEST_H__
#define _SENSOR_BBAT_TEST_H__

#include "sensor.h"

#define BBAT_TEST 0x38
#define BBAT_SENSOR_TEST 0x15
// This macro represent the code bbat Autotester tool group defined.
#define SPECIFICAL_SENSOR_TEST 0x20

enum {
	BBAT_TEST_SUCCESS = 0,
	BBAT_TEST_FAILED
};

/*
 * struct sensor_event_and_num - used by bbat test function
 * @sensor_event: in this bbat test module, store the type of sensor to test in sensors_event's type member.
 *				you can see this in diag_sensor_test_interface() function. and then when in test function,
 *				use sensor_event to store the sensor data event
 * @least_events_num_needed: this member store the least data a test should get to complete the task
 * @events_num_received: store the sensor data events num received
 */
typedef struct sensor_event_and_num {
	sensors_event_t sensor_event;
	int least_events_num_needed;
	int events_num_received;
} sensor_event_and_num;

enum BBAT_SENSOR_SKD_SINGLE_TEST_TYPE {
/* the type value in bbat test is defined early by guys of develop AutoTester tool and engmode tool,
 * if you want to change the value or add new sensor, you should ask them to redefine the type value.
 */
	ACCELEROMETER_SKD_SINGLE_TEST = 0x1,
	LIGHT_AND_PROXIMITY_SKD_SINGLE_TEST = 0x2,
	GYROSCOPE_SKD_SINGLE_TEST = 0x3,
	MAGNETIC_FIELD_SKD_SINGLE_TEST = 0x4,
	PRESSURE_SKD_SINGLE_TEST = 0x5,
};

enum LIGHT_AND_PROXIMITY_SENSOR {
/* the type value in bbat test is defined early by guys of develop AutoTester tool and engmode tool,
 * if you want to change the value or add new sensor, you should ask them to redefine the type value.
 */
	LIGHT = 0x1,
	PROXIMITY = 0x2,
};

int diag_sensor_test_interface(char *buf, int buf_len, char *rsp, int rsplen);

#endif //_SENSOR_BBAT_TEST_H__

