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


#ifndef CONFIGURATION_AKM_H
#define CONFIGURATION_AKM_H

#define SENSORS_MAGNETIC_FIELD_ENABLE	(1)

/* MAGNETIC FIELD SENSOR */
#define SENSOR_DATANAME_MAGNETIC	"compass_input"				// Name of input device: struct input_dev->name
#ifdef AKM_DEVICE_AK8963
#define SENSOR_MAGN_LABEL		"AK8963 3-axis Magnetic field sensor"	// Label views in Android Applications
#define MAGNETIC_DELAY_FILE_NAME	"/sys/class/compass/akm8963/delay_mag"	// name of sysfs file for setting the pollrate
#define MAGNETIC_ENABLE_FILE_NAME	"/sys/class/compass/akm8963/enable_mag"	// name of sysfs file for enable/disable the sensor state
#elif defined(AKM_DEVICE_AK09911)
#define SENSOR_MAGN_LABEL		"AK09911 3-axis Magnetic field sensor"	// Label views in Android Applications
#define MAGNETIC_DELAY_FILE_NAME	"/sys/class/compass/akm09911/delay_mag"	// name of sysfs file for setting the pollrate
#define MAGNETIC_ENABLE_FILE_NAME	"/sys/class/compass/akm09911/enable_mag"	// name of sysfs file for enable/disable the sensor state
#endif
#define MAGNETIC_MAX_RANGE		20000.0f
#define MAGNETIC_RESOLUTION		0.0625f
#define MAGNETIC_POWER_CONSUMPTION	0.28f					// Set sensor's power consumption [mA]
#define MAGNETIC_MIN_DELAY_NS		200000000LL /* nanoseconds */

/* SENSOR FUSION */
#define MAG_GBIAS_THRESHOLD		1200e-6					// Set magnetometer gbias threshold [uT]

/* INEMO_ENGINE SENSOR */
#define MAG_DEFAULT_RANGE		8					// full scale set to +-2.5Gauss (value depends on the driver sysfs file)
#define MAG_DEFAULT_DELAY		10					// 1/frequency (default: 10 -> 100 Hz) [ms]

/*****************************************************************************/
/* EVENT TYPE */
/*****************************************************************************/
#define EVENT_TYPE_MAGV_X		ABS_RX
#define EVENT_TYPE_MAGV_Y		ABS_RY
#define EVENT_TYPE_MAGV_Z		ABS_RZ
#define EVENT_TYPE_MAGV_STATUS		ABS_RUDDER

#define EVENT_TYPE_YAW			ABS_HAT0X
#define EVENT_TYPE_PITCH		ABS_HAT0Y
#define EVENT_TYPE_ROLL			ABS_HAT1X
#define EVENT_TYPE_ORIENT_STATUS	ABS_HAT1Y

/* conversion of magnetic data to uT units */
#define CONVERT_M                   (0.06f)
#define CONVERT_M_X                 (CONVERT_M)
#define CONVERT_M_Y                 (CONVERT_M)
#define CONVERT_M_Z                 (CONVERT_M)

#endif	/*	CONFIGURATION_AKM8963_H	*/
