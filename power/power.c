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

//#define LOG_NDEBUG 0
#define LOG_TAG "PowerHAL"

#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <hardware/power.h>
#include <log/log.h>
#include <cutils/iosched_policy.h>
#include <utils/Timers.h>
#include <sys/resource.h>

#include "power.h"

static pthread_t interaction_tid, boost_tid;
static unsigned int cpu_max_freq[PROFILE_NUM] = {1596000, 1795000, 1996000};
static char h[8];

static char *hint_str(power_hint_t hint) {

	switch(hint) {
	    case POWER_HINT_VSYNC:
		return "POWER_HINT_VSYNC";
	    case POWER_HINT_INTERACTION:
		return "POWER_HINT_INTERACTION";
	    case POWER_HINT_VIDEO_ENCODE:
		return "POWER_HINT_VIDEO_ENCODE";
	    case POWER_HINT_VIDEO_DECODE:
		return "POWER_HINT_VIDEO_DECODE";
	    case POWER_HINT_LOW_POWER:
		return "POWER_HINT_LOW_POWER";
	    case POWER_HINT_CAM_PREVIEW:
		return "POWER_HINT_CAM_PREVIEW";
	    case POWER_HINT_CPU_BOOST:
		return "POWER_HINT_CPU_BOOST";
	    case POWER_HINT_LAUNCH_BOOST:
		return "POWER_HINT_LAUNCH_BOOST";
	    case POWER_HINT_SET_PROFILE:
		return "POWER_HINT_SET_PROFILE";
	    default:
		snprintf(h, 7, "%u", hint);
		return h;
	}
}

static void sysfs_write(const char *path, const char *value) {

	int len, fd, ret = 0;

	fd = open(path, O_WRONLY);
	if (fd < 0) {
		ALOGE("Error opening %s: %d (%s)", path, errno, strerror(errno));
		return;
	}

	len = strlen(value);
	if (write(fd, value, len) != len)
		ALOGE("Error writing to %s: %d (%s)", path, errno, strerror(errno));

	close(fd);
}

static void pm_qos_write(int *fd, const char *const path, int value) {

	*fd = open(path, O_WRONLY);
	if (*fd < 0) {
		ALOGE("Error opening %s: %d (%s)", path, errno, strerror(errno));
		return;
	}

	if (write(*fd, &value, sizeof(value)) < 0) {
		ALOGE("Error writing to %s: %d (%s)", path, errno, strerror(errno));
		close (*fd);
		*fd = -1;
	}
}

static void *boost_thread(void *data) {

	struct power_context *context = (struct power_context *)data;
	struct sched_param param = {0};
	int cpu_fd, err;

	param.sched_priority =10;
	err = sched_setscheduler(0, SCHED_FIFO, &param);
	if (err != 0)
		ALOGE("boost_thread: failed to set priority");
	android_set_rt_ioprio(gettid(), 1);

	while (true) {
		pthread_mutex_lock(&context->lock);
		while (!context->boost_enabled)
			pthread_cond_wait(&context->boost_cond, &context->lock);
		pthread_mutex_unlock(&context->lock);

		pm_qos_write(&cpu_fd, PM_QOS_CPU_MIN_FREQ, cpu_max_freq[context->profile]);

		/* PM QOS request is active while we are holding file descriptor opened. */
		usleep(BOOST_DURATION);

		if (cpu_fd >= 0)
			close(cpu_fd);

		context->boost_enabled = false;
	}

	return NULL;
}

static void *interaction_thread(void *data) {

	struct power_context *context = (struct power_context *)data;
	struct sched_param param = {0};
	int cpu_fd, err;
	nsecs_t timestamp;

	param.sched_priority = 10;
	err = sched_setscheduler(0, SCHED_FIFO, &param);
	if (err != 0)
		ALOGE("interaction_thread: failed to set priority");
	android_set_rt_ioprio(gettid(), 1);

	while (true) {
		pthread_mutex_lock(&context->lock);
		while (!context->interaction_enabled)
			pthread_cond_wait(&context->interaction_cond, &context->lock);
		pthread_mutex_unlock(&context->lock);

		pm_qos_write(&cpu_fd, PM_QOS_CPU_MIN_FREQ, CPU_INTERACTION_FREQ);

		/* There might have been new hints while we were sleeping.
		 * So keep holding fd opened until the last hint will expire.
		 */
		do {
			pthread_mutex_lock(&context->lock);
			nsecs_t now = ns2us(systemTime(SYSTEM_TIME_BOOTTIME));
			useconds_t usecs = context->usecs;
			timestamp = context->timestamp;
			pthread_mutex_unlock(&context->lock);

			if (timestamp + usecs > now)
				usleep((timestamp + usecs) - now);
		}
		while (timestamp != context->timestamp);

		if (cpu_fd >= 0)
			close(cpu_fd);

		context->interaction_enabled = false;
	}

	return NULL;
}

