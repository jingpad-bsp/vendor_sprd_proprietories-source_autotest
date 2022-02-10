LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := \
	keypad_npi.c \
	input_npi.c \
	debug.cpp \
	driver.cpp \
	key_common.c \
	graphics.c graphics_adf.c graphics_fbdev.c events.c

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libkeypadnpi
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH := npidevice


LOCAL_C_INCLUDES += \
	$(TOP)/vendor/sprd/proprietories-source/autotest/interface/include \
	$(TOP)/vendor/sprd/proprietories-source/autotest/keypad_npi \
	vendor/sprd/proprietories-source/engpc/sprd_fts_inc \
	system/core/adf/libadf/include \
	external/libpng



LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils \
	libadf \
	libpng

LOCAL_STATIC_LIBRARIES :=

ifeq ($(subst ",,$(TARGET_RECOVERY_PIXEL_FORMAT)),RGBX_8888)
    LOCAL_CFLAGS += -DRECOVERY_RGBX
endif

ifeq ($(subst ",,$(TARGET_RECOVERY_PIXEL_FORMAT)),BGRA_8888)
    LOCAL_CFLAGS += -DRECOVERY_BGRA
endif

ifneq ($(TARGET_RECOVERY_OVERSCAN_PERCENT),)
    LOCAL_CFLAGS += -DOVERSCAN_PERCENT=$(TARGET_RECOVERY_OVERSCAN_PERCENT)
else
    LOCAL_CFLAGS += -DOVERSCAN_PERCENT=0
endif

include $(BUILD_SHARED_LIBRARY)
