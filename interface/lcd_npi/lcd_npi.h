
#ifndef __LCD_20180419_NPI_H__
#define __LCD_20180419_NPI_H__

#define LOG_TAG_LCD "lcd_npi"

#define LCD_LOGI(fmt, args...) \
  __android_log_print(ANDROID_LOG_INFO, LOG_TAG_LCD, fmt, ##args)
#define LCD_LOGD(fmt, args...) \
  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_LCD, fmt, ##args)
#define LCD_LOGE(fmt, args...) \
  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_LCD, fmt, ##args)


#define LCD_FUNCTEST	0x02
#define LCD_IDTEST		0x04

#define NPI_LCD_FAILED    1
#define NPI_LCD_SUCCESS    0

enum {

	MCU_LCD_SEND = 0x01,
	MCU_LCD_CLOSE = 0x02,
	MIPI_LCD_SEND = 0x11,
	MIPI_LCD_CLOSE = 0x12,
	MIPI_LCD_LOOP_COLR = 0x20,
	MIPI_LCD_SINGLE_COLR = 0x21,
	MIPI_LCD_CLOSE_COLR = 0x22
};

enum {

	COLR_WHITE = 0x00,
	COLR_RAD = 0x01,
	COLR_GREEN = 0x02,
	COLR_BULE = 0x03,
	COLR_BLACK = 0x04
};

#define START 1
#define CLOSE 0

#endif //__LCD_20180419_NPI_H__
