LOCAL_PATH := $(call my-dir)

#
# Build video encoder
#

include $(CLEAR_VARS)

OMX_CORE = $(LOCAL_PATH)/../omx_core
BELLAGIO_INCLUDE = $(OMX_CORE)/include

ENCODER_API_VERSION = 8290

ENCODER_RELEASE = $(LOCAL_PATH)/../../encoder

WRITE_OUTPUT_BUFFERS = false
#WRITE_OUTPUT_BUFFERS = true

EXTRA_FLAGS = -Wno-unused-parameter

ifeq ($(WRITE_OUTPUT_BUFFERS),true)
	EXTRA_FLAGS += -DDUMP_OUTPUT_BUFFER \
		       -DOUTPUT_BUFFER_PREFIX=\"/tmp/output-\"
endif



LOCAL_CFLAGS := -DOMX_ENCODER_VIDEO_DOMAIN \
                -DENC$(ENCODER_API_VERSION) \
                -DANDROID_MOD \
		-DANDROID \
		-DOMX_OSAL_TRACE \
		-DMEMALLOCHW \
		$(EXTRA_FLAGS)
#		-DALWAYS_CODE_INTRA_FRAMES

# uncomment below to use temporary output buffer
#LOCAL_CFLAGS += -DUSE_TEMP_OUTPUT_BUFFER

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
		    $(TOP)/frameworks/native/include/media/openmax \
		    $(ENCODER_RELEASE)/inc \
		    $(BELLAGIO_INCLUDE)


LOCAL_SHARED_LIBRARIES := \
		liblog \
		libhardware
#		libutils  \
#		libcutils \
#		libdl     \
#		libc

# source files:
base_SRCS = msgque.c OSAL.c basecomp.c port.c util.c
libhantrovideoenc_SRCS = $(base_SRCS) \
			 8290_encoder/encoder_constructor_video.c \
			 8290_encoder/encoder_h264.c \
			 8290_encoder/encoder.c \
			 8290_encoder/library_entry_point.c
#			 8290_encoder/encoder_mpeg4.c \
#			 8290_encoder/encoder_h263.c \

LOCAL_SRC_FILES := $(libhantrovideoenc_SRCS)

# static libraries:
#LOCAL_STATIC_LIBRARIES := libx280h264 \
#			  libx280ewl \
#			  libx280common
#			  libx280mpeg4

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_LDFLAGS := -L$(TOP)/vendor/huawei/hi6620oem-common/proprietary/lib -l_8290

LOCAL_MODULE := libhantrovideoenc
LOCAL_PRELINK_MODULE:=false
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

#
# Build image decoder
#

include $(CLEAR_VARS)

OMX_CORE = $(LOCAL_PATH)/../omx_core
BELLAGIO_INCLUDE = $(OMX_CORE)/include

ENCODER_API_VERSION = 8290

ENCODER_RELEASE = $(LOCAL_PATH)/../../encoder

LOCAL_CFLAGS := -DOMX_ENCODER_IMAGE_DOMAIN \
		-DENC$(ENCODER_API_VERSION) \
		-DOMX_OSAL_TRACE \
		-DANDROID_MOD \
		-DANDROID

LOCAL_CFLAGS += -Wno-unused-parameter

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
		    $(TOP)/frameworks/native/include/media/openmax \
		    $(ENCODER_RELEASE)/inc \
		    $(BELLAGIO_INCLUDE)

LOCAL_SHARED_LIBRARIES := \
		libutils  \
		libcutils \
		libdl     \
		libc

# source files:
base_SRCS = msgque.c OSAL.c basecomp.c port.c util.c
libhantrovideoenc_SRCS = $(base_SRCS) \
			 8290_encoder/encoder_constructor_image.c \
			 8290_encoder/encoder_jpeg.c \
			 8290_encoder/encoder.c \
			 8290_encoder/library_entry_point.c

LOCAL_SRC_FILES := $(libhantrovideoenc_SRCS)

# static libraries:
LOCAL_STATIC_LIBRARIES := libx280jpeg \
			  libx280ewl \
			  libx280common

LOCAL_MODULE := libhantroimageenc
LOCAL_PRELINK_MODULE:=false
LOCAL_MODULE_TAGS := optional

#include $(BUILD_SHARED_LIBRARY)
