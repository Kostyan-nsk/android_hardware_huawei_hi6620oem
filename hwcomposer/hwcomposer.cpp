/*
 * Copyright (C) 2010 The Android Open Source Project
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
#define LOG_TAG "hwcomposer"

#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/prctl.h>

#include <log/log.h>
#include <sync/sync.h>
#include <cutils/uevent.h>
#include <cutils/iosched_policy.h>
#include <utils/Timers.h>
#include <utils/String8.h>
#include <utils/AndroidThreads.h>
#include <utils/ThreadDefs.h>

#include <gralloc_priv.h>
#include "hwcomposer.h"

//#define DUMP_LAYERS
#ifdef DUMP_LAYERS
#include "bmp.h"
#include <fcntl.h>
static uint16_t frame = 0;
#endif

static pthread_cond_t hwc_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t hwc_mutex = PTHREAD_MUTEX_INITIALIZER;
static fb_overlay overlay;

static void dump_handle(buffer_handle_t h)
{
	const private_handle_t *handle = reinterpret_cast<const private_handle_t *>(h);
	ALOGD("handle:\n\tphy_addr = %lu\n\tformat = %d\n\twidth = %u\n\theight = %u\n\tstride = %u",
		handle->phys_addr, handle->format, handle->width, handle->height, handle->stride);
}

static void dump_layer(hwc_layer_1_t const *layer)
{
	ALOGD("\ttype = %d\n\tflags = %08x\n\thandle = %p\n\ttransform = %02x\n\tblending = %04x\n\tplaneAlpha = %01x"
		"\n\tsourceCrop = [%d, %d, %d, %d]"
		"\n\tdisplayFrame = [%d, %d, %d, %d}",
		layer->compositionType, layer->flags, layer->handle, layer->transform, layer->blending, layer->planeAlpha,
		(int)layer->sourceCropf.left, (int)layer->sourceCropf.top, (int)layer->sourceCropf.right, (int)layer->sourceCropf.bottom,
		layer->displayFrame.left, layer->displayFrame.top, layer->displayFrame.right, layer->displayFrame.bottom);

	if (layer->handle)
		dump_handle(layer->handle);
}

static void hwc_dump(hwc_composer_device_1* dev, char *buff, int buff_len)
{
	if (buff_len <= 0)
		return;

	struct hwc_context_1_t *context = (struct hwc_context_1_t *)dev;
	android::String8 result;

	result.appendFormat("    w = %u, h = %u, xdpi = %f, ydpi = %f, vsync_period = %u",
	context->gralloc->info.xres,
	context->gralloc->info.yres,
	context->gralloc->xdpi,
	context->gralloc->ydpi,
	context->vsync_period);
	result.append("\n");

	strlcpy(buff, result.string(), buff_len);
}
/* hwc 1.4
static int hwc_setPowerMode(struct hwc_composer_device_1 *dev, int disp, int mode)
{
	struct hwc_context_1_t *context = (struct hwc_context_1_t *)dev;
	int ret = -EINVAL, value;

	if (disp == HWC_DISPLAY_PRIMARY) {
		switch(mode) {
			case HWC_POWER_MODE_OFF:
			case HWC_POWER_MODE_DOZE:
			case HWC_POWER_MODE_DOZE_SUSPEND:
				value = FB_BLANK_POWERDOWN;
				context->blank = value;
				break;
			case HWC_POWER_MODE_NORMAL:
			default:
				value = FB_BLANK_UNBLANK;
				context->blank = value;
				break;
		}
		ret = ioctl(context->gralloc->framebuffer->fd, FBIOBLANK, value);
    }
    return ret;
}
*/
static int hwc_blank(struct hwc_composer_device_1* dev, int disp, int blank)
{
	struct hwc_context_1_t *context = (struct hwc_context_1_t *)dev;
	int ret = 0;

#ifdef DUMP_LAYERS
	if (!blank)
		frame = 0;
#endif
	if (disp == HWC_DISPLAY_PRIMARY) {
		ret = ioctl(context->gralloc->framebuffer->fd, FBIOBLANK, blank);
		if (ret == 0)
			context->blank = blank;
		else
			ALOGE("FBIOBLANK %d error!", blank);
	}

    return ret;
}

