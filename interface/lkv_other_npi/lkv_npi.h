
#ifndef __LKV_20180420_NPI_H__
#define __LKV_20180420_NPI_H__

#include <android/log.h>

#define LKV_TAG_KEYPAD "lkv_npi"

#define LKV_LOGI(fmt, args...) \
  __android_log_print(ANDROID_LOG_INFO, LKV_TAG_KEYPAD, fmt, ##args)
#define LKV_LOGD(fmt, args...) \
  __android_log_print(ANDROID_LOG_DEBUG, LKV_TAG_KEYPAD, fmt, ##args)
#define LKV_LOGE(fmt, args...) \
  __android_log_print(ANDROID_LOG_ERROR, LKV_TAG_KEYPAD, fmt, ##args)

#endif //__LKV_20180420_NPI_H__
