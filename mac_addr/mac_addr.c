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

#define LOG_TAG "mac_addr"

#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <log/log.h>
#include <private/android_filesystem_config.h>

#include <nvme.h>

#define MACWIFI		"/data/misc/wifi/macwifi"
#define MACBT		"/data/misc/bluedroid/macbt"

static void write_mac(const char *const path, const u_char *addr, short owner) {

	const char colon = ':';
	int i, fd;

	fd = open(path, O_CREAT | O_WRONLY, 0);
	if (fd < 0) {
		ALOGE("failed to create/open %s: %d (%s)", path, errno, strerror(errno));
		return;
	}

	for (i = 0; i < 12; i += 2) {
		write(fd, addr + i, 2);
		if (i < 10)
			write(fd, &colon, 1);
	}
	close(fd);

	if (chown(path, owner, owner) < 0)
		ALOGE("failed to set uid/gid for %s: %d (%s)", path, errno, strerror(errno));
	if (chmod(path, (mode_t)0640) < 0)
		ALOGE("failed to set premissions for %s: %d (%s)", path, errno, strerror(errno));
}

int main(void) {
	int err;
	struct nve_info_user info;

	info.nv_operation = NV_READ;
	info.valid_size = 20;
	info.nv_number = NVE_MACWLAN;
	memset(info.nv_data, 0, NVE_NV_DATA_SIZE);

	err = nvme_access(&info);
	if (!err) {
		ALOGI("wifi mac = %s", info.nv_data);
		write_mac(MACWIFI, info.nv_data, AID_WIFI);
	}
	else
		ALOGE("failed to read wifi mac: %d (%s)", err, strerror(err));

	info.nv_number = NVE_MACBT;
	memset(info.nv_data, 0, NVE_NV_DATA_SIZE);
	err = nvme_access(&info);
	if (!err) {
		ALOGI("bt mac = %s", info.nv_data);
		write_mac(MACBT, info.nv_data, AID_BLUETOOTH);
	}
	else
		ALOGE("failed to read bt mac: %d (%s)", err, strerror(err));

	return 0;
}