#define MIN(a, b)	a < b ? a : b

static void power_hint(struct power_module *module,
			power_hint_t hint, void *data)
{
	struct power_context *context = (struct power_context *)module;
	struct power_decoder_data *power_data;
	static int cpu_fd = -1, num_fd = -1;

	/* Video encoder & decoder use a new instance of power module,
	   so it has uninitialized context. Exclude them from check.
	 */
	if (!context->initialized &&
	    hint != POWER_HINT_VIDEO_DECODE && hint != POWER_HINT_VIDEO_ENCODE)
		return;

	switch (hint) {
	    case POWER_HINT_VIDEO_DECODE:
		if (!data)
			return;

		power_data = (struct power_decoder_data *)data;
		ALOGV("%s: executing = %u width = %u heigth = %u", hint_str(hint),
			power_data->executing, power_data->frameWidth, power_data->frameHeight);
		if (power_data->executing) {
			if (cpu_fd < 0) {
				uint32_t height = MIN(power_data->frameHeight, power_data->frameWidth);
				if (height <= 720) // up to HD
					pm_qos_write(&cpu_fd, PM_QOS_CPU_MIN_FREQ, 624000);
				else // FHD
					pm_qos_write(&cpu_fd, PM_QOS_CPU_MIN_FREQ, 1196000);
			}
		}
		else
			if (cpu_fd >= 0) {
				close(cpu_fd);
				cpu_fd = -1;
			}
		break;

	    case POWER_HINT_VIDEO_ENCODE:
		if (!data)
			return;

		ALOGV("%s: executing = %u", hint_str(hint), *(uintptr_t *)data);
		if (*(uintptr_t *)data == 1) { /* Executing */
			if (num_fd < 0)
				pm_qos_write(&num_fd, PM_QOS_CPU_NUMBER_MIN, NUM_CPUS);
			if (cpu_fd < 0) {
				FILE *f;
				char profile[12];
				/* Because of new instance we have to get current profile */
				f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_policy", "r");
				if (f != NULL) {
					fscanf(f, "%s", profile);
					fclose(f);
					if (!strcmp(profile, "performance"))
						context->profile = PROFILE_PERFORMANCE;
					else
					    if (!strcmp(profile, "normal"))
						    context->profile = PROFILE_BALANCE;
					    else
						    context->profile = PROFILE_POWERSAVE;
				}
				else {
					ALOGE("Failed to open scaling_cur_policy: %d (%s)", errno, strerror(errno));
					context->profile = PROFILE_POWERSAVE;
				}
				pm_qos_write(&cpu_fd, PM_QOS_CPU_MIN_FREQ, cpu_max_freq[context->profile]);
			}
		}
		else {
			if (cpu_fd >= 0) {
				close(cpu_fd);
				cpu_fd = -1;
			}
			if (num_fd >= 0) {
				close(num_fd);
				num_fd = -1;
			}
		}
		break;

	    case POWER_HINT_LAUNCH_BOOST:
		if (context->profile == PROFILE_POWERSAVE ||
		    context->boost_enabled)
			return;

		ALOGV("%s: pid = %u name = %s", hint_str(hint),
			((launch_boost_info_t *)data)->pid, ((launch_boost_info_t *)data)->packageName);
		pthread_mutex_lock(&context->lock);
		context->boost_enabled = true;
		pthread_cond_signal(&context->boost_cond);
		pthread_mutex_unlock(&context->lock);
		break;

	    case POWER_HINT_CPU_BOOST:
	    case POWER_HINT_INTERACTION:
		if (context->profile == PROFILE_POWERSAVE)
			return;

		pthread_mutex_lock(&context->lock);
		if (data != NULL && *(uintptr_t *)data > 0) {
			context->usecs = *(uintptr_t *)data;
			/* INTERACTION is in msecs and CPU_BOOST is in usecs */
			if (hint == POWER_HINT_INTERACTION)
				context->usecs *= 1000;
		}
		else
			context->usecs = INTERACTION_DURATION;
		context->timestamp = ns2us(systemTime(SYSTEM_TIME_BOOTTIME));

		if (context->interaction_enabled || context->boost_enabled) {
			pthread_mutex_unlock(&context->lock);
			return;
		}

		ALOGV("%s: %u", hint_str(hint), context->usecs);
		context->interaction_enabled = true;
		pthread_cond_signal(&context->interaction_cond);
		pthread_mutex_unlock(&context->lock);
		break;

	    case POWER_HINT_SET_PROFILE:
		ALOGV("%s: %u", hint_str(hint), *(uintptr_t *)data);
		if (*(uintptr_t *)data < PROFILE_NUM) {
			char path[64], value[12];

			context->profile = *(uintptr_t *)data;
			switch (context->profile) {
			    case PROFILE_POWERSAVE:
				strcpy(value, "powersave");
				break;
			    case PROFILE_BALANCE:
				strcpy(value, "normal");
				break;
			    case PROFILE_PERFORMANCE:
				strcpy(value, "performance");
				break;
			    default:
				value[0] = 0;
				break;
			}
			if (strlen(value) > 0)
				sysfs_write("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_policy", value);
		}
		break;

	    default:
		ALOGV("%s: hint = %s", __func__, hint_str(hint));
		break;
	}
}

