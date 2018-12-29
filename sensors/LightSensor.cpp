/*
 * Copyright (C) 2012 The Android Open-Source Project
 * Copyright (C) 2014 Rudolf Tammekivi <rtammekivi@gmail.com>
 * Copyright (C) 2016 Kostyan_nsk
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

#define LOG_NDEBUG 0

#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>

#include "LightSensor.h"

/*****************************************************************************/

LightSensor::LightSensor()
	: SensorBase(NULL, SENSOR_DATANAME_ALS),
	mInputReader(4), mEnabled(0), mHasPendingEvent(false)
{
	ALOGV("LightSensor: Initializing...");

	mPendingEvent.version = sizeof(sensors_event_t);
	mPendingEvent.sensor = ID_LIGHT;
	mPendingEvent.type = SENSOR_TYPE_LIGHT;
	memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
}

LightSensor::~LightSensor() {}

int LightSensor::enable(int32_t handle, int en, int  __attribute__((unused))type)
{
	ALOGV("LightSensor: enable %d %d", handle, en);

	en = !!en;

	if (mEnabled == en)
		return 0;

	int err = writeEnable(SENSORS_LIGHT_HANDLE, en);
	if (err >= 0)
		err = 0;
	else
		goto err;

	mEnabled = en;

err:
	return err;
}

int LightSensor::setDelay(int32_t handle, int64_t delay_ns)
{
	ALOGV("LightSensor: setDelay %d %lld", handle, delay_ns);

	if (!mEnabled)
		return 0;

	int64_t delay_ms = NSEC_TO_MSEC(delay_ns);
	int err = 0;

	if (delay_ms == 0)
		return -EINVAL;

	err = writeDelay(handle, delay_ms);

	return err >= 0 ? 0 : err;
}

int LightSensor::getWhatFromHandle(int32_t __attribute__((unused))handle)
{
	return 0;
}

bool LightSensor::hasPendingEvents() const
{
	return mHasPendingEvent;
}

int LightSensor::readEvents(sensors_event_t* data, int count)
{
	if (count < 1)
		return -EINVAL;

	if (mHasPendingEvent) {
		mHasPendingEvent = false;
	}

	ssize_t n = mInputReader.fill(data_fd);
	if (n < 0)
		return n;

	int numEventReceived = 0;
	input_event const *event;

	while (count && mInputReader.readEvent(&event)) {

		if (event->type == EV_ABS) {
			if (event->code == EVENT_TYPE_ALS)
				mPendingEvent.light = event->value;
		}
		else
			if (event->type == EV_SYN) {
				mPendingEvent.timestamp = timevalToNano(event->time);
				if (mEnabled) {
					*data++ = mPendingEvent;
					count--;
					numEventReceived++;
				}
			}
			else
				ALOGE("LightSensor: unknown event (type=%d, code=%d)", event->type, event->code);
		mInputReader.next();
	}

	return numEventReceived;
}
