/*
 * Copyright (C) 2013 The Android Open Source Project
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

#define LOG_TAG "memtrack"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <hardware/memtrack.h>
#include <log/log.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

struct memtrack_record record_templates[] = {
	{
	    .flags = MEMTRACK_FLAG_SMAPS_UNACCOUNTED |
		     MEMTRACK_FLAG_PRIVATE |
		     MEMTRACK_FLAG_NONSECURE,
	},
};

int memtrack_init(const struct memtrack_module *module)
{
	return 0;
}

static int mali_memtrack_get_memory(pid_t pid, enum memtrack_type type,
				    struct memtrack_record *records,
				    size_t *num_records)
{
	FILE *fp;
	char tmp[128], line[1024];
	char *name;
	size_t unaccounted_size = 0;
	unsigned long int start, end;
	int name_pos;

	size_t allocated_records = ARRAY_SIZE(record_templates);
	*num_records = ARRAY_SIZE(record_templates);

	if (records == NULL)
		return 0;

	memcpy(records, record_templates, sizeof(struct memtrack_record) * allocated_records);

	sprintf(tmp, "/proc/%d/smaps", pid);
	fp = fopen(tmp, "r");

	if (fp) {
		while (fgets(line, sizeof(line), fp)) {
			if (sscanf(line, "%lx-%lx %*s %*x %*x:%*x %*d%n", &start, &end, &name_pos) == 2) {
				while (isspace(line[name_pos]))
					name_pos++;
				name = line + name_pos;
				if (!strncmp(name, "/dev/mali", 9))
					unaccounted_size += end - start;
			}
		}
		fclose(fp);
	}
	else {
		ALOGE("open /proc/%d/smaps: %s", pid, strerror(errno));
		return -errno;
	}

	if (allocated_records > 0)
		records[0].size_in_bytes = unaccounted_size;

	return 0;
}

static int ion_memtrack_get_memory(pid_t pid, enum memtrack_type type,
				    struct memtrack_record *records,
				    size_t *num_records)
{
	FILE *fp;
	char line[1024];
	size_t unaccounted_size = 0;
	unsigned int line_size;
	int line_pid;

	size_t allocated_records = ARRAY_SIZE(record_templates);
	*num_records = ARRAY_SIZE(record_templates);

	if (records == NULL)
		return 0;

	memcpy(records, record_templates, sizeof(struct memtrack_record) * allocated_records);

	fp = fopen("/sys/kernel/debug/ion/gralloc", "r");

	if (fp) {
		while (fgets(line, sizeof(line), fp)) {
			if (sscanf(line, "%*s %u %u\n", &line_pid, &line_size) == 2) {
				if (line_pid == pid)
					unaccounted_size += line_size;
			}
		}
		fclose(fp);
	}
	else {
		ALOGE("/sys/kernel/debug/ion/gralloc: %s", strerror(errno));
		return -errno;
	}

	if (allocated_records > 0)
		records[0].size_in_bytes = unaccounted_size;

	return 0;
}

int memtrack_get_memory(const struct memtrack_module *module,
				pid_t pid,
				int type,
				struct memtrack_record *records,
				size_t *num_records)
{
	if (type == MEMTRACK_TYPE_GL)
		return mali_memtrack_get_memory(pid, type, records, num_records);
	else if (type == MEMTRACK_TYPE_GRAPHICS)
		return ion_memtrack_get_memory(pid, type, records, num_records);
	     else
		return -EINVAL;
}

static struct hw_module_methods_t memtrack_module_methods = {
	.open = NULL,
};

struct memtrack_module HAL_MODULE_INFO_SYM = {
	.common = {
		.tag = HARDWARE_MODULE_TAG,
		.module_api_version = MEMTRACK_MODULE_API_VERSION_0_1,
		.hal_api_version = HARDWARE_HAL_API_VERSION,
		.id = MEMTRACK_HARDWARE_MODULE_ID,
		.name = "Memory Tracker HAL Module For hi6620oem Platform",
		.author = "Kostyan_nsk",
		.methods = &memtrack_module_methods,
	},
	.init = memtrack_init,
	.getMemory = memtrack_get_memory,
};
