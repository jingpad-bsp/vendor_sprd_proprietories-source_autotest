#include "interface.h"
#include "atci.h"
//int testSIM(const uchar * data, int data_len, uchar * rsp, int rsp_size)
int testSIM(char *buf, int len, char *rsp, int rsplen)
{
	int respLen = 0;
	char cmd[128];
	char resp[128];
    FUN_ENTER;
    LOGD("testSIM cmd = %d\n", buf[9]);
	
	 int ret = -1;  //用于表示具体测试函数返回值，
	/****************************************************************
    ret <0,表示函数返回失败。
	ret =0,表示测试OK，无需返回数据。
	ret >0,表示测试OK，获取ret个数据，此时，ret的大小表示data中保存的数据的个数。
	***********************************************************************/

	char data[1014] = {0 }; //用于保存获取到的数据。
	/*
SIM card  pc-->engpc:	7E xx xx xx xx 09 00 38 09 00 7E	读取SIM卡	最后一个09后面的00表示SIM卡1，如果是SIM卡2就用01表示
	      engpc-->pc:   7E 00 00 00 00 08 00 38 00 7E 		读不到卡就返回7E 00 00 00 00 08 00 38 01 7E 
	*/
	//打印PC下发的数据。
	LOGD("pc->engpc:%x %x %x %x %x %x %x %x %x %x %x",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10]); //78 xx xx xx xx _ _38 _ _ 打印返回的10个数据	
	memset(data, 0, sizeof(data));
	snprintf(cmd, sizeof(cmd),"AT+EUICC?");
	ret = sendATCmd(buf[9], cmd, data, sizeof(data));
	if(ret==-1)
	{
	LOGD("send the command fail\n");
	}
	else if (ret==0)
	{
		LOGD("data = %s", data);
		char * pval = strstr(data, "EUICC");
		LOGD("read OK: %s\n", data);
		if( NULL != pval ) {
		LOGD("pval=%s\n", pval);
		pval += 5;
                while( *pval && (*pval < '0' || *pval > '9') ) {
                    pval++;
                }
		LOGD("%s\n", pval);
		ret = ('0' == *pval) ? 0 : -1;
		}
		else{
		ret = -1;
		}
	}

/*------------------------------后续代码为通用代码，所有模块可以直接复制，------------------------------------------*/

	//填充协议头7E xx xx xx xx 08 00 38
	MSG_HEAD_T *msg_head_ptr;
	memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T)-1);  //复制7E xx xx xx xx 08 00 38至rsp,，减1是因为返回值中不包含MSG_HEAD_T的subtype（38后面的数据即为subtype）
	msg_head_ptr = (MSG_HEAD_T *)(rsp + 1);

	//修改返回数据的长度，如上复制的数据长度等于PC发送给engpc的数据长度，现在需要改成engpc返回给pc的数据长度。
	msg_head_ptr->len = 8; //返回的最基本数据格式：7E xx xx xx xx 08 00 38 xx 7E，去掉头和尾的7E，共8个数据。如果需要返回数据,则直接在38 XX后面补充


	//填充成功或失败标志位rsp[1 + sizeof(MSG_HEAD_T)]，如果ret>0,则继续填充返回的数据。
	LOGD("msg_head_ptr,ret=%d",ret);

	if(ret<0)
	{
	rsp[sizeof(MSG_HEAD_T)] = 1;    //38 01 表示测试失败。
	}else if (ret==0){
	rsp[sizeof(MSG_HEAD_T)] = 0;    //38 00 表示测试ok
	}else if(ret >0)
	{
	rsp[sizeof(MSG_HEAD_T)] = 0;    //38 00 表示测试ok,ret >0,表示需要返回 ret个字节数据。
    memcpy(rsp + 1 + sizeof(MSG_HEAD_T), data, ret);	  //将获取到的ret个数据，复制到38 00 后面
	msg_head_ptr->len+=ret;   //返回长度：基本数据长度8+获取到的ret个字节数据长度。
	}
	LOGD("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d",sizeof(MSG_HEAD_T),rsp[sizeof(MSG_HEAD_T)]);
	//填充协议尾部的7E
    rsp[msg_head_ptr->len + 2 - 1]=0x7E;  //加上数据尾标志
    LOGD("dylib test :return len:%d",msg_head_ptr->len + 2);
	LOGD("engpc->pc:%x %x %x %x %x %x %x %x %x %x",rsp[0],rsp[1],rsp[2],rsp[3],rsp[4],rsp[5],rsp[6],rsp[7],rsp[8],rsp[9]); //78 xx xx xx xx _ _38 _ _ 打印返回的10个数据
	FUN_EXIT ; //打印函数体名字
   return msg_head_ptr->len + 2;
/*------------------------------如上虚线之间代码为通用代码，直接赋值即可---------*/	
	
}

extern "C" void register_this_module_ext(struct eng_callback *reg, int *num)
{
int moudles_num = 0;   //用于表示注册的节点的数目。
LOGD("register_this_module :dllibtest");

    //1st command
   reg->type = 0x38;  //38对应BBAT测试
   reg->subtype = 0x09;   //0x09对应BBAT中的SIM测试。
   reg->eng_diag_func = testSIM; //testGpio对应BBAT中GPIO测试功能函数，见上。
   moudles_num++;

   /*************
   //2st command
   reg->type = 0x39;  //39对应nativemmi测试
   reg->subtype = 0x07;   //0x07对应nativemmi中的GPIO测试。
   reg->eng_diag_func = testnativeGpio;
   moudles_num++;
   **********************/

   *num = moudles_num;
LOGD("register_this_module_ext: %d - %d",*num, moudles_num);
}