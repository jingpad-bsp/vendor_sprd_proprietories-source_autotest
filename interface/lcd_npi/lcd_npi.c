#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <android/log.h>
//#include <cutils/android_reboot.h>
#include "sprd_fts_type.h"
#include "sprd_fts_diag.h"
#include "sprd_fts_log.h"

#include "mcu_lcd.h"
#include "mipi_lcd.h"
#include "lcd_npi.h"

int mcu_lcd_test(const char *data, char flag)
{
	int ret = 0;
	unsigned int rgbcolr = 0;

	if (flag == START) {

		memcpy(&rgbcolr, data, sizeof(char)*3);
		ret = lcdOpen();
		if (ret < 0) {
			LCD_LOGE("mcu lcd test [lcdOpen] failed\n"); 
			return NPI_LCD_FAILED;
		}

		ret = lcdDrawBackground(rgbcolr);
		if (ret < 0) {
			LCD_LOGE("mcu lcd test [lcdDrawBackground] failed\n"); 
			return NPI_LCD_FAILED;
		}

	} else
		lcdClose();
	return ret;
}

void mipi_lcd_send_test(const char *data)
{
	test_result_init();
	test_lcd_start();
}


void mipi_lcd_close_test(const char *data)
{
	test_lcd_uinit();
}

void mipi_lcd_single_colr(const char *data)
{
	test_result_init();
	test_lcd_splitdisp(*data);
}


void mipi_lcd_close_colr(const char *data)
{
	mipi_lcd_close_test(data);
}


int testLCD(char *buf, int buf_len, char *rsp, int rsplen)
{

	int ret = NPI_LCD_SUCCESS;

	MSG_HEAD_T *msg_head_ptr = NULL;
	char *data_sec = NULL;
	unsigned int data_len = 0;
	unsigned char *ret_val = NULL;

	memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T));
	//memcpy(rsp, buf, buf_len);
	msg_head_ptr = (MSG_HEAD_T *)(rsp+1);
	ret_val = &msg_head_ptr->subtype;

	data_sec = buf + 1 + sizeof(MSG_HEAD_T);
	if (data_sec == NULL) {
			LCD_LOGE(" data formal failed\n");
			ret = -1;
			goto out;
	}

	switch (*data_sec) {

	case MCU_LCD_SEND:
		ret = mcu_lcd_test(data_sec + 1, START);
		break;
	case MCU_LCD_CLOSE:
		ret =mcu_lcd_test(data_sec + 1, CLOSE);
		break;
	case MIPI_LCD_SEND:
		mipi_lcd_send_test(data_sec + 1);
		break;
	case MIPI_LCD_CLOSE:
		mipi_lcd_close_test(data_sec + 1);
		break;
	case MIPI_LCD_LOOP_COLR:
		mipi_lcd_send_test(data_sec + 1);
		break;
	case MIPI_LCD_SINGLE_COLR:
		mipi_lcd_single_colr(data_sec + 1);
		break;
	case MIPI_LCD_CLOSE_COLR:
		mipi_lcd_close_colr(data_sec + 1);
		break;
	default:
		LCD_LOGE("this formal test can`t find\n");
		ret = -1;
		break;
	}

out:
	if (ret < 0)
		ret = 1;

	*ret_val = ret;
	msg_head_ptr->len = sizeof(MSG_HEAD_T);
	rsp[msg_head_ptr->len + 1] = 0x7e;

	return (1 + msg_head_ptr->len + 1);
}

int testID(char *buf, int buf_len, char *rsp, int rsplen)
{
	LCD_LOGI("this func not suport!!!\n");
	return 0;
}

void register_this_module_ext(struct eng_callback *reg, int *num)
{

	int moudles_num = 0;

	(reg + moudles_num)->type = 0x38;
	(reg + moudles_num)->subtype = LCD_FUNCTEST;
	(reg + moudles_num)->eng_diag_func = testLCD;
	LCD_LOGD("register module for Diag cmd testLCD : [%p] \n", testLCD);
	moudles_num ++;

	(reg + moudles_num)->type = 0x38;
	(reg + moudles_num)->subtype = LCD_IDTEST;
	(reg + moudles_num)->eng_diag_func = testID;
	LCD_LOGD("register module for Diag cmd testID : [%p] \n", testID);
	moudles_num ++; 
	
	*num = moudles_num;
	return;
}


