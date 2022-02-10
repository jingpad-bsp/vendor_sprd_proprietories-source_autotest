#include <fcntl.h>
#include <pthread.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include <android/log.h>
#include <cutils/android_reboot.h>
#include "sprd_fts_type.h"
#include "sprd_fts_diag.h"
#include "sprd_fts_log.h"
#include "eng_attok_comm.h"

#include "lkv_npi.h"
#include "light.h"
#include "tp.h"

#define LKV_FUNCTEST 0x0c

enum {
	LCD_BL = 0x00,
	KEYPAD_BL = 0x01,
	VIBRATOR = 0x03,
	FLASHLIGHT = 0x04,
	TOUCHSCREEN = 0x07,
	RGB_LIGHT = 0x09
};

int lcd_bl_test(char *data)
{
	int ret = 0;
	int value = *data;

	ret = lightOpen();
	if (ret < 0) {
		LKV_LOGE("lcd_bl_test open lightOpen failed\n");
		return ret;
	}

	if( value > 0 ) {
		ret = lightSetLCD(value);
	} else {
		ret = lightSetLCD(0);
		ret = lightClose();
	}

	return ret;
}

int keypad_bl_test(char *data)
{
	int ret = 0;
	int value = *data;

	ret = lightOpen();
	if (ret < 0) {
		LKV_LOGE("keypad_bl_test open lightOpen failed\n");
		return ret;
	}

	if (value > 0) {
		ret = lightSetKeypad(value);
	} else {
		ret = lightSetKeypad(0);
		ret = lightClose();
	}

	return ret;
}

int vibrator_test(char *data)
{
	int ret = 0;
	int value = *data;

	if (value > 0) {

		ret = vibOpen();
		if (ret < 0){
			LKV_LOGE("vibrator_test open fail\n");
			return ret;
		} else {
			ret = vibTurnOn(value);
		}

	} else {
		ret = vibTurnOff();
		ret = vibClose();
	}

	return ret;
}

int flashlightSetValue(int value)
{
	int ret = 0;
	char cmd[200] = " ";

	sprintf(cmd, "echo 0x%02x > /sys/class/misc/sprd_flash/test", value);
	ret = system(cmd) ? -1 : 0;
	LKV_LOGD("cmd = %s,ret = %d\n", cmd,ret);

	return ret;
}

int flashlight_test(char *data)
{
	int ret = 0;
	int cmd_val = 0;

	//0x00:off,!0x00 open
	if((*data) != 0x00) {
		cmd_val |= 0x00; //open flashlight
		cmd_val |= ((*data) & 0x0f) << 4; //flash index
	} else if((*data) == 0x00) {
		cmd_val |= 0x31; //close flashlight
	}

	if( flashlightSetValue(cmd_val) < 0 ) {
		ret = -1;
	}

	return ret;
}

int rgb_light_test(char *data)
{
	int ret = 0;
	int value1 = (int)data[0];//0x00:RED , 0x01:GREEN , 0x02:BLUE
	int value2 = (int)data[1];//0x01:on ,0x00:off

	if(value1 >= 0x00)
		ret = lightSetRgb(value1 , value2);
	else
		ret = -1;

	return ret;
}

