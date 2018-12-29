/*
 * Copyright (C) 2012 STMicroelectronics
 * Matteo Dameno, Ciocca Denis, Alberto Marinoni, Giuseppe Barba
 * Motion MEMS Product Div.
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


/*
 * Library Version: 2.0.0
 */

#ifndef CONFIGURATION_HAL_H
#define CONFIGURATION_HAL_H

/* ANDROID API VERSION */
#define ANDROID_ICS				(14)
#define ANDROID_JB				(16)
#define ANDROID_JBMR2				(18)
#define ANDROID_KK				(19)
#define ANDROID_L				(21)
#define ANDROID_M				(23)

#if (ANDROID_VERSION >= ANDROID_JBMR2)
#define OS_VERSION_ENABLE			(1)
#else
#define OS_VERSION_ENABLE			(0)
#endif

#if defined(LSM330)
#include "conf_LSM330.h"
#endif

#if defined(APDS9900)
#include "conf_APDS9900.h"
#endif

#if defined(SENSOR_FUSION)
#include "conf_FUSION.h"
#endif

#if defined(AKM8963) || defined(AKM09911)
#include "conf_AKM.h"
#endif

#if defined(GBIAS)
#include "conf_GBIAS.h"
#endif

#if defined(FILE_CALIB)
#include "conf_FILE_CALIB.h"
#endif

#ifdef SENSORS_ORIENTATION_ENABLE
#if (SENSORS_ORIENTATION_ENABLE == 1)
#undef GEOMAG_COMPASS_ORIENTATION_ENABLE
#define GEOMAG_COMPASS_ORIENTATION_ENABLE (0)
#endif
#endif

#if (SENSORS_GRAVITY_ENABLE == 1)
#undef GEOMAG_GRAVITY_ENABLE
#define GEOMAG_GRAVITY_ENABLE (0)
#endif

#if (SENSORS_LINEAR_ACCELERATION_ENABLE == 1)
#undef GEOMAG_LINEAR_ACCELERATION_ENABLE
#define GEOMAG_LINEAR_ACCELERATION_ENABLE (0)
#endif

#ifndef SENSORS_GYROSCOPE_ENABLE
#define SENSORS_GYROSCOPE_ENABLE 		(0)
#endif

#ifndef SENSORS_ACCELEROMETER_ENABLE
#define SENSORS_ACCELEROMETER_ENABLE 		(0)
#endif

#ifndef SENSORS_MAGNETIC_FIELD_ENABLE
#define SENSORS_MAGNETIC_FIELD_ENABLE 	(0)
#endif

/* Sensors power consumption */
#if (GYROSCOPE_GBIAS_ESTIMATION_STANDALONE == 1)
#define UNCALIB_GYRO_POWER_CONSUMPTION		(GYRO_POWER_CONSUMPTION + ACCEL_POWER_CONSUMPTION)
#else
#define UNCALIB_GYRO_POWER_CONSUMPTION		(GYRO_POWER_CONSUMPTION + MAGN_POWER_CONSUMPTION + ACCEL_POWER_CONSUMPTION)
#endif

/* DEBUG INFORMATION */
#define DEBUG_ACCELEROMETER			(0)
#define DEBUG_MAGNETOMETER			(0)
#define DEBUG_GYROSCOPE				(0)
#define DEBUG_INEMO_SENSOR			(0)
#define DEBUG_PRESSURE_SENSOR			(0)
#define DEBUG_CALIBRATION			(0)
#define DEBUG_MAG_SI_COMPENSATION		(0)
#define DEBUG_VIRTUAL_GYROSCOPE			(0)
#define DEBUG_TILT				(0)
#define DEBUG_STEP_C				(0)
#define DEBUG_STEP_D				(0)
#define DEBUG_SIGN_M				(0)
#define DEBUG_POLL_RATE				(0)
#define DEBUG_ACTIVITY_RECO			(0)

#define STLOGI(...)				ALOGI(__VA_ARGS__)
#define STLOGE(...)				ALOGE(__VA_ARGS__)
#define STLOGD(...)				ALOGD(__VA_ARGS__)
#define STLOGD_IF(...)				ALOGD_IF(__VA_ARGS__)
#define STLOGE_IF(...)				ALOGE_IF(__VA_ARGS__)

#if !defined(ACCEL_MIN_ODR)
#define ACCEL_MIN_ODR				1
#endif

#if !defined(MAGN_MIN_ODR)
#define MAGN_MIN_ODR				1
#endif

#if !defined(GYRO_MIN_ODR)
#define GYRO_MIN_ODR				1
#endif

#if !defined(ORIENTATION_MIN_ODR)
#define ORIENTATION_MIN_ODR			1
#endif

#if !defined(FUSION_MIN_ODR)
#define FUSION_MIN_ODR				1
#endif

#if !defined(PRESS_TEMP_MIN_ODR)
#define PRESS_TEMP_MIN_ODR			1
#endif

#if !defined(VIRTUAL_GYRO_MIN_ODR)
#define VIRTUAL_GYRO_MIN_ODR			1
#endif

#endif	/*	CONFIGURATION_HAL_H	*/