static int hwc_query(struct hwc_composer_device_1 *dev, int what, int *value)
{
	struct hwc_context_1_t *context = (struct hwc_context_1_t *)dev;

	switch (what) {
	    case HWC_BACKGROUND_LAYER_SUPPORTED:
		/* we don't support background layer */
		*value = 0;
		break;
	    case HWC_VSYNC_PERIOD:
		/* vsync period in nanoseconds */
		*value = context->vsync_period;
		break;
	    case HWC_DISPLAY_TYPES_SUPPORTED:
		/* ATM we support only primary and virtual displays */
		*value = HWC_DISPLAY_PRIMARY_BIT | HWC_DISPLAY_VIRTUAL_BIT;
		break;
	    default:
		/* unsupported query */
		return -EINVAL;
	}

    return 0;
}

static int hwc_eventControl(struct hwc_composer_device_1* dev, int disp, int event, int enabled)
{
	struct hwc_context_1_t *context = (struct hwc_context_1_t *)dev;
	int err;

	if (disp != HWC_DISPLAY_PRIMARY)
		return -EINVAL;

	switch (event) {
	    case HWC_EVENT_VSYNC:
		ALOGV("HWC_EVENT_VSYNC enabled = %d", enabled);
		err = ioctl(context->gralloc->framebuffer->fd, K3FB_VSYNC_INT_SET, &enabled);
		if (!err) {
			context->vsync_enabled = enabled;
			if (enabled)
				pthread_cond_signal(&hwc_cond);
		}
		else
			ALOGE("K3FB_VSYNC_INT_SET %d returned error!", enabled);
		break;
	    default:
		ALOGE("%s: unsupported event %d", __func__, event);
		return -EINVAL;
	}

	return 0;
}

static inline void hwc_unset(int fb_fd, int id)
{
	if (ioctl(fb_fd, K3FB_OVERLAY_UNSET, &id) != 0)
		ALOGE("K3FB_OVERLAY_UNSET returned error!");
}

static int hwc_prepare(struct hwc_composer_device_1 *dev, size_t numDisplays,
					hwc_display_contents_1_t** displays)
{
	hwc_context_1_t *context = (hwc_context_1_t *)dev;
	hwc_display_contents_1_t *display_content = NULL;
	int ret = 0, fb_fd = context->gralloc->framebuffer->fd;

	if (!numDisplays || !displays)
		return 0;

	display_content = displays[HWC_DISPLAY_PRIMARY];
	if (display_content) {
		hwc_layer_1_t* layer = &display_content->hwLayers[display_content->numHwLayers - 1];

		if (!overlay.enabled) {
			overlay.info.id = 1;
			ret = ioctl(fb_fd, K3FB_OVERLAY_GET, &overlay.info);
			if (ret != 0) {
				ALOGE("K3FB_OVERLAY_GET returned error!");
				return -errno;
			}
			overlay.enabled = true;
			overlay.data.id = overlay.info.id = overlay.info.is_overlay_compose = 1;

			overlay.info.src_rect.x = layer->displayFrame.left;
			overlay.info.src_rect.y = layer->displayFrame.top;
			overlay.info.src_rect.h = layer->displayFrame.bottom - layer->displayFrame.top;
			overlay.info.src_rect.w = layer->displayFrame.right - layer->displayFrame.left;

			overlay.info.dst_rect.x = layer->displayFrame.left;
			overlay.info.dst_rect.y = layer->displayFrame.top;
			overlay.info.dst_rect.h = layer->displayFrame.bottom - layer->displayFrame.top;
			overlay.info.dst_rect.w = layer->displayFrame.right - layer->displayFrame.left;

			ret = ioctl(fb_fd, K3FB_OVERLAY_SET, &overlay.info);
			if (ret < 0)
				ALOGE("K3FB_OVERLAY_SET returned error!");
		}
	}

	return 0;
}

