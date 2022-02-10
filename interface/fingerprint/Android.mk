LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(BBAT_BIT64), true)
LOCAL_32_BIT_ONLY := false
else
LOCAL_32_BIT_ONLY := true
endif

LOCAL_SRC_FILES := fingerprint_sensor.cpp

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := autotestfinger
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH := npidevice


LOCAL_C_INCLUDES:= \
    $(TOP)/vendor/sprd/proprietories-source/engpc/sprd_fts_inc \
    hardware/libhardware/include \
    hardware/libhardware_legacy/include

LOCAL_SHARED_LIBRARIES:= \
    liblog \
    libcutils  \
    libhardware
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

ifeq ($(BBAT_BIT64), true)
LOCAL_32_BIT_ONLY := false
else
LOCAL_32_BIT_ONLY := true
endif

LOCAL_SRC_FILES := \
           eng_attok.cpp \
           fingerprint_sensor_mmi.cpp

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := nativemmifinger
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH := npidevice


LOCAL_C_INCLUDES:= \
    $(TOP)/vendor/sprd/proprietories-source/engpc/sprd_fts_inc \
    hardware/libhardware/include \
    hardware/libhardware_legacy/include

LOCAL_SHARED_LIBRARIES:= \
    liblog \
    libcutils  \
    libhardware  \


include $(BUILD_SHARED_LIBRARY)
