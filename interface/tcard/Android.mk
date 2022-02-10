LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := tcardtest.cpp \
		   bbattest.cpp

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := autotesttcard
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH := npidevice

LOCAL_C_INCLUDES:= \
    vendor/sprd/proprietories-source/engpc/sprd_fts_inc \
    vendor/sprd/proprietories-source/autotest/interface/include

LOCAL_SHARED_LIBRARIES:= \
    libcutils \
    liblog 

include $(BUILD_SHARED_LIBRARY)
