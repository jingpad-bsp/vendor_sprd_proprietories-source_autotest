

#ifndef __KEYPAD_20180419_NPI_H__
#define __KEYPAD_20180419_NPI_H__

#include <android/log.h>

#define KEY_TAG_KEYPAD "keypad_npi"

#define KEY_LOGI(fmt, args...) \
  __android_log_print(ANDROID_LOG_INFO, KEY_TAG_KEYPAD, fmt, ##args)
#define KEY_LOGD(fmt, args...) \
  __android_log_print(ANDROID_LOG_DEBUG, KEY_TAG_KEYPAD, fmt, ##args)
#define KEY_LOGE(fmt, args...) \
  __android_log_print(ANDROID_LOG_ERROR, KEY_TAG_KEYPAD, fmt, ##args)


#define KEYPAD_FUNCTEST 0x01

enum {
	KEYPAD_OPEN_TEST  = 0x01,
	KEYPAD_READ_TEST  = 0x02,
	KEYPAD_CLOSE_TEST = 0x03,
	KEYPAD_PBINT_TEST = 0x04
};

#define YES 	1
#define NO 		0


#endif //__KEYPAD_20180419_NPI_H__
