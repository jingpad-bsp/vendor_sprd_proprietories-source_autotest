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
 
#ifndef _SENSOR_CALI_H__
#define _SENSOR_CALI_H__

static int check_calibration_status();
static int remove_calibration_files();
static void *accsensorcali_thread(void *data);
static int check_offset_value();
static int store_offset_value();
int eng_linuxcmd_accsensor_calibration(char *req, char *rsp);

#endif //_SENSOR_CALI_H__