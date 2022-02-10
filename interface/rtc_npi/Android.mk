LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := \
    npi_rtc_test.cpp \

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libnpi_rtc
LOCAL_MODULE_TAGS := optional


LOCAL_C_INCLUDES += hardware/libhardware/include \
    vendor/sprd/proprietories-source/engpc/sprd_fts_inc \

LOCAL_SHARED_LIBRARIES:= \
    libcutils \
    libdl \
    libutils \
    liblog  \
    libbase \

AUDIO_NPI_FILE := /vendor/lib/libnpi_rtc.so
SYMLINK := $(TARGET_OUT_VENDOR)/lib/npidevice/libnpi_rtc.so
LOCAL_POST_INSTALL_CMD := $(hide) \
    mkdir -p $(TARGET_OUT_VENDOR)/lib/npidevice; \
    rm -rf $(SYMLINK) ;\
    ln -sf $(AUDIO_NPI_FILE) $(SYMLINK);

include $(BUILD_SHARED_LIBRARY)
