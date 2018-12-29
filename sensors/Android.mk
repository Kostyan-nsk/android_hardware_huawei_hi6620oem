# Copyright (C) 2015 STMicroelectronics
# Giuseppe Barba, Alberto Marinoni - Motion MEMS Product Div.
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := sensors.$(TARGET_BOARD_PLATFORM)

define def_macro
	$(foreach d,$1,$(addprefix -D,$(d)))
endef

define all-cpp-source-files
       $(patsubst ./%,%, \
               $(shell cd $(LOCAL_PATH); find . -name "*.cpp"))
endef

ENABLED_SENSORS := LSM330 \
		   APDS9900 \
		   AKM8963 \
		   AKM09911

ENABLED_MODULES := GBIAS \
		   GYROSCOPE_GBIAS_MODULE_PRESENT \
		   SENSOR_FUSION \
		   SENSOR_FUSION_MODULE_PRESENT

LOCAL_C_INCLUDES := $(LOCAL_PATH)/conf/ \
		    $(LOCAL_PATH)/lib/iNemoEngine_gBias/ \
		    $(LOCAL_PATH)/lib/iNemoEngine_SensorFusion/


LOCAL_STATIC_LIBRARIES := iNemoEngine_SensorFusion.$(TARGET_ARCH) \
			  iNemoEngine_gBias.$(TARGET_ARCH) \

LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\" \
		-DANDROID_VERSION=$(PLATFORM_SDK_VERSION) \
		$(call def_macro, $(ENABLED_SENSORS)) \
		$(call def_macro, $(ENABLED_MODULES)) \

ifeq ($(AKMD_DEVICE_TYPE), 8963)
LOCAL_CFLAGS += -DAKM_DEVICE_AK8963
endif
ifeq ($(AKMD_DEVICE_TYPE), 9911)
LOCAL_CFLAGS += -DAKM_DEVICE_AK09911
endif

LOCAL_SRC_FILES := $(call all-cpp-source-files)

LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libdl libc

include $(BUILD_SHARED_LIBRARY)
