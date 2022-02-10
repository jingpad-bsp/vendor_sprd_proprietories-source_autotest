LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)


LOCAL_SRC_FILES := \
	lcd_npi.c \
	mcu_lcd.c \
	mipi_lcd.c \
	ui_npi.c \
	graphics.c \
	graphics_adf.c \
	graphics_fbdev.c \
	graphics_drm.c \
	events.c \
	resources.c

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := liblcdnpi
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH := npidevice

LOCAL_C_INCLUDES += \
	vendor/sprd/proprietories-source/engpc/sprd_fts_inc \
	vendor/sprd/proprietories-source/autotest/lcd_npi/ \
	vendor/sprd/proprietories-source/autotest/minui/ \
	$(TOP)/vendor/sprd/proprietories-source/autotest/interface/include \
	system/core/adf/libadf/include \
	external/libpng

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils \
	libadf \
	libdrm \
	libpng

LOCAL_STATIC_LIBRARIES :=

$(warning "TARGET_ARCH= $(TARGET_ARCH)")
ifeq ($(strip $(TARGET_ARCH)),arm64)
LOCAL_CFLAGS += -DTARGET_ARCH_ARM64
else
ifeq ($(strip $(TARGET_ARCH)),x86_64)
LOCAL_CFLAGS += -DTARGET_ARCH_x86_64
else
LOCAL_CFLAGS += -DTARGET_ARCH_ARM
endif
endif

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
