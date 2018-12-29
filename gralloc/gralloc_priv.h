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

#ifndef GRALLOC_PRIV_H_
#define GRALLOC_PRIV_H_

#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <unistd.h>
#include <log/log.h>
#include <sys/mman.h>

#include <hardware/gralloc.h>
#include <cutils/native_handle.h>
#include "alloc_device.h"

#include <linux/ion.h>

#define GRALLOC_USAGE_PRIVATE_CAMERA 0x30000000

enum HISI_ION_HEAP_ID {
    HISI_ION_HEAP_CAMERA_ID = 1,
    HISI_ION_HEAP_GRALLOC_ID,
    HISI_ION_HEAP_OVERLAY_ID,
    HISI_ION_HEAP_SYS_ID,
    HISI_ION_HEAP_SYS_CONTIG_ID,
    HISI_ION_HEAP_DMA_ID,
};

//struct used for get phys addr of contig heap
struct ion_phys_data {
	int fd;
	uint32_t phys;
	size_t size;
};

//user command add for additional use
enum ION_HISI_CUSTOM_CMD {
    ION_HISI_CUSTOM_MSYNC,
    ION_HISI_CUSTOM_PHYS,
    ION_HISI_CUSTOM_FLUSH_CACHE
};

#define ION_HEAP(bit) (1 << (bit))

#define GRALLOC_ARM_DMA_BUF_MODULE 1

/* NOTE:
 * If your framebuffer device driver is integrated with dma_buf, you will have to
 * change this IOCTL definition to reflect your integration with the framebuffer
 * device.
 * Expected return value is a structure filled with a file descriptor
 * backing your framebuffer device memory.
 */
struct fb_dmabuf_export
{
	__u32 fd;
	__u32 flags;
};
/*#define FBIOGET_DMABUF    _IOR('F', 0x21, struct fb_dmabuf_export)*/


/* the max string size of GRALLOC_HARDWARE_GPU0 & GRALLOC_HARDWARE_FB0
 * 8 is big enough for "gpu0" & "fb0" currently
 */
#define MALI_GRALLOC_HARDWARE_MAX_STR_LEN 8
#define GRALLOC_ARM_NUM_FDS 1
#define ION_INVALID_HANDLE 0
#define NUM_INTS_IN_PRIVATE_HANDLE ((sizeof(struct private_handle_t) - sizeof(native_handle)) / sizeof(int) - sNumFds)
#define MALI_IGNORE(x) (void)x

typedef enum
{
	MALI_YUV_NO_INFO,
	MALI_YUV_BT601_NARROW,
	MALI_YUV_BT601_WIDE,
	MALI_YUV_BT709_NARROW,
	MALI_YUV_BT709_WIDE,
} mali_gralloc_yuv_info;

struct private_handle_t;

struct private_module_t
{
	gralloc_module_t base;

	struct private_handle_t *framebuffer;
	uint32_t flags;
	uint32_t numBuffers;
	uint32_t bufferMask;
	pthread_mutex_t lock;
	buffer_handle_t currentBuffer;
//	uint32_t padding;
	int ion_client;

	struct fb_var_screeninfo info;
	struct fb_fix_screeninfo finfo;
	float xdpi;
	float ydpi;
	float fps;

	enum
	{
		// flag to indicate we'll post this buffer
		PRIV_USAGE_LOCKED_FOR_POST = 0x80000000
	};
#ifdef __cplusplus
	/* default constructor */
	private_module_t();
#endif
};


/*
ATTENTION: don't add member in this struct, it is used by other modules.
*/
#ifdef __cplusplus
struct private_handle_t : public native_handle
{
#else
struct private_handle_t
{
	struct native_handle nativeHandle;
#endif

	enum
	{
		PRIV_FLAGS_FRAMEBUFFER   = 0x00000001,
		PRIV_FLAGS_USES_UMP      = 0x00000002,
		PRIV_FLAGS_USES_ION      = 0x00000004,
		PRIV_FLAGS_VIDEO_OVERLAY = 0x00000010,
		PRIV_FLAGS_VIDEO_OMX     = 0x00000020,
		PRIV_FLAGS_CURSOR        = 0x00000040,
		PRIV_FLAGS_OSD_VIDEO_OMX = 0x00000080,
	};

	enum
	{
		LOCK_STATE_WRITE        = 1 << 31,
		LOCK_STATE_MAPPED       = 1 << 30,
		LOCK_STATE_UNREGISTERED = 1 << 29,
		LOCK_STATE_READ_MASK    = 0x1FFFFFFF
	};