// vibrator LCD B/L Keypad B/L RGB Light
int testLKV(char *buf, int buf_len, char *rsp, int rsplen)
{
	int ret = 0;
	char *data_sec = NULL;
	MSG_HEAD_T *msg_head_ptr = NULL;
	char *ret_val = NULL;
	struct tp_point point;

	memcpy(rsp, buf, sizeof(MSG_HEAD_T) + 1);
	msg_head_ptr = (MSG_HEAD_T *)(rsp + 1);
	data_sec = buf + sizeof(MSG_HEAD_T) + 1;
	ret_val = &msg_head_ptr->subtype;
	msg_head_ptr->len = sizeof(MSG_HEAD_T);

	switch (*data_sec) {

	case LCD_BL:
		ret = lcd_bl_test(data_sec + 1);
		break;
	case KEYPAD_BL:
		ret = keypad_bl_test(data_sec + 1);
		break;
	case VIBRATOR:
		ret = vibrator_test(data_sec + 1);
		break;
	case FLASHLIGHT:
		ret = flashlight_test(data_sec + 1);
		break;
	case TOUCHSCREEN:
		tp_resume();
		switch (data_sec[1]) {
		case 1:
			ret = tp_test_id();
			if (!ret)
				rsp[++(msg_head_ptr->len)] = 0;
			else
				rsp[++(msg_head_ptr->len)] = 1;
			ret = 0;
			break;
		case 2:
			memset(&point, 0, sizeof(point));
			ret = tp_test_point(&point);
			if (!ret) {
				rsp[++(msg_head_ptr->len)] =
					(((unsigned int)point.x) >> 8) & 0xff;
				rsp[++(msg_head_ptr->len)] =
					((unsigned int)point.x) & 0xff;
				rsp[++(msg_head_ptr->len)] =
					(((unsigned int)point.y) >> 8) & 0xff;
				rsp[++(msg_head_ptr->len)] =
					((unsigned int)point.y) & 0xff;
				rsp[++(msg_head_ptr->len)] =
					(((unsigned int)point.x) >> 8) & 0xff;
				rsp[++(msg_head_ptr->len)] =
					((unsigned int)point.x) & 0xff;
				rsp[++(msg_head_ptr->len)] =
					(((unsigned int)point.y) >> 8) & 0xff;
				rsp[++(msg_head_ptr->len)] =
					((unsigned int)point.y) & 0xff;
			} else {
				rsp[++(msg_head_ptr->len)] = 1;
			}
			ret = 0;
			break;
		default:
			return ENG_DIAG_RET_UNSUPPORT;
		}
		break;
	case RGB_LIGHT:
		ret = rgb_light_test(data_sec + 1);
		break;

	default:
		break;
	}

	if (ret < 0)
		ret = 1;

	*ret_val = ret;
	rsp[msg_head_ptr->len + 1] = 0x7e;

	return  (1 + msg_head_ptr->len + 1);
}

static int _light_test_internal(LIGHT_IDX light_index, char *req, char *rsp)
{
    char *ptr = NULL;
    int ret = 0;
    int brightness = 0;

    if (light_index >= LIGHT_COUNT) {
        LKV_LOGE("%s, invalid index number: %d\n", __func__, light_index);
        return -1;
    }

    if (!req || !rsp) {
        LKV_LOGE("%s, invalid req or rsp\n", __func__);
        return -1;
    }

    ptr = strdup(req);
    if (!ptr) {
        LKV_LOGE("%s, dump request failed\n", __func__);
        return -1;
    }

    ret = at_tok_equel_start(&ptr);
    if (ret) {
        LKV_LOGE("%s, equel_start failed, ptr=%s,ret=%d\n", __func__, ptr, ret);
        goto err;
    }

    ret = at_tok_nextint(&ptr, &brightness);
    if (ret) {
        LKV_LOGE("%s, parse brightness failed, ptr=%s,ret=%d\n", __func__, ptr, ret);
        goto err;
    }

    if (brightness >0) {
        ret = lightOpen();
        if (ret < 0){
            LKV_LOGE("%s open light failed, ret=%d\n", __func__, ret);
            goto err;
        }
        ret = set_light(light_index, brightness);
        if (ret < 0) {
            LKV_LOGE("%s turn on light[%d] failed, ret=%d\n", __func__, light_index, ret);
            goto err;
        }
    } else {
        ret = set_light(light_index, 0);
        ret = lightClose();
        if (ret) {
            LKV_LOGE("%s turn off light[%d] failed, ret=%d\n", __func__, light_index, ret);
            goto err;
        }
    }

ok:
    free(ptr);
    ptr = NULL;
    sprintf(rsp, "\r\nOK\r\n");

    return 0;
err:
    free(ptr);
    ptr = NULL;
    sprintf(rsp, "\r\nERROR\r\n");

    return -1;
}

