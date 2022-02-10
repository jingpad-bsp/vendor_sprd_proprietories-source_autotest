
#ifndef _FINGER_MMI_20180903_H__
#define _FINGER_MMI_20180903_H__


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdlib.h>
#include <hardware/hardware.h>
#include <cutils/log.h>
#include <dlfcn.h>

#define FINGERPRINT_LIB_Default "libfactorylib.so"             //default fingerprint lib path
#define LOG_TAG "MMI_FINGER"

int fp_factory_init();
int fp_spi_test();
int fp_deadpixel_test();
int fp_interrupt_test();
int fp_finger_detect();
int fp_factory_exit();
int fp_test(int cmd_type);
int fp_test_fun(int cmd_type);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _FINGER_MMI_20180903_H__
