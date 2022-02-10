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

#include <android/log.h>
#include <cutils/android_reboot.h>
#include <cutils/properties.h>

#include "sprd_fts_type.h"
#include "sprd_fts_log.h"
#include "sprd_fts_diag.h"

#include "keypad_npi.h"
#include "input_npi.h"
#include "key_common.h"

static int keypad_open_test(void)
{
	return inputOpen();
}

static int keypad_read_test(char *buff)
{
	int ret = 0;
	struct kpd_info_t kpdinfo;
	memset(&kpdinfo, 0, sizeof(struct kpd_info_t));

	ret = inputKPDGetKeyInfo(&kpdinfo);
	if (ret < 0) {
		buff[0] = 0xff; //col
		buff[1] = 0xff; //row
		buff[2] = 0; // key hight byte
		buff[3] = 0; // key low byte
		buff[4] = 0; // gpio high byte
		buff[5] = 0; // gpio low byte
	} else {
		buff[0] = (char)(kpdinfo.col & 0xff);
		buff[1] = (char)(kpdinfo.row & 0xff);
		buff[2] = (char)((kpdinfo.key >> 8) & 0xff);
		buff[3] = (char)(kpdinfo.key & 0xff);
		buff[4] = (char)((kpdinfo.gio >> 8) & 0xff);
		buff[5] = (char)(kpdinfo.gio & 0xff);
	}

	return 0;
}

static int keypad_close_test(void)
{
	return inputClose();
}

static int keypad_pbint_test()
{
	KEY_LOGD("this function can`t suport\n");
	return 0;
}

int testKEYPAD(char *buf, int buf_len, char *rsp, int rsplen)
{
	int ret = 1;
	char *data_sec = NULL;
	MSG_HEAD_T *msg_head_ptr = NULL;
	unsigned char ready_read = NO;
	char rvlbuff[6] = {0};
	char *ret_val = NULL;

	KEY_LOGI("entry testKEYPAD\n");

	memcpy(rsp, buf, sizeof(MSG_HEAD_T) + 1);
	msg_head_ptr = (MSG_HEAD_T *)(rsp + 1);
	data_sec = buf + 1 + sizeof(MSG_HEAD_T);
	ret_val = &msg_head_ptr->len;

	switch(*data_sec) {

	case KEYPAD_OPEN_TEST:
		ret = keypad_open_test();
		break;
	case KEYPAD_READ_TEST:
		ret = keypad_read_test(rvlbuff);
		ready_read = YES;
		break;
	case KEYPAD_CLOSE_TEST:
		ret = keypad_close_test();
		break;
	case KEYPAD_PBINT_TEST:
		ret = keypad_pbint_test();
		break;
	default:
		KEY_LOGD("can`t find CMD function\n");
		ret = -1;
		break;
	};

	if (ret < 0){
		msg_head_ptr->subtype = 1;
	}else{
		msg_head_ptr->subtype = 0;
	}

	*ret_val = ret;
	msg_head_ptr->len = sizeof(MSG_HEAD_T);
	if (ready_read == YES) {
		memcpy(&rsp[msg_head_ptr->len + 1], rvlbuff, sizeof(rvlbuff));
		msg_head_ptr->len += sizeof(rvlbuff);
	}
	rsp[msg_head_ptr->len + 1] = 0x7e;

	return (1 + msg_head_ptr->len + 1);
}


void register_this_module_ext(struct eng_callback *reg, int *num)
{
	unsigned int moudles_num = 0;
	char boot_mode[PROPERTY_VALUE_MAX] = {'\0'};

	(reg + moudles_num)->type = 0x38;
	(reg + moudles_num)->subtype = KEYPAD_FUNCTEST;
	(reg + moudles_num)->eng_diag_func = testKEYPAD;
	KEY_LOGD("register module for Diag cmd testKEYPAD : [%p] \n", testKEYPAD);
	moudles_num ++;

	*num = moudles_num;

	if (property_get("ro.bootmode", boot_mode, NULL)
		&& !strcmp(boot_mode, "autotest"))
		key_init();

	return;
}

