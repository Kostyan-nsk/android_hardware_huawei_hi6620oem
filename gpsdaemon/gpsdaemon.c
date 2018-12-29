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
#define LOG_TAG "gpsdaemon"

#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <log/log.h>
#include <cutils/properties.h>

#define GPIO_PATH	"/sys/kernel/debug/boardid/common/gps/gpio_gps_en"

int main(void) {
	int fd_in, fd_out, gpio, err = 0;
	struct stat fileinfo = {0};
	char filename[40];
	FILE *f;

	f = fopen(GPIO_PATH, "r");
	if (f == NULL) {
		ALOGE("failed to open " GPIO_PATH ": %d (%s)", errno, strerror(errno));
		return -errno;
	}

	if (!fscanf(f, "int:%d", &gpio)) {
		ALOGE("failed to read GPIO value");
		fclose(f);
		return -EILSEQ;
	}
	fclose(f);
	sprintf(filename, "/system/etc/gpsconfig_gpio%d.xml", gpio);
	ALOGV("gps config: %s", filename);

	if (stat(filename, &fileinfo) < 0) {
		ALOGE("failed to stat %s: %d (%s)", filename, errno, strerror(errno));
		return -errno;
	}

	fd_in = open(filename, O_RDONLY);
	if (fd_in < 0) {
		ALOGE("failed to open %s: %d (%s)", filename, errno, strerror(errno));
		return -errno;
	}

	fd_out = open("/data/gps/gpsconfig.xml", O_CREAT | O_WRONLY, (mode_t)0644);
	if (fd_out < 0) {
		ALOGE("failed to create/open /data/gps/gpsconfig.xml: %d (%s)", errno, strerror(errno));
		err = -errno;
		goto exit;
	}

	if (sendfile(fd_out, fd_in, NULL, fileinfo.st_size) != fileinfo.st_size) {
		ALOGE("failed to copy file %s: %d (%s)", filename, errno, strerror(errno));
		err = -errno;
	}

	close(fd_out);
exit:
	close(fd_in);

	if (!err)
		err = property_set("ctl.start", "gpsd");

	return err;
}
