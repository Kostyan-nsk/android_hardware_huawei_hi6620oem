/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
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

#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <sys/ioctl.h>

#include "alloc_device.h"
#include "gralloc_priv.h"
#include "gralloc_helper.h"
#include "framebuffer_device.h"

#include <linux/ion.h>
#include <ion/ion.h>
#include <backtrace.h>

#define GRALLOC_ALIGN( value, base ) (((value) + ((base) - 1)) & ~((base) - 1))

static int gralloc_alloc_buffer(alloc_device_t *dev, size_t size, int usage, buffer_handle_t *pHandle)
{
	private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
	ion_user_handle_t ion_hnd;
	unsigned char *cpu_ptr;
	int shared_fd;
	int ret;
	unsigned int ion_flags = 0;
	struct ion_custom_data custom_data;
	struct ion_phys_data phys_data;

	if ((usage & GRALLOC_USAGE_SW_READ_MASK) == GRALLOC_USAGE_SW_READ_OFTEN)
		ion_flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
//	ret = ion_alloc(m->ion_client, size, 0, ION_HEAP_SYSTEM_MASK, 0, &(ion_hnd));
	ret = ion_alloc(m->ion_client, size, 0, ION_HEAP(HISI_ION_HEAP_GRALLOC_ID), ion_flags, &ion_hnd);



	if (ret != 0)
	{
		AERR("Failed to ion_alloc from ion_client:%d", m->ion_client);
		return -1;
	}

	ret = ion_share(m->ion_client, ion_hnd, &shared_fd);

	if (ret != 0)
	{
		AERR("ion_share( %d ) failed", m->ion_client);

		if (0 != ion_free(m->ion_client, ion_hnd))
		{
			AERR("ion_free( %d ) failed", m->ion_client);
		}

		return -1;
	}

//	if (usage & (GRALLOC_USAGE_PRIVATE_CAMERA | GRALLOC_USAGE_HW_ENCODER)) {
		phys_data.fd = shared_fd;
		custom_data.cmd = ION_HISI_CUSTOM_PHYS;
		custom_data.arg = (uintptr_t)&phys_data;
		ret = ioctl(m->ion_client, ION_IOC_CUSTOM, &custom_data);
		if (ret != 0) {
			ALOGE("gralloc_alloc_buffer: failed to get physical address!");
			phys_data.phys = 0;
		}
//	}
//	else
//		phys_data.phys = 0;

	cpu_ptr = (unsigned char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);

	if (cpu_ptr == MAP_FAILED)
	{
		AERR("ion_map( %d ) failed", m->ion_client);

		if (0 != ion_free(m->ion_client, ion_hnd))
		{
			AERR("ion_free( %d ) failed", m->ion_client);
		}
			close(shared_fd);
		return -1;
	}

	private_handle_t *hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_USES_ION, usage, size, cpu_ptr, private_handle_t::LOCK_STATE_MAPPED);

	if (hnd != NULL)
	{
		hnd->share_fd = shared_fd;
		hnd->ion_hnd = ion_hnd;
		hnd->phys_addr = phys_data.phys;
		if (usage & GRALLOC_USAGE_PRIVATE_CAMERA)
			hnd->fd = m->ion_client;

//		ALOGD("gralloc_alloc_buffer: handle = %p size = %u phy = 0x%x vir = %p ion_hnd = %d, ion_client = %d", hnd, size, hnd->phys_addr, hnd->base, hnd->ion_hnd, m->ion_client);
		*pHandle = hnd;
		return 0;
	}
	else
	{
		AERR("Gralloc out of mem for ion_client:%d", m->ion_client);
	}

	close(shared_fd);
	ret = munmap(cpu_ptr, size);

	if (0 != ret)
	{
		AERR("munmap failed for base:%p size: %lu", cpu_ptr, (unsigned long)size);
	}

	ret = ion_free(m->ion_client, ion_hnd);

	if (0 != ret)
	{
		AERR("ion_free( %d ) failed", m->ion_client);
	}

	return -1;
}

static int gralloc_alloc_framebuffer_locked(alloc_device_t *dev, size_t size, int usage, buffer_handle_t *pHandle)
{
	private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);

	// allocate the framebuffer
	if (m->framebuffer == NULL)
	{
		// initialize the framebuffer, the framebuffer is mapped once and forever.
		int err = init_frame_buffer_locked(m);

		if (err < 0)
		{
			return err;
		}
	}

	const uint32_t bufferMask = m->bufferMask;
	const uint32_t numBuffers = m->numBuffers;
	const size_t bufferSize = m->finfo.line_length * m->info.yres;

	if (numBuffers == 1)
	{
		// If we have only one buffer, we never use page-flipping. Instead,
		// we return a regular buffer which will be memcpy'ed to the main
		// screen when post is called.
		int newUsage = (usage & ~GRALLOC_USAGE_HW_FB) | GRALLOC_USAGE_HW_2D;
		AERR("fallback to single buffering. Virtual Y-res too small %d", m->info.yres);
		return gralloc_alloc_buffer(dev, bufferSize, newUsage, pHandle);
	}

	if (bufferMask >= ((1LU << numBuffers) - 1))
	{
		// We ran out of buffers.
		ALOGE(" We ran out of buffers. BufferMask = %u numBuffers = %u", bufferMask, numBuffers);
		return -ENOMEM;
	}

	void *vaddr = m->framebuffer->base;

	// find a free slot
	for (uint32_t i = 0 ; i < numBuffers ; i++)
	{
		if ((bufferMask & (1LU << i)) == 0)
		{
			m->bufferMask |= (1LU << i);
			break;
		}

		vaddr = (void *)((uintptr_t)vaddr + bufferSize);
	}

