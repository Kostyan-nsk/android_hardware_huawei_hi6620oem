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


#ifndef CONFIGURATION_APDS9900_H
#define CONFIGURATION_APDS9900_H

#define SENSORS_LIGHT_ENABLE		(1)
#define SENSORS_PROXIMITY_ENABLE	(1)

/* AMBIENT LIGHT SENSOR */
#define SENSOR_LIGHT_LABEL		"APDS9900 Light sensor"			// Label views in Android Applications
#define SENSOR_DATANAME_ALS		"als_input"				// Name of input device: struct input_dev->name
#define ALS_DELAY_FILE_NAME		"device/als_poll_delay"			// name of sysfs file for setting the pollrate
#define ALS_ENABLE_FILE_NAME		"device/enable_als_sensor"		// name of sysfs file for enable/disable the sensor state
#define ALS_MAX_RANGE			3000.0f
#define ALS_RESOLUTION			1.0f
#define ALS_POWER_CONSUMPTION		0.75f					// Set sensor's power consumption [mA]

/* PROXIMITY SENSOR */
#define SENSOR_PROXIMITY_LABEL		"APDS9900 Proximity sensor"		// Label views in Android Applications
#define SENSOR_DATANAME_PS		"ps_input"				// Name of input device: struct input_dev->name
#define PROXIMITY_ENABLE_FILE_NAME	"device/enable_ps_sensor"		// name of sysfs file for enable/disable the sensor state
#define PROXIMITY_MAX_RANGE		5.0f
#define PROXIMITY_RESOLUTION		5.0f
#define PROXIMITY_POWER_CONSUMPTION	0.75f					// Set sensor's power consumption [mA]

/*****************************************************************************/
/* EVENT TYPE */
/*****************************************************************************/
#define EVENT_TYPE_ALS			ABS_MISC
#define EVENT_TYPE_PS			ABS_DISTANCE

#endif	/*	CONFIGURATION_APDS9900_H	*/
