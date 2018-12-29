LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE:= gpsdaemon
LOCAL_SRC_FILES:= gpsdaemon.c
LOCAL_SHARED_LIBRARIES := libc liblog libcutils
include $(BUILD_EXECUTABLE)