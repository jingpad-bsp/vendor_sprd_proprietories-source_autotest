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

#include <sensors/convert.h>
#include <android/hardware/sensors/1.0/ISensors.h>


using android::hardware::hidl_vec;
using namespace android::hardware::sensors::V1_0;
using namespace android::hardware::sensors::V1_0::implementation;

// ---------------------------------------------------------------------------

using namespace android;
using android::hardware::Return;

Vector<sensor_t> sensor_list;
sp<android::hardware::sensors::V1_0::ISensors> mSensors;

int sensor_count = 0;
int senloaded = -1;

#define S_ON    1
#define S_OFF   0

// ---------------------------------------------------------------------------

void handleHidlDeath(const std::string & detail) {
    // restart is the only option at present.
    LOG_ALWAYS_FATAL("Abort due to ISensors hidl service failure, detail: %s.", detail.c_str());
}

template<typename T>
static Return<T> checkReturn(Return<T> &&ret) {
    if (!ret.isOk()) {
        handleHidlDeath(ret.description());
    }
    return std::move(ret);
}

int is_onchange_reporting_mode(int sensor_type)
{
	int i;

	for(i = 0 ; i < sensor_count; i++)
	{
		if(sensor_list[i].type == sensor_type)
		{
			LOGD("is_onchange_reporting_mode: ID = %d, sensor name = %s, type = %d!!", i, sensor_list[i].name, sensor_list[i].type);
			if(SENSOR_FLAG_ON_CHANGE_MODE & sensor_list[i].flags)
				return 1;
			else
				return 0;
		}
	}

	LOGE("failed to get the sensor flags!!");
	return -1;

}

int sensor_getlist()
{
    float minPowerMa = 0.001; // 1 microAmp

    checkReturn(mSensors->getSensorsList(
            [&](const auto &outlist) {
                sensor_count = outlist.size();

                for (int i=0 ; i < sensor_count; i++) {
                    sensor_t sensor;
                    convertToSensor(outlist[i], &sensor);
                    // Sanity check and clamp power if it is 0 (or close)
                    if (sensor.power < minPowerMa) {
                        LOGE("Reported power %f not deemed sane, clamping to %f",
                              sensor.power, minPowerMa);
                        sensor.power = minPowerMa;
                    }
                    sensor_list.push_back(sensor);

                    //checkReturn(mSensors->activate(outlist[i].sensorHandle, 0 /* enabled */));
                }
            }));
    return 1;
}

int get_sensor_id(int sensor_type)
{
	int i;

	for(i = 0 ; i < sensor_count; i++)
	{
		if(sensor_list[i].type == sensor_type)
		{
			LOGD("get sensor success! ID = %d, sensor name = %s, type = %d!!", i, sensor_list[i].name, sensor_list[i].type);
			return i;
		}
	}
	LOGE("failed to get the sensor ID!!");
	return -1;
}

static int activate_sensors(int id, int delay, int opt)
{
	int err = 0;

	LOGD("sensor parameters: dalay = %d; enable = %d! %d IN",
	     delay, opt, __LINE__);
	if (opt == S_ON){
		mSensors->batch(sensor_list[id].handle,
				us2ns(sensor_list[id].minDelay), ms2ns(delay));
		LOGD("Sets a sensor's parameters! %d IN", __LINE__);
		mSensors->flush(sensor_list[id].handle);
		LOGD("flush sensor event! %d IN", __LINE__);
	}
	mSensors->activate(sensor_list[id].handle, opt);
	LOGD("activate_sensors! %d IN", __LINE__);

	return err;
}

int sensor_poll(sensors_event_t* buffer, size_t count)
{
    if (mSensors == nullptr) return NO_INIT;

    ssize_t err;
    auto ret = mSensors->poll(
         count,
         [&](auto result,
             const auto &events,
             const auto &dynamicSensorsAdded) {
            if (result == Result::OK) {
                for (size_t i = 0; i < events.size(); ++i) {
                    convertToSensorEvent(events[i], &buffer[i]);
                }
                err = (ssize_t)events.size();
            } else {
                err = -1;
            }
         });

    return err;
}

int sensor_enable(int sensor_type)
{
	int id = -1;
	int rtn = -1;

	id = get_sensor_id(sensor_type);
	if (id != -1) {
		LOGD("activate_sensors(ON) for '%s'", sensor_list[id].name);
		rtn = activate_sensors(id, 50, S_ON);
	}
	if (rtn != 0 || id == -1)
	{
		LOGE("activate_sensors(ON) for '%s'failed", sensor_list[id].name);
		return -1;
	}

	return id;
}

int sensor_disable(int sensor_type)
{
	int id = -1;
	int err = -1;

	/*********activate_sensors(OFF)***************/
	id = get_sensor_id(sensor_type);
	if (id != -1){
		err = activate_sensors(id, 0, S_OFF);
	}
	if (err != 0 || id == -1) {
		LOGE("activate_sensors(OFF) for '%s'failed", sensor_list[id].name);
		return -1;
	}

	return id;
}

int sensor_load()
{
    // SensorDevice may wait upto 100ms * 10 = 1s for hidl service.
    constexpr auto RETRY_DELAY = std::chrono::milliseconds(100);
    size_t retry = 10;

    while (true) {
        int initStep = 0;
        mSensors = ISensors::getService();
        if (mSensors != nullptr) {
            if(mSensors->poll(0, [](auto, const auto &, const auto &) {}).isOk()) {
                // ok to continue
                break;
            }
            // hidl service is restarting, pointer is invalid.
            mSensors = nullptr;
        }

        if (--retry <= 0) {
            LOGE("Cannot connect to ISensors hidl service!");
            break;
        }
        // Delay 100ms before retry, hidl service is expected to come up in short time after
        // crash.
        LOGD("%s unsuccessful, try again soon (remaining retry %zu).",
                (initStep == 0) ? "getService()" : "poll() check", retry);
        std::this_thread::sleep_for(RETRY_DELAY);
    }

    if (mSensors != nullptr) {
        sensor_getlist();
    }
    return (mSensors != nullptr)?0:-1;
}
