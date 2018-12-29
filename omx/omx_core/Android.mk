LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

BELLAGIO_INCLUDE = $(LOCAL_PATH)/include
OMXIL_HEADERS = $(TOP)/frameworks/native/include/media/openmax

LOCAL_SRC_FILES := \
	OMX_Core.c \
	st_static_component_loader.c

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH) \
	$(BELLAGIO_INCLUDE) \
	$(OMXIL_HEADERS)

LOCAL_CFLAGS := -Wno-unused-parameter

LOCAL_SHARED_LIBRARIES := \
	libdl \
	libutils \
	liblog

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libhantro_omx_core
LOCAL_PRELINK_MODULE:=false

include $(BUILD_SHARED_LIBRARY)