	/*
	 * Shared file descriptor for dma_buf sharing. This must be the first element in the
	 * structure so that binder knows where it is and can properly share it between
	 * processes.
	 * DO NOT MOVE THIS ELEMENT!
	 */
	int        share_fd; //12

	int        share_attr_fd;

	ion_user_handle_t ion_hnd;

	// ints
	int        magic;
	int        req_format;
	uint64_t   internal_format;
	int        byte_stride;
	int        flags;
	int        usage;
	int        size; //52
	int        width;
	int        height;
	int        format;
	int        internalWidth;
	int        internalHeight;
	int        stride;
//	union {
		void*    base;
		uint32_t phys_addr; //84
//		uint64_t padding;
//	};
	int        lockState;
	int        writeOwner;
	int        pid;

	// locally mapped shared attribute area
	union {
		void*    attr_base;
		uint64_t padding3;
	};

	mali_gralloc_yuv_info yuv_info;

    // Following members is for framebuffer only
	int        fd; //to mmap osd memory
    //current buffer offset from framebuffer base
	union {
		off_t    offset;
		uint64_t padding4;
	};

	uint64_t padding_1;
	uint64_t padding_2;
	uint64_t padding_3;
	uint64_t padding_4;

#ifdef __cplusplus
	/*
	 * We track the number of integers in the structure. There are 16 unconditional
	 * integers (magic - pid, yuv_info, fd and offset). Note that the fd element is
	 * considered an int not an fd because it is not intended to be used outside the
	 * surface flinger process. The GRALLOC_ARM_NUM_INTS variable is used to track the
	 * number of integers that are conditionally included. Similar considerations apply
	 * to the number of fds.
	 */
	static const int sNumFds = GRALLOC_ARM_NUM_FDS;
	static const int sMagic = 0x3141592;

	private_handle_t(int flags, int usage, int size, void *base, int lock_state):
		share_fd(-1),
		share_attr_fd(-1),
		ion_hnd(ION_INVALID_HANDLE),
		magic(sMagic),
		flags(flags),
		usage(usage),
		size(size),
		width(0),
		height(0),
		format(0),
		stride(0),
		base(base),
		lockState(lock_state),
		writeOwner(0),
		pid(getpid()),
		attr_base(MAP_FAILED),
		yuv_info(MALI_YUV_NO_INFO),
		fd(0),
		offset(0)

	{
		version = sizeof(native_handle);
		numFds = sNumFds;
		numInts = NUM_INTS_IN_PRIVATE_HANDLE;
	}

	private_handle_t(int flags, int usage, int size, void *base, int lock_state, int fb_file, off_t fb_offset, int unused):
		share_fd(-1),
		share_attr_fd(-1),
		ion_hnd(ION_INVALID_HANDLE),
		magic(sMagic),
		flags(flags),
		usage(usage),
		size(size),
		width(0),
		height(0),
		format(0),
		stride(0),
		base(base),
		lockState(lock_state),
		writeOwner(0),
		pid(getpid()),
		attr_base(MAP_FAILED),
		yuv_info(MALI_YUV_NO_INFO),
		fd(fb_file),
		offset(fb_offset)

	{
		version = sizeof(native_handle);
		numFds = sNumFds;
		numInts = NUM_INTS_IN_PRIVATE_HANDLE;
        /*
         * because -Werror option, need to remove this warning;
         */
        (void)unused;
	}

	~private_handle_t()
	{
		magic = 0;
	}

	bool usesPhysicallyContiguousMemory()
	{
		return (flags & PRIV_FLAGS_FRAMEBUFFER) ? true : false;
	}

	static int validate(const native_handle *h)
	{
		const private_handle_t *hnd = (const private_handle_t *)h;

		if (!h ||
		    h->version != sizeof(native_handle) ||
		    h->numInts != NUM_INTS_IN_PRIVATE_HANDLE ||
		    h->numFds != sNumFds ||
		    hnd->magic != sMagic)
		{
		ALOGE("validate: h->version = %u sizeof(native_handle) = %u h->numInts = %u sNumInts = %u h->numFds = %u sNumFds = %u hnd->magic = %u sMagic = %u",
			h->version, sizeof(native_handle), h->numInts, NUM_INTS_IN_PRIVATE_HANDLE, h->numFds, sNumFds, hnd->magic, sMagic);
			return -EINVAL;
		}

		return 0;
	}

	static private_handle_t *dynamicCast(const native_handle *in)
	{
		if (validate(in) == 0)
		{
			return (private_handle_t *) in;
		}

		return NULL;
	}
#endif
};

#endif /* GRALLOC_PRIV_H_ */
