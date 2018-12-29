LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/..

LOCAL_SRC_FILES := mpeg2decapi.c \
		   mpeg2decapi_internal.c \
		   mpeg2hwd_headers.c \
		   mpeg2hwd_strm.c \
		   mpeg2hwd_utils.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/ \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/config \
		    $(common_src_dir)/common \
		    $(common_src_dir)/dwl
LOCAL_CFLAGS := -DANDROID_MOD
#		-DDEC_X170_OUTPUT_FORMAT=DEC_X170_OUTPUT_FORMAT_RASTER_SCAN
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libx170mpeg2
include $(BUILD_STATIC_LIBRARY)

