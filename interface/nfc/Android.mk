LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(BBAT_BIT64), true)
LOCAL_32_BIT_ONLY := false
else
LOCAL_32_BIT_ONLY := true
endif

LOCAL_SRC_FILES := nfc.cpp 

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := autotestnfc
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