static int at_backlight_test(char *req, char *rsp)
{
    int ret = 0;

    ret |= _light_test_internal(LIGHT_INDEX_BACKLIGHT, req, rsp);
    ret |= _light_test_internal(LIGHT_INDEX_BUTTONS, req, rsp);
    ret |= _light_test_internal(LIGHT_INDEX_KEYBOARD, req, rsp);
    LKV_LOGE("%s, at_backlight_test ret=%d\n", __func__);
    return ret;
}

static int at_vibrator_test(char *req, char *rsp)
{
	char *ptr = NULL;
	int ret = 0;
	int timeout_ms = 0;

	if (!req || !rsp) {
		LKV_LOGE("%s, invalid req or rsp\n", __func__);
		return -1;
	}

	ptr = strdup(req);
	if (!ptr) {
		LKV_LOGE("%s, dump request failed\n", __func__);
		return -1;
	}

	ret = at_tok_equel_start(&ptr);
	if (ret) {
		LKV_LOGE("%s, equel_start failed, ptr=%s,ret=%d\n", __func__, ptr, ret);
		goto err;
	}

	ret = at_tok_nextint(&ptr, &timeout_ms);
	if (ret) {
		LKV_LOGE("%s, parse timeout failed, ptr=%s,ret=%d\n", __func__, ptr, ret);
		goto err;
	}

	if (timeout_ms > 0) {
		ret = vibOpen();
		if (ret < 0){
			LKV_LOGE("%s open vibrator failed, ret=%d\n", __func__, ret);
			goto err;
		} else {
			ret = vibTurnOn(timeout_ms);
			if (ret < 0) {
				vibTurnOff();
				vibClose();
				LKV_LOGE("%s turn on vibrator failed, ret=%d\n", __func__, ret);
				goto err;
			}
		}
	} else {
		ret = vibTurnOff();
		ret |= vibClose();
		if (ret) {
			LKV_LOGE("%s turn off vibrator failed, ret=%d\n", __func__, ret);
			goto err;
		}
	}

ok:
	free(ptr);
	ptr = NULL;
	sprintf(rsp, "\r\nOK\r\n");

	return 0;
err:
	free(ptr);
	ptr = NULL;
	sprintf(rsp, "\r\nERROR\r\n");

	return -1;
}

static int at_rgb_light_test(char *req, char *rsp)
{
	char *ptr = NULL;
	int ret = 0;
	int cmd = 0;

	if (!req || !rsp) {
		LKV_LOGE("%s, invalid req or rsp\n", __func__);
		return -1;
	}

	ptr = strdup(req);
	if (!ptr) {
		LKV_LOGE("%s, dump request failed\n", __func__);
		return -1;
	}

	ret = at_tok_equel_start(&ptr);
	if (ret) {
		LKV_LOGE("%s, equel_start failed, ptr=%s,ret=%d\n", __func__, ptr, ret);
		goto err;
	}

	ret = at_tok_nextint(&ptr, &cmd);
	if (ret) {
		LKV_LOGE("%s, parse cmd failed, ptr=%s,ret=%d\n", __func__, ptr, ret);
		goto err;
	}

	switch (cmd) {
	case 0:
		ret = lightSetRgb(0, 0);
		break;
	case 1:
		ret = lightSetRgb(0, 1);
		ret |= lightSetRgb(1, 1);
		ret |= lightSetRgb(2, 1);
		break;
	case 10:
		ret = lightSetRgb(0x1f, 0);
		break;
	case 11:
		ret = lightSetRgb(0, 1);
		break;
	case 20:
		ret = lightSetRgb(0x2f, 0);
		break;
	case 21:
		ret = lightSetRgb(1, 1);
		break;
	case 30:
		ret = lightSetRgb(0x3f, 0);
		break;
	case 31:
		ret = lightSetRgb(2, 1);
		break;
	default:
		LKV_LOGE("%s, invalid cmd(%d)\n", __func__, cmd);
		goto err;
	}

	if (ret) {
		LKV_LOGE("%s, lightSetRgb failed, cmd=%d,ret=%d\n", __func__, cmd, ret);
		goto err;
	}
ok:
	free(ptr);
	ptr = NULL;
	sprintf(rsp, "\r\nOK\r\n");

	return 0;
err:
	free(ptr);
	ptr = NULL;
	sprintf(rsp, "\r\nERROR\r\n");

	return -1;
}

