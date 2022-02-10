
#ifndef _MCU_LCD_20180419_H__
#define _MCU_LCD_20180419_H__


#define LOG_TAG_LCD "mcu_lcd_npi"

#define LCD_LOGI(fmt, args...) \
  __android_log_print(ANDROID_LOG_INFO, LOG_TAG_LCD, fmt, ##args)
#define LCD_LOGD(fmt, args...) \
  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_LCD, fmt, ##args)
#define LCD_LOGE(fmt, args...) \
  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_LCD, fmt, ##args)


#define PAGE_SIZE 4096

int lcdOpen( void );
int lcdDrawBackground( uint rgbValue );
int lcdClose( void );

#endif // _MCU_LCD_20180419_H__