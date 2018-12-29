LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/..

LOCAL_SRC_FILES :=	vp8decapi.c \
			vp8hwd_asic.c \
			vp8hwd_bool.c \
			vp8hwd_decoder.c \
			vp8hwd_headers.c \
			vp8hwd_pp_pipeline.c \
			vp8hwd_probs.c \
			vp8hwd_error.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/ \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/config \
		    $(common_src_dir)/common \
		    $(common_src_dir)/dwl

LOCAL_CFLAGS := -DANDROID_MOD
#-DDEC_X170_OUTPUT_FORMAT=DEC_X170_OUTPUT_FORMAT_RASTER_SCAN

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libx170vp8

include $(BUILD_STATIC_LIBRARY)

