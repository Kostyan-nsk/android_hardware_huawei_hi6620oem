LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#LOCKING := ioctl
LOCKING := normal

common_src_dir := $(LOCAL_PATH)/..
LOCAL_C_INCLUDES := . \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/common
LOCAL_SRC_FILES := dwl_linux.c \
	           dwl_x170_linux_irq.c
ifeq ($(LOCKING), ioctl)
  LOCAL_CFLAGS += -DUSE_LINUX_LOCK_IOCTL
  LOCAL_SRC_FILES += dwl_linux_lock_ioctl.c
else
  LOCAL_SRC_FILES += dwl_linux_lock.c
endif
LOCAL_CFLAGS := -DDEC_MODULE_PATH=\"/dev/hx170dec\" \
		-DMEMALLOC_MODULE_PATH=\"/dev/memalloc\" \
		-DANDROID_MOD \
		-DMEMORY_USAGE_TRACE \
		-Wno-unused-parameter
#		-D_DWL_DEBUG
#		-DSUPPORT_ZERO_COPY

#LOCAL_CFLAGS += -D_DWL_HW_PERFORMANCE

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libdwlx170
include $(BUILD_STATIC_LIBRARY)

