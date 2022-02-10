LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := \
	lkv_npi.c \
	light.cpp \
	debug.cpp \
	vibrator.cpp \
	power.cpp \
	tp.cpp

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := liblkvnpi
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH := npidevice


LOCAL_C_INCLUDES += vendor/sprd/proprietories-source/autotest/minui \
	$(TOP)/vendor/sprd/proprietories-source/autotest/interface/include \
	$(TOP)/vendor/sprd/proprietories-source/autotest/liblkvnpi \
	vendor/sprd/proprietories-source/engpc/sprd_fts_inc \
	$(TOP)/vendor/sprd/proprietories-source/autotest/interface/at_tok

LOCAL_SHARED_LIBRARIES := \
	libcutils    \
	liblog \
	libhardware \
	libeng_tok

include $(BUILD_SHARED_LIBRARY)
