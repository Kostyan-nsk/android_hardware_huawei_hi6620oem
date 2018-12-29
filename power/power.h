/*
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

#define DT2W_PATH		"/sys/android_touch/doubletap2wake"
#define PM_QOS_CPU_MIN_FREQ	"/dev/cpu_minprofile"
#define PM_QOS_CPU_MAX_FREQ	"/dev/cpu_maxprofile"
#define PM_QOS_CPU_NUMBER_MIN	"/dev/cpu_number_min"
#define NUM_CPUS		4
#define BOOST_DURATION		2000000   /* 2 seconds  */
#define INTERACTION_DURATION	200000    /* 200 msec */
#define CPU_INTERACTION_FREQ	624000

enum {
	PROFILE_POWERSAVE,
	PROFILE_BALANCE,
	PROFILE_PERFORMANCE,
	PROFILE_NUM
};

struct power_decoder_data {
    bool executing;
    uint32_t frameWidth;
    uint32_t frameHeight;
};

struct power_context {
	struct power_module base;

	pthread_mutex_t lock;
	pthread_cond_t interaction_cond;
	pthread_cond_t boost_cond;
	nsecs_t timestamp;
	useconds_t usecs;
	unsigned int profile;
	bool interaction_enabled;
	bool boost_enabled;
	bool initialized;
};