static int hwc_set(struct hwc_composer_device_1 *dev, size_t numDisplays,
				hwc_display_contents_1_t **displays)
{
	hwc_context_1_t *context = (hwc_context_1_t *)dev;
	hwc_display_contents_1_t *display_content = NULL;
	int ret = 0, fb_fd = context->gralloc->framebuffer->fd;

	if (!numDisplays || !displays)
		return 0;

#ifdef DUMP_LAYERS
	/* dump frames with different amount of layers to try
	 * to find out which ones of them should be composed
	 */
	display_content = displays[HWC_DISPLAY_PRIMARY];
//	if (!(frame & (1 << display_content->numHwLayers))) {
		char fname[32];
		int fd;
		BITMAPFILEHEADER bmfh;
		BITMAPINFOHEADER bmih;

		frame |= 1 << display_content->numHwLayers;
		memset(&bmfh, 0, 14);
		bmfh.bfType = BF_TYPE;
		memset(&bmih, 0, 40);
		bmih.biSize = 40;
		bmih.biPlanes = 1;
		bmih.biBitCount = 32;

		for (size_t i = 0; i < display_content->numHwLayers; i++) {
			hwc_layer_1_t *layer = &display_content->hwLayers[i];
			if (layer->flags & HWC_SKIP_LAYER)
				continue;
			if (private_handle_t::validate(layer->handle) < 0)
				continue;
			const private_handle_t *handle = reinterpret_cast<const private_handle_t *>(layer->handle);
//if (handle->usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
//ALOGD("GRALLOC_USAGE_HW_VIDEO_ENCODER");
			bmih.biWidth = handle->width;
			bmih.biHeight = -handle->height;
			bmih.biSizeImage = handle->size;
			bmfh.bfSize = 14 + bmih.biSize + bmih.biSizeImage;
			bmfh.bfOffBits = 14 + bmih.biSize;
			if (layer->reserved[1]) {
				sprintf(fname, "/tmp/frame%u_%u_%s.bmp", display_content->numHwLayers, i, &layer->reserved[1]);
			}
			else
				sprintf(fname, "/tmp/frame%u_%u.bmp", display_content->numHwLayers, i);
			fd = open(fname, O_CREAT | O_WRONLY, (mode_t)0644);
			if (fd < 0)
				continue;
			write(fd, &bmfh, 14);
			write(fd, &bmih, 40);

			if (layer->acquireFenceFd >= 0) {
				if (sync_wait(layer->acquireFenceFd, 1000) < 0)
					ALOGE("sync_wait error: %d (%s)", errno, strerror(errno));
				close(layer->acquireFenceFd);
				layer->acquireFenceFd = -1;
			}
			write(fd, handle->base, handle->size);
			close(fd);
//}
		}
//	}
#endif
	for (size_t i = 0; i < numDisplays; i++) {
		display_content = displays[i];
		if (display_content) {
			hwc_layer_1_t *layer = &display_content->hwLayers[display_content->numHwLayers - 1];

			if (i == HWC_DISPLAY_PRIMARY) {
				if (!layer->handle)
					continue;
				if (private_handle_t::validate(layer->handle) < 0)
					continue;

				const private_handle_t *handle = reinterpret_cast<const private_handle_t *>(layer->handle);

				if (overlay.enabled) {
					if (context->blank)
						goto unset;

					overlay.data.src.phy_addr = context->gralloc->finfo.smem_start + handle->offset;
					overlay.data.src.blending = (layer->planeAlpha << 16) | layer->blending;
					overlay.data.src.compose_mode = OVC_FULL;
					overlay.data.src.bgr_fmt = EDC_BGR;
					overlay.data.src.actual_width = handle->width;
					overlay.data.src.actual_height = handle->height;
					overlay.data.src.width = handle->width;
					overlay.data.src.height = handle->height;
					overlay.data.src.stride = handle->width * (context->gralloc->info.bits_per_pixel / 8);
					overlay.data.is_graphic = 1;
/*
					switch (handle->format) {
					    case HAL_PIXEL_FORMAT_RGB_565:
						overlay.data.src.format = EDC_RGB_565;
						break;
					    case HAL_PIXEL_FORMAT_RGBX_8888:
						overlay.data.src.format = EDC_XRGB_8888;
						break;
					    case HAL_PIXEL_FORMAT_RGBA_8888:
					    default:
						overlay.data.src.format = EDC_ARGB_8888;
						break;
					}
*/
					overlay.data.src.format = EDC_ARGB_8888;

//					overlay.data.src.acquire_fence = layer->acquireFenceFd;
					if (layer->acquireFenceFd >= 0) {
						if (sync_wait(layer->acquireFenceFd, 1000) < 0)
							ALOGE("sync_wait error: %d (%s)", errno, strerror(errno));
						close(layer->acquireFenceFd);
					}

					ret = ioctl(fb_fd, K3FB_OVERLAY_PLAY, &overlay.data);
					if (ret < 0) {
						ALOGE("K3FB_OVERLAY_PLAY returned error!");
						dump_layer(layer);
						layer->releaseFenceFd = -1;
					}
					else
						layer->releaseFenceFd = overlay.data.src.release_fence;

					if (layer->releaseFenceFd >= 0)
						display_content->retireFenceFd = dup(layer->releaseFenceFd);
				}
			}
			else {
				/* deal with virtural display fence */
				if (i == HWC_DISPLAY_VIRTUAL) {
					layer->releaseFenceFd = layer->acquireFenceFd;
					display_content->retireFenceFd = display_content->outbufAcquireFenceFd;
				}
				else {
					ALOGE("display %d is not supported", i);
					return -EINVAL;
				}
			}
		}
	}
	return ret;

unset:
	hwc_unset(fb_fd, overlay.data.id);
	overlay.enabled = false;
	return 0;
}

