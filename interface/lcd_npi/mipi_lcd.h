#ifndef __MIPI_LCD_20180419_H__
#define __MIPI_LCD_20180419_H__

#include <android/log.h>

#define LOG_TAG_LCD "mipi_lcd_npi"

#define LCD_LOGI(fmt, args...) \
  __android_log_print(ANDROID_LOG_INFO, LOG_TAG_LCD, fmt, ##args)
#define LCD_LOGD(fmt, args...) \
  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_LCD, fmt, ##args)
#define LCD_LOGE(fmt, args...) \
  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_LCD, fmt, ##args)

void test_result_init(void);
void test_lcd_uinit(void);
void test_lcd_splitdisp(int  data);
int skd_test_lcd_start(void);
void test_bl_lcd_start(void);

void test_lcd_start(void);
void test_lcd_close(void);


#endif //__MIPI_LCD_20180419_H__
