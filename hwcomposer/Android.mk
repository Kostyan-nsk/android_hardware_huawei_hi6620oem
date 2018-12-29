LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SHARED_LIBRARIES := liblog libutils libcutils libhardware libsync
LOCAL_CFLAGS := -Wno-unused-parameter
LOCAL_SRC_FILES := hwcomposer.cpp

LOCAL_C_INCLUDES := \
	system/core/libsync/include \
	hardware/huawei/$(TARGET_BOARD_PLATFORM)/gralloc

LOCAL_MODULE := hwcomposer.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