static nsecs_t uevent_event(int fd) {
	char *c, msg[UEVENT_MSG_LEN + 2];
	int n;
	nsecs_t timestamp;

	while ((n = uevent_kernel_multicast_recv(fd, msg, UEVENT_MSG_LEN)) > 0) {
		if (n >= UEVENT_MSG_LEN)   /* overflow -- discard */
			return 0;

		msg[n] = '\0';
		msg[n+1] = '\0';
		c = msg;

		while (*c) {
			ALOGV("%s: %s", __func__, c);
			if (!strncmp(c, "VSYNC=", 6))
				return strtoll(c + 6, NULL, 10);

			/* advance to after the next \0 */
			while (*c++)
				;
		}
	}
	return 0;
}

static void *hwc_vsync_thread(void *data)
{
	struct hwc_context_1_t *context = (struct hwc_context_1_t*)data;
	struct sched_param param = {0};
	nsecs_t timestamp;
	int ret;

	prctl(PR_SET_NAME, "vsync_thread", 0, 0, 0);
//	androidSetThreadPriority(0, android::PRIORITY_URGENT_DISPLAY +
//				    android::PRIORITY_MORE_FAVORABLE);
	param.sched_priority = 2;
	ret = sched_setscheduler(0, SCHED_FIFO, &param);
	if (ret != 0)
		ALOGE("vsync_thread: failed to set priority");
	android_set_rt_ioprio(gettid(), 1);

	while (true) {
		pthread_mutex_lock(&hwc_mutex);
		while (!context->vsync_enabled)
			pthread_cond_wait(&hwc_cond, &hwc_mutex);
		pthread_mutex_unlock(&hwc_mutex);

		timestamp = uevent_event(context->uevent_fd);
		if (!timestamp) {
			timestamp = systemTime();
			ALOGE("vsync failed!");
		}
		context->procs->vsync(context->procs, HWC_DISPLAY_PRIMARY, timestamp);
		ALOGV("%s: timestamp = %lld", __func__, timestamp);
	}
}

static void hwc_registerProcs(hwc_composer_device_1 *dev,
				hwc_procs_t const* procs)
{
	struct hwc_context_1_t *context = (struct hwc_context_1_t*)dev;

	if (context)
		context->procs = procs;
}

static int hwc_getDisplayConfigs(hwc_composer_device_1 *dev,
		int disp ,uint32_t *config ,size_t *numConfigs)
{
	struct hwc_context_1_t *context = (struct hwc_context_1_t*)dev;

	if (*numConfigs == 0)
		return 0;

	if (disp == HWC_DISPLAY_PRIMARY) {
		config[0] = 0;
		*numConfigs = 1;
		return 0;
	}

	return -EINVAL;
}