void register_this_module_ext(struct eng_callback *reg, int *num)
{
	unsigned int modules_num = 0;

	(reg + modules_num)->type = 0x38;
	(reg + modules_num)->subtype = LKV_FUNCTEST;
	(reg + modules_num)->diag_ap_cmd = 0x00;
	(reg + modules_num)->eng_diag_func = testLKV;
	LKV_LOGD("register module for Diag cmd test-LCDBL : [%p] \n", testLKV);
	modules_num ++;

	(reg + modules_num)->type = 0x38;
	(reg + modules_num)->subtype = LKV_FUNCTEST;
	(reg + modules_num)->diag_ap_cmd = 0x01;
	(reg + modules_num)->eng_diag_func = testLKV;
	LKV_LOGD("register module for Diag cmd test-KPDBL : [%p] \n", testLKV);
	modules_num ++;

	(reg + modules_num)->type = 0x38;
	(reg + modules_num)->subtype = LKV_FUNCTEST;
	(reg + modules_num)->diag_ap_cmd = 0x03;
	(reg + modules_num)->eng_diag_func = testLKV;
	LKV_LOGD("register module for Diag cmd test-Vibra : [%p] \n", testLKV);
	modules_num ++;

	(reg + modules_num)->type = 0x38;
	(reg + modules_num)->subtype = LKV_FUNCTEST;
	(reg + modules_num)->diag_ap_cmd = 0x07;
	(reg + modules_num)->eng_diag_func = testLKV;
	LKV_LOGD("register module for Diag cmd test-Touchpanel : [%p] \n", testLKV);
	modules_num ++;
#if 0
	(reg + modules_num)->type = 0x38;
	(reg + modules_num)->subtype = LKV_FUNCTEST;
	(reg + modules_num)->diag_ap_cmd = 0x04;
	(reg + modules_num)->eng_diag_func = testLKV;
	LKV_LOGD("register module for Diag cmd test-Flsh : [%p] \n", testLKV);
	modules_num ++;
#endif
	(reg + modules_num)->type = 0x38;
	(reg + modules_num)->subtype = LKV_FUNCTEST;
	(reg + modules_num)->diag_ap_cmd = 0x09;
	(reg + modules_num)->eng_diag_func = testLKV;
	LKV_LOGD("register module for Diag cmd test-RGB : [%p] \n", testLKV);
	modules_num ++;

	sprintf((reg + modules_num)->at_cmd, "%s", "AT+VIBRATORTEST");
	(reg + modules_num)->eng_linuxcmd_func = at_vibrator_test;
	(reg + modules_num)->subtype = LKV_FUNCTEST;
	LKV_LOGD("register_at cmd for vibrator : [%p] \n", at_vibrator_test);
	modules_num++;

	sprintf((reg + modules_num)->at_cmd, "%s", "AT+LEDTEST");
	(reg + modules_num)->eng_linuxcmd_func = at_rgb_light_test;
	(reg + modules_num)->subtype = LKV_FUNCTEST;
	LKV_LOGD("register_at cmd for rgb light : [%p] \n", at_rgb_light_test);
	modules_num++;

	sprintf((reg + modules_num)->at_cmd, "%s", "AT+BACKLIGHTTEST");
	(reg + modules_num)->eng_linuxcmd_func = at_backlight_test;
	(reg + modules_num)->subtype = LKV_FUNCTEST;
	LKV_LOGD("register_at cmd for backlight AT+BACKLIGHTTEST: [%p] "" \n", at_backlight_test);
	modules_num++;

	*num = modules_num;
	return;
}
