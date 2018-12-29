LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
common_src_dir := $(LOCAL_PATH)/..
LOCAL_SRC_FILES := avsdecapi.c \
		   avsdecapi_internal.c \
		   avs_headers.c \
		   avs_strm.c \
		   avs_utils.c \
		   avs_vlc.c
LOCAL_C_INCLUDES := . \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/config \
		    $(common_src_dir)/common \
		    $(common_src_dir)/dwl

LOCAL_CFLAGS := -DANDROID_MOD
#		-DDEC_X170_OUTPUT_FORMAT=DEC_X170_OUTPUT_FORMAT_RASTER_SCAN

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libdecx170avs
include $(BUILD_STATIC_LIBRARY)

