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

#ifndef _SENSOR_H__
#define _SENSOR_H__

#include <hardware/sensors.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <utils/KeyedVector.h>
#include <utils/Singleton.h>
#include <utils/String8.h>
#include <utils/StrongPointer.h>
#include <unordered_map>
#include <algorithm> //std::max std::min
#include <thread>

int sensor_load();
int sensor_disable(int sensor_type);
int sensor_enable(int sensor_type);
int sensor_poll(sensors_event_t* buffer, size_t count);
int sensor_getlist();
int is_onchange_reporting_mode(int sensor_type);

#if defined(ANDROID)
#include <android/log.h>
#include <utils/Log.h>

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

#endif // _SENSOR_H__
