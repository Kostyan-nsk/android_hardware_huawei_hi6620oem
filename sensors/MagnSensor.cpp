/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2016 The CyanogenMod Project
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

#include <fcntl.h>
#include <string.h>
#include <cutils/log.h>

#include "MagnSensor.h"

sensors_vec_t  MagnSensor::dataBuffer;
pthread_mutex_t MagnSensor::dataMutex;

MagnSensor::MagnSensor()
	: SensorBase(NULL, SENSOR_DATANAME_MAGNETIC),
	mInputReader(10),
	 mPendingEventsMask(0)
{
	ALOGV("MagnSensor: Initializing...");

	pthread_mutex_init(&dataMutex, NULL);
	mEnabled = false;
	mPendingEvent.version = sizeof(sensors_event_t);
	mPendingEvent.sensor = ID_MAGNETIC_FIELD;
	mPendingEvent.type = SENSOR_TYPE_MAGNETIC_FIELD;
	memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
}

MagnSensor::~MagnSensor() {}

int MagnSensor::enable(int32_t handle, int en, int  __attribute__((unused))type)
{
	ALOGV("MagnSensor: enable %d %d", handle, en);

	en = !!en;

	if (mEnabled == en)
		return 0;

	int fd = open(MAGNETIC_ENABLE_FILE_NAME, O_WRONLY);
	if (fd < 0) {
		ALOGE("MagnSensor: could not open %s: %d", MAGNETIC_ENABLE_FILE_NAME, fd);
		return fd;
	}

	char buf[2] = {0};
	sprintf(buf, "%d", en);
	int ret = write(fd, buf, 2);
	if (ret < 0) {
		ALOGE("MagnSensor: could not set state of handle %d to %d", handle, en);
		close(fd);
		return ret;
	}

	close(fd);
	mEnabled = en;

	return 0;
}

int MagnSensor::setDelay(int32_t handle, int64_t delay_ns)
{
	ALOGV("MagnSensor: setDelay %d %lld", handle, delay_ns);

	if (!mEnabled)
		return 0;

	if (delay_ns < MAGNETIC_MIN_DELAY_NS)
		delay_ns = MAGNETIC_MIN_DELAY_NS;

	int fd = open(MAGNETIC_DELAY_FILE_NAME, O_WRONLY);
	if (fd >= 0) {
		char buf[24];
		int ret;
		sprintf(buf, "%lld", delay_ns);
		ret = write(fd, buf, strlen(buf)+1);
		close(fd);
		if (ret < 0)
			ALOGE("MagnSensor: could not set delay of handle %d to %lld", handle, delay_ns);

		return ret >= 0 ? 0 : ret;
	}

	return fd;
}

int MagnSensor::getWhatFromHandle(int32_t __attribute__((unused))handle)
{
	return 0;
}

bool MagnSensor::hasPendingEvents() const
{
	return mHasPendingEvent;
}

int MagnSensor::readEvents(sensors_event_t* data, int count)
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
		if (event->type == EV_ABS)
			switch (event->code) {
			    case EVENT_TYPE_MAGV_X:
				mPendingEvent.magnetic.x = event->value * CONVERT_M_X;
			    break;
			    case EVENT_TYPE_MAGV_Y:
				mPendingEvent.magnetic.y = event->value * CONVERT_M_Y;
			    break;
			    case EVENT_TYPE_MAGV_Z:
				mPendingEvent.magnetic.z = event->value * CONVERT_M_Z;
			    break;
			    case EVENT_TYPE_MAGV_STATUS:
				mPendingEvent.magnetic.status = event->value;
			    break;
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
				ALOGE("MagnSensor: unknown event (type=%d, code=%d)", event->type, event->code);

		mInputReader.next();
	}

    return numEventReceived;
}

bool MagnSensor::setBufferData(sensors_vec_t *value)
{
	pthread_mutex_lock(&dataMutex);
	memcpy(&dataBuffer, value, sizeof(sensors_vec_t));
	pthread_mutex_unlock(&dataMutex);

	return true;
}

bool MagnSensor::getBufferData(sensors_vec_t *lastBufferedValues)
{
	pthread_mutex_lock(&dataMutex);
	memcpy(lastBufferedValues, &dataBuffer, sizeof(sensors_vec_t));
	pthread_mutex_unlock(&dataMutex);

	ALOGV("MagnSensor: getBufferData got values: x:(%f),y:(%f), z:(%f).",
						lastBufferedValues->x,
						lastBufferedValues->y,
						lastBufferedValues->z);

	return true;
}
