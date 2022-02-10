LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	sensor_test_registeration.cpp \
	sensor_native_mmi_test.cpp \
	sensor_bbat_test.cpp \
	sensor.cpp \
	eng_attok.cpp


LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libsensornpi
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES += hardware/libhardware/include \
	vendor/sprd/proprietories-source/engpc/sprd_fts_inc \
	$(TOP)/vendor/sprd/proprietories-source/autotest/interface/sensor_npi

ifeq ($(USE_SPRD_SENSOR_LIB),true)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/no_sensorhub_cali
LOCAL_SRC_FILES += no_sensorhub_cali/accsensor_calibration.cpp
LOCAL_CPPFLAGS += -DNONSENSORHUB
endif # USE_SPRD_SENSOR_LIB

ifeq ($(USE_SPRD_SENSOR_HUB), true)
#LOCAL_SRC_FILES += sensor_native_mmi_cali.cpp
LOCAL_CPPFLAGS += -DSENSORHUB
endif # USE_SPRD_SENSOR_HUB

LOCAL_SHARED_LIBRARIES:= \
    libcutils \
    libdl \
    libutils \
    liblog	\
    libbase \
    libhidlbase \
    libhidltransport \
    libhardware \
    android.hardware.sensors@1.0

LOCAL_STATIC_LIBRARIES := \
	android.hardware.sensors@1.0-convert

AUDIO_NPI_FILE := /vendor/lib/libsensornpi.so
SYMLINK := $(TARGET_OUT_VENDOR)/lib/npidevice/libsensornpi.so
LOCAL_POST_INSTALL_CMD := $(hide) \
    mkdir -p $(TARGET_OUT_VENDOR)/lib/npidevice; \
    rm -rf $(SYMLINK) ;\
    ln -sf $(AUDIO_NPI_FILE) $(SYMLINK);

include $(BUILD_SHARED_LIBRARY)
