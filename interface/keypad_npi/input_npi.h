
#ifndef __INPUT_20180419_H__
#define __INPUT_20180419_H__

#include <android/log.h>

#define INPUT_TAG_KEYPAD "input_npi"

#define INPUT_LOGI(fmt, args...) \
  __android_log_print(ANDROID_LOG_INFO, INPUT_TAG_KEYPAD, fmt, ##args)
#define INPUT_LOGD(fmt, args...) \
  __android_log_print(ANDROID_LOG_DEBUG, INPUT_TAG_KEYPAD, fmt, ##args)
#define INPUT_LOGE(fmt, args...) \
  __android_log_print(ANDROID_LOG_ERROR, INPUT_TAG_KEYPAD, fmt, ##args)

//#define FUN_ENTER do { \
//	INPUT_LOGI("%s enter ...\n", __FUNCTION__); } while(0)

//#define FUN_EXIT do { \
//	INPUT_LOGI("%s exit ...\n", __FUNCTION__); } while(0)

struct kpd_info_t {
	int key;
	ushort row;
	ushort col;
	ushort gio;
};

int inputOpen( void );
int inputOpen2( void );

int inputKPDGetKeyInfo( struct kpd_info_t * info );

int inputKPDWaitKeyPress( struct kpd_info_t * info, int timeout );
int inputKPDWaitKeyPress2( struct kpd_info_t * info, int timeout );

int inputTPGetPoint( int *x, int *y, int timeout );

int inputClose( void );
int inputClose2( void );

#endif // __INPUT_20180419_H__