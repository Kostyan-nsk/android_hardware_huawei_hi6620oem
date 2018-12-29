LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE:= mac_addr
LOCAL_SRC_FILES:= mac_addr.c
LOCAL_SHARED_LIBRARIES := libc liblog
include $(BUILD_EXECUTABLE)