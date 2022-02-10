#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "interface.h"
#ifndef KERNEL_OLD
#include <linux/autotest.h>
#endif

#define CMD_OFFSET          (10)
#define CMD_DIS_OTG         (1)
#define CMD_READ_ID_STATUS  (2)
#define AUTOTEST_DEV_FILE	"/dev/autotest0"
#define BUF_MAX_LEN  200

#ifdef KERNEL_OLD
static int get_dev_node_path(char *node_name, char *path)
{
	if (!node_name && !path) {
		LOGD("%s, invalid parameter\n", __func__);
	}

	sprintf(path, "/sys/devices/platform/soc/soc:ap-ahb/20200000.usb/\"%s\"", node_name);
	LOGD("%s, path: %s", __func__, path);
	return 0;
}

static int otg_is_support(void)
{
	char path[BUF_MAX_LEN] = "";

	LOGD("%s, enter\n", __func__);
	get_dev_node_path("host_enable", path);

	if (access(path, F_OK) != -1) {
		LOGD("%s otg is support\n", __func__);
		return 1;
	} else {
		LOGD("%s otg is not support\n", __func__);
		return 0;
	}
}

static int otgEnable(void)
{
	char host_enable[8] = {0};
	int host_enable_fd = 0;
	int ret = 0;
	char cmd[200] = "";
	char path[BUF_MAX_LEN] = "";

	LOGD("%s, enter\n", __func__);
	FUN_ENTER;
	if(otg_is_support() == 1)
	{
		/* echo enable > /sys/devices/platform/soc/soc:ap-ahb/20200000.usb/host_enable */
		get_dev_node_path("host_enable", path);
		sprintf(cmd, "echo enable > %s", path);
		system(cmd);
		host_enable_fd = open(path, O_RDONLY);
		read(host_enable_fd, host_enable, 8);
		if(strncmp(host_enable, "enabled", sizeof("enabled") - 1) == 0)
		{
			ret = 0;
		}
		else
		{
			ret = -1;
		}
		close(host_enable_fd);
	}
	else
	{
		ret = -1;
	}
	FUN_EXIT;
	return ret;
}

static int otgDisable(void)
{
	char host_enable[8] = {0};
	int host_enable_fd = 0;
	int ret = 0;
	char cmd[200] = "";
	char path[BUF_MAX_LEN] = "";

	LOGD("%s, enter\n", __func__);
	FUN_ENTER;
	if(otg_is_support() == 1)
	{
		/* echo disable > /sys/devices/platform/soc/soc:ap-ahb/20200000.usb/host_enable */
		get_dev_node_path("host_enable", path);
		sprintf(cmd, "echo disable > %s", path);
		system(cmd);

		host_enable_fd = open(path, O_RDONLY);
		read(host_enable_fd, host_enable, 8);
		if(strncmp(host_enable, "disabled", sizeof("disabled") - 1) == 0)
		{
			ret = 0;
		}
		else
		{
			ret = -1;
		}
		close(host_enable_fd);
	}
	else
	{
		ret = -1;
	}
	FUN_EXIT;
	return ret;
}

static int otgIdStatus(unsigned char* value)
{
	char otg_status[8] = {0};
	int otg_status_fd = 0;
	char path[BUF_MAX_LEN] = "";
	int ret = 0;

	LOGD("%s, enter\n", __func__);
	FUN_ENTER;
	if(otg_is_support() == 1)
	{
		get_dev_node_path("otg_status", path);
		otg_status_fd = open(path, O_RDONLY);
		if(otg_status_fd < 0)
		{
			ret = -1;
		}
		else
		{
			memset(otg_status, 0, sizeof(otg_status));
			read(otg_status_fd, otg_status, 4);
			LOGD("otg_status = %s", otg_status);
			if(strncmp(otg_status, "high", sizeof("high") - 1) == 0)
			{
				*value = 1;
			}
			else if(strncmp(otg_status, "low", sizeof("low") - 1) == 0)
			{
				*value = 0;
			}
			close(otg_status_fd);
			ret = 0;
		}
	}
	else
	{
		ret = -1;
	}
	FUN_EXIT;
	return ret;
}
#else
struct otg_info {
	int cmd;
	int value;
};
static int otgDisable(void)
{
	int ret, fd;
	struct otg_info info;
	info.cmd = CMD_DIS_OTG;

	fd = open(AUTOTEST_DEV_FILE, O_RDWR);
	if (fd < 0) {
		LOGD("open auto test dev file failed.\n");
		goto err;
	}

	ret = ioctl(fd, AT_OTG, &info);
	if (ret) {
		LOGD("failed to disable otg !\n");
		ret = -1;
	}
err:
	close(fd);
	return ret;
}

