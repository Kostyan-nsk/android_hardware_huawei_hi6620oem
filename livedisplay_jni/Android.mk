LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := liblivedisplay_jni
LOCAL_SRC_FILES := livedisplay.cpp
LOCAL_CFLAGS := -Wno-unused-parameter
LOCAL_SHARED_LIBRARIES := liblog libnativehelper
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)