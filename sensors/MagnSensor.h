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

#ifndef ANDROID_MAGN_SENSOR_H
#define ANDROID_MAGN_SENSOR_H

#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

struct input_event;

class MagnSensor : public SensorBase {

private:
	bool mEnabled;
	bool mHasPendingEvent;
	static sensors_vec_t dataBuffer;
	static pthread_mutex_t dataMutex;
	InputEventCircularReader mInputReader;
	sensors_event_t mPendingEvent;
	uint32_t mPendingEventsMask;
	int mPendingEventsFlushCount;
	virtual bool setBufferData(sensors_vec_t *value);

public:
	MagnSensor();
	virtual ~MagnSensor();
	virtual int enable(int32_t handle, int enabled, int type);
	virtual int setDelay(int32_t handle, int64_t ns);
	virtual bool hasPendingEvents() const;
	virtual int getWhatFromHandle(int32_t handle);
	virtual int readEvents(sensors_event_t* data, int count);
	static bool getBufferData(sensors_vec_t *lastBufferedValues);
};

#endif  // ANDROID_COMPORI_SENSOR_H
