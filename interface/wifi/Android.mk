LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(BBAT_BIT64), true)
LOCAL_32_BIT_ONLY := false
else
LOCAL_32_BIT_ONLY := true
endif

LOCAL_SRC_FILES := wifinew.cpp \
                   wifi_interface.cpp

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := autotestwifi
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH := npidevice

ifdef WIFI_DRIVER_MODULE_PATH
LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_PATH=\"$(WIFI_DRIVER_MODULE_PATH)\"
endif

ifdef WIFI_DRIVER_MODULE_NAME
LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_NAME=\"$(WIFI_DRIVER_MODULE_NAME)\"
endif

LOCAL_C_INCLUDES:= \
    external/wpa_supplicant_8/wpa_supplicant/wpa_client_include \
    frameworks/opt/net/wifi/libwifi_hal/include \
    $(TOP)/vendor/sprd/proprietories-source/engpc/sprd_fts_inc

LOCAL_SHARED_LIBRARIES:= \
    liblog \
    libcutils  \
    android.hardware.wifi@1.0 \
    libwpa_client


include $(BUILD_SHARED_LIBRARY)
