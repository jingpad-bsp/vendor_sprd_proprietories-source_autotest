LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := otg.cpp

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := autotestotg
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH := npidevice

ifneq (,$(findstring 9863, $(TARGET_PRODUCT)))
PLATFORM := sharkl3
else ifneq (,$(findstring 7731, $(TARGET_PRODUCT)))
PLATFORM := pike2
else ifneq (,$(findstring 9832, $(TARGET_PRODUCT)))
PLATFORM := sharkle
else ifneq (,$(findstring 9820, $(TARGET_PRODUCT)))
PLATFORM := sharkle
else ifneq (,$(findstring 9853, $(TARGET_PRODUCT)))
PLATFORM := isharkl2
else
PLATFORM := default
endif

ifeq ($(KERNEL_PATH), kernel4.4)
LOCAL_CFLAGS += -DKERNEL_OLD
else
LOCAL_CFLAGS += -DKERNEL_NEW
endif

$(warning TOP:$(TOP) LOCAL_PATH:$(LOCAL_PATH) TARGET_PRODUCT:$(TARGET_PRODUCT)  platform:$(PLATFORM))
LOCAL_C_INCLUDES:= \
    $(TOP)/vendor/sprd/proprietories-source/engpc/sprd_fts_inc \
    $(TOP)/vendor/sprd/proprietories-source/autotest/interface/include \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL/source/include/uapi \
    $(LOCAL_PATH)/$(PLATFORM)

LOCAL_SHARED_LIBRARIES:= \
    libcutils \
    liblog

include $(BUILD_SHARED_LIBRARY)