static int hwc_getDisplayAttributes(hwc_composer_device_1 *dev, int disp, uint32_t config,
					const uint32_t *attributes, int32_t *values)
{
	struct hwc_context_1_t *context = (struct hwc_context_1_t*)dev;

	for (int i = 0; attributes[i] != HWC_DISPLAY_NO_ATTRIBUTE; i++) {
		switch(attributes[i]) {
		    case HWC_DISPLAY_VSYNC_PERIOD:
			values[i] = context->vsync_period;
			break;
		    case HWC_DISPLAY_WIDTH:
			values[i] = context->gralloc->info.xres;
			break;
		    case HWC_DISPLAY_HEIGHT:
			values[i] = context->gralloc->info.yres;
			break;
		    case HWC_DISPLAY_DPI_X:
			values[i] = context->gralloc->xdpi*1000.0;
			break;
		    case HWC_DISPLAY_DPI_Y:
			values[i] = context->gralloc->ydpi*1000.0;
			break;
		    default:
			values[i] = -EINVAL;
			break;
		}
	}

	return 0;
}

/* hwc 1.4
static int hwc_getActiveConfig(struct hwc_composer_device_1* dev, int disp)
{
	struct hwc_context_1_t *context = (struct hwc_context_1_t *)dev;

	//TODO:

    return 0;
}

static int hwc_setActiveConfig(struct hwc_composer_device_1* dev, int disp, int index)
{
	struct hwc_context_1_t *context = (struct hwc_context_1_t *)dev;

	//TODO:

	return 0;
}
static int hwc_setCursorPositionAsync(struct hwc_composer_device_1 *dev,
					int disp, int x_pos, int y_pos)
{
	//TODO:

	return 0;
}
*/

static int hwc_close(hw_device_t *dev)
{
	struct hwc_context_1_t *context = (struct hwc_context_1_t *)dev;

	pthread_kill(context->vsync_thread, SIGTERM);
	pthread_join(context->vsync_thread, NULL);

	if (context)
		free(context);

	return 0;
}

static int hwc_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device)
{
	int ret;

	if (strcmp(name, HWC_HARDWARE_COMPOSER))
		return -EINVAL;

	struct hwc_context_1_t *context;
	context = (struct hwc_context_1_t *)malloc(sizeof(*context));
	memset(context, 0, sizeof(*context));

	const struct private_module_t *gralloc_module;
	if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&gralloc_module)) {
		ALOGE("failed to get gralloc hw module");
		ret = -EINVAL;
		goto err;
	}

	context->gralloc = gralloc_module;
	context->vsync_period  = 16666667; //60HZ
	context->vsync_enabled = false;
	context->blank = 0;

	context->base.common.tag = HARDWARE_DEVICE_TAG;
	context->base.common.version = HWC_DEVICE_API_VERSION_1_3;
	context->base.common.module = const_cast<hw_module_t *>(module);
	context->base.common.close = hwc_close;

	context->base.prepare = hwc_prepare;
	context->base.set = hwc_set;
	context->base.eventControl = hwc_eventControl;
	context->base.query = hwc_query;
	context->base.registerProcs = hwc_registerProcs;
	context->base.dump = hwc_dump;
	context->base.getDisplayConfigs = hwc_getDisplayConfigs;
	context->base.getDisplayAttributes = hwc_getDisplayAttributes;
	context->base.blank = hwc_blank;
/* hwc 1.4
	context->base.setPowerMode = hwc_setPowerMode;
	context->base.getActiveConfig = hwc_getActiveConfig;
	context->base.setActiveConfig = hwc_setActiveConfig;
	context->base.setCursorPositionAsync = hwc_setCursorPositionAsync;
*/
	*device = &context->base.common;

	context->uevent_fd = uevent_open_socket(64 * 1024, true);
	if (context->uevent_fd < 0) {
		ALOGE("uevent_open_socket failed\n");
		ret = -1;
		goto err;
	}

	ret = pthread_create(&context->vsync_thread, NULL, hwc_vsync_thread, context);
	if (ret) {
		ALOGE("failed to start vsync thread: %d (%s)", ret, strerror(ret));
		ret = -ret;
		goto err;
	}

	return 0;

err:
	if (context)
		free(context);

	return ret;
}

static struct hw_module_methods_t hwc_module_methods = {
	.open = hwc_device_open
};

hwc_module_t HAL_MODULE_INFO_SYM = {
	.common = {
		.tag = HARDWARE_MODULE_TAG,
		.version_major = 1,
		.version_minor = 0,
		.id = HWC_HARDWARE_MODULE_ID,
		.name = "Hardware Composer HAL Module For hi6620oem Platform",
		.author = "Kostyan_nsk",
		.methods = &hwc_module_methods,
		.dso = NULL,
		.reserved = {0},
	}
};
