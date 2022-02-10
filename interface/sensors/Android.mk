LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := get_sensor_info.c

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := autotestsensorinfo
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH := npidevice

LOCAL_C_INCLUDES:= \
    $(TOP)/vendor/sprd/proprietories-source/engpc/sprd_fts_inc \
    hardware/libhardware/include \
    vendor/sprd/proprietories-source/autotest/interface/include \

LOCAL_SHARED_LIBRARIES:= \
    libcutils \
    liblog \
    libhardware

include $(BUILD_SHARED_LIBRARY)