//	ALOGD("allocate buffer %p bufferSize %d", vaddr, bufferSize);

	// The entire framebuffer memory is already mapped, now create a buffer object for parts of this memory
	private_handle_t *hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_FRAMEBUFFER, usage, size, vaddr,
				    0, m->framebuffer->fd, (uintptr_t)vaddr - (uintptr_t) m->framebuffer->base, 0);

#ifdef FBIOGET_DMABUF
		struct fb_dmabuf_export fb_dma_buf;

		if (ioctl(m->framebuffer->fd, FBIOGET_DMABUF, &fb_dma_buf) == 0)
		{
			AINF("framebuffer accessed with dma buf (fd 0x%x)\n", (int)fb_dma_buf.fd);
			hnd->share_fd = fb_dma_buf.fd;
		}

#endif

	*pHandle = hnd;

	return 0;
}

static int gralloc_alloc_framebuffer(alloc_device_t *dev, size_t size, int usage, buffer_handle_t *pHandle)
{
	private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
	pthread_mutex_lock(&m->lock);
	int err = gralloc_alloc_framebuffer_locked(dev, size, usage, pHandle);
	pthread_mutex_unlock(&m->lock);
	return err;
}

static int alloc_device_alloc(alloc_device_t *dev, int w, int h, int format, int usage, buffer_handle_t *pHandle, int *pStride)
{
	if (!pHandle || !pStride)
	{
		return -EINVAL;
	}
/*
	ALOGD("alloc_device_alloc: width %u = height = %u format = 0x%x usage = 0x%08x", w, h, format, usage);
	if (usage == 0x30000033)
		backtrace();
*/
	size_t size;
	size_t stride;

	if (format == HAL_PIXEL_FORMAT_YCrCb_420_SP || format == HAL_PIXEL_FORMAT_YCbCr_420_SP ||
	    format == HAL_PIXEL_FORMAT_BLOB)
	{
		switch (format)
		{
			case HAL_PIXEL_FORMAT_YCrCb_420_SP:
			case HAL_PIXEL_FORMAT_YCbCr_420_SP:
				stride = GRALLOC_ALIGN(w, 16);
				size = GRALLOC_ALIGN(h, 16) * (stride + GRALLOC_ALIGN(stride / 2, 16));
				break;
			case HAL_PIXEL_FORMAT_BLOB:
				stride = GRALLOC_ALIGN(w, 16);
				size = GRALLOC_ALIGN(h, 2) * (stride + GRALLOC_ALIGN(stride / 2, 16));
				break;
/*
			case OMX_COLOR_FormatYCbYCr:
				stride = GRALLOC_ALIGN(w, 16);
				size = h * stride * 2;
				break;
			case HAL_PIXEL_FORMAT_BLOB:
				stride = w;
				size = w * h;
				break;
*/
			default:
				return -EINVAL;
		}
	}
	else
	{
		int bpp = 0;

		switch (format)
		{
			case HAL_PIXEL_FORMAT_RGBA_8888:
			case HAL_PIXEL_FORMAT_RGBX_8888:
			case HAL_PIXEL_FORMAT_BGRA_8888:
				bpp = 4;
				break;

			case HAL_PIXEL_FORMAT_RGB_888:
				bpp = 3;
				break;

			case HAL_PIXEL_FORMAT_RGB_565:
			case HAL_PIXEL_FORMAT_RAW16:
#if PLATFORM_SDK_VERSION < 19
			case HAL_PIXEL_FORMAT_RGBA_5551:
			case HAL_PIXEL_FORMAT_RGBA_4444:
#endif
				bpp = 2;
				break;
			case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
				if (usage & (GRALLOC_USAGE_HW_VIDEO_ENCODER | GRALLOC_USAGE_HW_COMPOSER)) {
					bpp = 4;
					break;
				}
			default:
				return -EINVAL;
		}

		size_t bpr;
		/* Align the lines to 64 bytes.
		 * It's more efficient to write to 64-byte aligned addresses because it's the burst size on the bus.
		 * But in case of video decoder, buffer size have to match frame size for not messing up picture.
		 */
		if (usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
			bpr = GRALLOC_ALIGN(w, 2) * bpp;
		else
//			bpr = GRALLOC_ALIGN(w * bpp, 64);
			bpr = GRALLOC_ALIGN(w * bpp, 32);
		size = bpr * h;
		stride = bpr / bpp;

	}

	int err;

	if (usage & GRALLOC_USAGE_HW_FB)
	{
		err = gralloc_alloc_framebuffer(dev, size, usage, pHandle);
	}
	else
	{
		err = gralloc_alloc_buffer(dev, size, usage, pHandle);
	}

	if (err < 0)
	{
		ALOGE("alloc failed: %d", err);
		return err;
	}

	/* match the framebuffer format */
	if (usage & GRALLOC_USAGE_HW_FB)
	{
#ifdef GRALLOC_16_BITS
		format = HAL_PIXEL_FORMAT_RGB_565;
#else
		format = HAL_PIXEL_FORMAT_RGBA_8888;
#endif
	}

	private_handle_t *hnd = (private_handle_t *)*pHandle;
	int private_usage = usage & (GRALLOC_USAGE_PRIVATE_0 |
		                      GRALLOC_USAGE_PRIVATE_1);

	switch (private_usage)
	{
		case 0:
			hnd->yuv_info = MALI_YUV_BT601_NARROW;
			break;

		case GRALLOC_USAGE_PRIVATE_1:
			hnd->yuv_info = MALI_YUV_BT601_WIDE;
			break;

		case GRALLOC_USAGE_PRIVATE_0:
			hnd->yuv_info = MALI_YUV_BT709_NARROW;
			break;

		case (GRALLOC_USAGE_PRIVATE_0 | GRALLOC_USAGE_PRIVATE_1):
			hnd->yuv_info = MALI_YUV_BT709_WIDE;
			break;
	}

	hnd->width = w;
	hnd->height = h;
	hnd->format = format;
	hnd->stride = stride;

	*pStride = stride;
	return 0;
}

static int alloc_device_free(alloc_device_t *dev, buffer_handle_t handle)
{
	int ion_client;

	if (private_handle_t::validate(handle) < 0)
	{
		return -EINVAL;
	}

	private_handle_t const *hnd = reinterpret_cast<private_handle_t const *>(handle);

	if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
	{
		// free this buffer
		private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
		const size_t bufferSize = m->finfo.line_length * m->info.yres;
		int index = ((uintptr_t)hnd->base - (uintptr_t)m->framebuffer->base) / bufferSize;
		m->bufferMask &= ~(1 << index);
		close(hnd->fd);
	}
	else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP)
	{
		AERR("Can't free ump memory for handle:0x%p. Not supported.", hnd);
	}
	else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION)
	{
		private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
//		ALOGD("ion_free(handle = %p ion_client: %d ion_hnd: %p )", hnd, m->ion_client, (void *)(uintptr_t)hnd->ion_hnd);
		/* Buffer might be unregistered so we need to check for invalid ump handle*/
		if (0 != hnd->base)
		{
			if (0 != munmap((void *)hnd->base, hnd->size))
			{
				AERR("Failed to munmap handle 0x%p", hnd);
			}
		}

		close(hnd->share_fd);

		if (hnd->usage & GRALLOC_USAGE_PRIVATE_CAMERA)
			ion_client = hnd->fd;
		else
			ion_client = m->ion_client;
/*
		if ((hnd->usage & GRALLOC_USAGE_PRIVATE_CAMERA) && m->ion_client != hnd->fd) {
			m->ion_client = hnd->fd;
			ALOGD("alloc_device_free: restoring ion_client from private handle's fd = %d", hnd->fd);
		}
*/
//		if (0 != ion_free(m->ion_client, hnd->ion_hnd))
		if (0 != ion_free(ion_client, hnd->ion_hnd))
		{
//			AERR("Failed to ion_free( ion_client: %d ion_hnd: %p )", m->ion_client, (void *)(uintptr_t)hnd->ion_hnd);
			AERR("Failed to ion_free(handle = %p ion_client: %d ion_hnd: %p )", hnd, m->ion_client, (void *)(uintptr_t)hnd->ion_hnd);
			backtrace();
		}

		memset((void *)hnd, 0, sizeof(*hnd));
	}

	delete hnd;

	return 0;
}

static int alloc_device_close(struct hw_device_t *device)
{
	alloc_device_t *dev = reinterpret_cast<alloc_device_t *>(device);

	if (dev)
	{
		private_module_t *m = reinterpret_cast<private_module_t *>(device);

		if (0 != ion_close(m->ion_client))
		{
			AERR("Failed to close ion_client: %d", m->ion_client);
			backtrace();
		}

		delete dev;
	}

	return 0;
}

int alloc_device_open(hw_module_t const *module, const char *name, hw_device_t **device)
{
	MALI_IGNORE(name);
	alloc_device_t *dev;

	dev = new alloc_device_t;

	if (NULL == dev)
	{
		return -1;
	}

	/* initialize our state here */
	memset(dev, 0, sizeof(*dev));

	/* initialize the procs */
	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = const_cast<hw_module_t *>(module);
	dev->common.close = alloc_device_close;
	dev->alloc = alloc_device_alloc;
	dev->free = alloc_device_free;

	private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
	m->ion_client = ion_open();

	if (m->ion_client < 0)
	{
		AERR("ion_open failed with %s", strerror(errno));
		delete dev;
		return -1;
	}

	*device = &dev->common;

	return 0;
}
