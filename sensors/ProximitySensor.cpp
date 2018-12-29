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

#include "ProximitySensor.h"

/*****************************************************************************/

ProximitySensor::ProximitySensor()
    : SensorBase(NULL, SENSOR_DATANAME_PS),
      mInputReader(4), mEnabled(0), mHasPendingEvent(false)
{
    ALOGV("ProximitySensor: Initializing...");

    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_PROXIMITY;
    mPendingEvent.type = SENSOR_TYPE_PROXIMITY;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
}

ProximitySensor::~ProximitySensor() {
}

int ProximitySensor::enable(int32_t handle, int en, int  __attribute__((unused))type)
{
	ALOGV("ProximitySensor: enable %d %d", handle, en);

	en = !!en;

	if (mEnabled == en)
		return 0;

	int err = writeEnable(SENSORS_PROXIMITY_HANDLE, en);
	if (err >= 0)
		err = 0;
	else
		goto err;

	mEnabled = en;

err:
	return err;
}

int ProximitySensor::getWhatFromHandle(int32_t __attribute__((unused))handle)
{
	return 0;
}

bool ProximitySensor::hasPendingEvents() const
{
	return mHasPendingEvent;
}

int ProximitySensor::readEvents(sensors_event_t* data, int count)
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
	input_event const* event;

	while (count && mInputReader.readEvent(&event)) {
		if (event->type == EV_ABS) {
			if (event->code == EVENT_TYPE_PS)
				mPendingEvent.distance = event->value * 5;
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
				ALOGE("ProximitySensor: unknown event (type=%d, code=%d)", event->type, event->code);

		mInputReader.next();
	}

	return numEventReceived;
}