static void power_init(struct power_module *module) {

	struct power_context *context = (struct power_context *)module;

	/* This function is called twice, firstly by SystemServer
	 * and secondly by CM's PerformanceManager. So we have to
	 * add this check to create threads only once.
	 */
	pthread_mutex_lock(&context->lock);
	if (!context->initialized) {
		context->profile = PROFILE_PERFORMANCE;

		if (pthread_create(&interaction_tid, NULL, interaction_thread, context)) {
			ALOGE("%s: failed to start interaction thread", __func__);
			return;
		}

		if (pthread_create(&boost_tid, NULL, boost_thread, context)) {
			ALOGE("%s: failed to start boost thread", __func__);
			return;
		}

		context->initialized = true;
		ALOGI("Power HAL for hi6620oem initialized");
	}
	pthread_mutex_unlock(&context->lock);
}

static void power_set_interactive(struct power_module *module, int on)
{
}

static int power_get_feature(struct power_module *module,
				feature_t feature)
{
	if (feature == POWER_FEATURE_SUPPORTED_PROFILES)
		return PROFILE_NUM;

	return -1;
}

static void power_set_feature(struct power_module *module,
			feature_t feature, int mode)
{
	char mode_str[2] = {0};

	if (feature == POWER_FEATURE_DOUBLE_TAP_TO_WAKE) {
		mode_str[0] = '0' + !!mode;
		sysfs_write(DT2W_PATH, mode_str);
	}
}

static struct hw_module_methods_t power_module_methods = {
	.open = NULL,
};

struct power_context HAL_MODULE_INFO_SYM = {
	.base = {
		.common = {
			.tag = HARDWARE_MODULE_TAG,
			.module_api_version = POWER_MODULE_API_VERSION_0_3,
			.hal_api_version = HARDWARE_HAL_API_VERSION,
			.id = POWER_HARDWARE_MODULE_ID,
			.name = "Power HAL Module For hi6620oem Platform",
			.author = "Kostyan_nsk",
			.methods = &power_module_methods,
		},
		.init = power_init,
		.setInteractive = power_set_interactive,
		.powerHint = power_hint,
		.setFeature = power_set_feature,
		.getFeature = power_get_feature
	},
	.lock = PTHREAD_MUTEX_INITIALIZER,
	.interaction_cond = PTHREAD_COND_INITIALIZER,
	.boost_cond = PTHREAD_COND_INITIALIZER,
	.interaction_enabled = false,
	.boost_enabled = false,
	.initialized = false,
};