static int otgIdStatus(unsigned char* value)
{
	int ret, fd;
	struct otg_info info;

	fd = open(AUTOTEST_DEV_FILE, O_RDWR);
	if (fd < 0) {
		LOGD("open auto test dev file failed.\n");
		goto err;
	}

	info.cmd = CMD_READ_ID_STATUS;
	info.value = 0xff;
	ret = ioctl(fd, AT_OTG, &info);
	if (ret) {
		LOGD("failed to read ID status !\n");
		ret = -1;
	}
	*value = info.value;
err:
	close(fd);
	return ret;
}
#endif

/*
   otg test
   PC-->engpc: 7E 49 00 00 00 0A 00 38 0C 05 01 7E   disable OTG
   ENGPC-->pc: 7E 00 00 00 00 08 00 38 00 7E
   PC-->engpc: 7E 40 00 00 00 0A 00 38 0C 05 02 7E   读取ID状态, XX:ID信号电平状态
   ENGPC-->pc: 7E 00 00 00 00 09 00 38 00 XX 7E
 */
int testOtg(char *buf, int len, char *rsp, int rsplen)
{
	int i;
	int ret = -1;
	unsigned char value = 0xff;
	unsigned char cmd;

	LOGD("pc->engpc:%d number--> ",len); //78 xx xx xx xx _ _38 _ _ 打印返回的10个数据
	for (i = 0; i < len; i++)
		LOGD("%x ", buf[i]);

	cmd = buf[CMD_OFFSET];

	switch (cmd) {
		case CMD_DIS_OTG:
			ret = otgDisable();
			break;
		case CMD_READ_ID_STATUS:
			ret = otgIdStatus(&value);
			break;
	}

	/*------------------------------后续代码为通用代码，所有模块可以直接复制，------------------------------------------*/

	//填充协议头7E xx xx xx xx 08 00 38
	MSG_HEAD_T *msg_head_ptr;
	memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T)-1);  //复制7E xx xx xx xx 08 00 38至rsp,，减1是因为返>回值中不包含MSG_HEAD_T的subtype（38后面的数据即为subtype）
	msg_head_ptr = (MSG_HEAD_T *)(rsp + 1);

	//修改返回数据的长度，如上复制的数据长度等于PC发送给engpc的数据长度，现在需要改成engpc返回给pc的数据长度。
	msg_head_ptr->len = 8; //返回的最基本数据格式：7E xx xx xx xx 08 00 38 xx 7E，去掉头和尾的7E，
	//共8个数据。如果需要返回数据,则直接在38 XX后面补充


	//填充成功或失败标志位rsp[1 + sizeof(MSG_HEAD_T)]，如果ret>0,则继续填充返回的数据。
	LOGD("msg_head_ptr,ret=%d, value: 0x%x",ret, value);

	if (ret < 0) {
		rsp[sizeof(MSG_HEAD_T)] = 1;    //38 01 表示测试失败。
	} else {
		rsp[sizeof(MSG_HEAD_T)] = 0;	//38 00 表示测试ok
		if (value != 0xff) {
			memcpy(rsp + 1 + sizeof(MSG_HEAD_T), &value, 1);        //将获取到的value复制>到38 00 后面
			msg_head_ptr->len+=1;   //返回长度：基本数据长度8+获value的长度.
		}
	}

	LOGD("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d",sizeof(MSG_HEAD_T),rsp[sizeof(MSG_HEAD_T)]);
	//填充协议尾部的7E
	rsp[msg_head_ptr->len + 2 - 1]=0x7E;  //加上数据尾标志
	LOGD("dylib test :return len:%d",msg_head_ptr->len + 2);
	if (value != 0xff) {
		LOGD("engpc->pc:%x %x %x %x %x %x %x %x %x %x %x",rsp[0],rsp[1],rsp[2],rsp[3],rsp[4],rsp[5],rsp[6],rsp[7],rsp[8],rsp[9],rsp[10]); //78 xx xx xx xx _ _38 _ _ 打印返回的10个数据
	} else {
		LOGD("engpc->pc:%x %x %x %x %x %x %x %x %x %x",rsp[0],rsp[1],rsp[2],rsp[3],rsp[4],rsp[5],rsp[6],rsp[7],rsp[8],rsp[9]); //78 xx xx xx xx _ _38 _ _ 打印返回的10个数据
	}
	FUN_EXIT ; //打印函数体名字
	return msg_head_ptr->len + 2;
	/*------------------------------如上虚线之间代码为通用代码，直接赋值即可---------*/
}

extern "C" void register_this_module(struct eng_callback * reg)
{
	LOGD("register_this_module : autotestotg");

	reg->type = 0x38;  // main cmd
	reg->subtype = 0x0C; // sub cmd
	reg->diag_ap_cmd = 0x5; // data cmd
	reg->eng_diag_func = testOtg;
}
