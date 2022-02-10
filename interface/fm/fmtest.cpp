#include <stdio.h>
#include "sprd_fts_type.h"
#include "sprd_fts_log.h"
#include "sprd_fts_diag.h"
#include "fm.h"
#define FM_START_FREQ 875 
unsigned char skd_fm_status_r = 0x02;//0: pass 1: fail
unsigned int skd_fm_freq_r = 0;
int skd_fm_rssi_r = 0;
int bbat_fm_rssi_r = 0;
//int testFM(const uchar * data, int data_len, uchar * rsp, int rsp_size)
int testFM(char *buf, int len, char *rsp, int rsplen)
{
	 int ret = 0;  //���ڱ�ʾ������Ժ�������ֵ��
	/****************************************************************
    ret <0,��ʾ��������ʧ�ܡ�
	ret =0,��ʾ����OK�����践�����ݡ�
	ret >0,��ʾ����OK����ȡret�����ݣ���ʱ��ret�Ĵ�С��ʾdata�б�������ݵĸ�����
	***********************************************************************/
/***
FM	PC-->engpc:7E 4B 00 00 00 0C 00 38 0D 01 00 04 38 7E	FM��	��ɫ�ֶα�ʾƵ����Ϣ��04 38��ʾ108.0MHz��FMҪ���źŴӶ��������������
	engpc-->pc:7E 00 00 00 00 08 00 38 00 7E 		
	PC-->engpc:7E 4A 00 00 00 09 00 38 0D 02 7E	FM�ر�	�ر�FM����
	engpc-->pc:7E 00 00 00 00 08 00 38 00 7E 		
***/
	char data[1014] = {0 }; //���ڱ����ȡ�������ݡ�

	FUN_ENTER ; //��ӡ���������֣�testGpio ++
	//��ӡPC�·������ݡ�
	LOGD("pc->engpc:%d number--> %x %x %x %x %x %x %x %x %x %x %x ",len,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10]); //78 xx xx xx xx _ _38 _ _ ��ӡ���ص�10������	

    switch( buf[9] ) {
	case 1:
	    {
		uint freq = ((buf[10] << 16) | (buf[11] << 8) | (buf[12] << 0)); //899

		//for android 6 and upon platform Using google original apk  or marlin
#if ((defined( GOOGLE_FM_INCLUDED) || defined (SPRD_WCNBT_MARLIN) )&&(!defined(BOARD_HAVE_FM_BCM)))
		if( Radio_Open(freq) < 0 || Radio_Play(freq) < 0 ){
		    ret = -1;
		    bbat_fm_rssi_r = 0;
		    LOGD("Radio_Open or Radio_Play the fm open failll!!!\n");
		}else{
		    ret =  Radio_GetRssi(&bbat_fm_rssi_r);
		    if(0!=ret){
			ret = -1;
			LOGD("Radio_Open or Radio_Play the fm open failll!!!\n");
		    }else{
			ret = 4;
			data[0]  = bbat_fm_rssi_r >> 24;
			data[1]  = (bbat_fm_rssi_r >> 16)&0xFF;
			data[2]  = (bbat_fm_rssi_r >> 8)&0xFF;
			data[3]  = (bbat_fm_rssi_r >> 0)&0xFF;
			LOGD("-----------data[0] = %d data[1] = %d data[2]= %d data[3] = %d  \n", data[0], data[1], data[2], data[3]);
		}

		}
#else
#ifdef SPRD_WCNBT_SR2351 //2351
		if( fmOpen() < 0 || fmPlay(freq) < 0 ){ ret = -1;}
#else
#ifdef BOARD_HAVE_FM_BCM //BCM

		if( Bcm_fmOpen() < 0 || Bcm_fmPlay(freq,&bbat_fm_rssi_r) < 0 ){
		    //ret = 5;
		    ret = -1;
		LOGD("the fm open failll!!!\n");
		}else{
		    ret = 4;
		    //(unsigned char &)bbat_fm_rssi_r;
		    data[0]  = bbat_fm_rssi_r >> 24;
		    data[1]  = (bbat_fm_rssi_r >> 16)&0xFF;
		    data[2]  = (bbat_fm_rssi_r >> 8)&0xFF;
		    data[3]  = (bbat_fm_rssi_r >> 0)&0xFF;

		    LOGD("-----------data[0] = %d data[1] = %d data[2]= %d data[3] = %d  \n", data[0], data[1], data[2], data[3]);
		}


#endif
#endif
#endif
	    }
	    break;
	case 2:
	    //for android 6 and upon platform Using google original apk marlin
#if ((defined( GOOGLE_FM_INCLUDED) || defined (SPRD_WCNBT_MARLIN))&&(!defined(BOARD_HAVE_FM_BCM)))
	    if( Radio_Close() < 0 )
		ret = -1;
#endif

#ifdef SPRD_WCNBT_SR2351 //2351
	    fmStop();
	    fmClose();
#endif

#ifdef BOARD_HAVE_FM_BCM //bcm
	    Bcm_fmStop();
	    Bcm_fmClose();
#endif
	    break;
	case 0x20:
	    {
		skd_fm_freq_r = FM_START_FREQ;
		ret = Radio_SKD_Test(&skd_fm_status_r, &skd_fm_freq_r, &skd_fm_rssi_r);
		LOGD("status = %d, freq = %d, rssi = %d\n",
			skd_fm_status_r, skd_fm_freq_r, skd_fm_rssi_r);
	    }
	    break;
	default:
	    break;
    }


/*------------------------------��������Ϊͨ�ô��룬����ģ�����ֱ�Ӹ��ƣ�------------------------------------------*/

	//���Э��ͷ7E xx xx xx xx 08 00 38
	MSG_HEAD_T *msg_head_ptr;
	memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T)-1);  //����7E xx xx xx xx 08 00 38��rsp,����1����Ϊ����ֵ�в�����MSG_HEAD_T��subtype��38��������ݼ�Ϊsubtype��
	msg_head_ptr = (MSG_HEAD_T *)(rsp + 1);

	//�޸ķ������ݵĳ��ȣ����ϸ��Ƶ����ݳ��ȵ���PC���͸�engpc�����ݳ��ȣ�������Ҫ�ĳ�engpc���ظ�pc�����ݳ��ȡ�
	msg_head_ptr->len = 8; //���ص���������ݸ�ʽ��7E xx xx xx xx 08 00 38 xx 7E��ȥ��ͷ��β��7E����8�����ݡ������Ҫ��������,��ֱ����38 XX���油��


	//���ɹ���ʧ�ܱ�־λrsp[1 + sizeof(MSG_HEAD_T)]�����ret>0,�������䷵�ص����ݡ�
	LOGD("msg_head_ptr,ret=%d",ret);

	if(ret<0)
	{
	rsp[sizeof(MSG_HEAD_T)] = 1;    //38 01 ��ʾ����ʧ�ܡ�
	}else if (ret==0){
	rsp[sizeof(MSG_HEAD_T)] = 0;    //38 00 ��ʾ����ok
	}else if(ret >0)
	{
	rsp[sizeof(MSG_HEAD_T)] = 0;    //38 00 ��ʾ����ok,ret >0,��ʾ��Ҫ���� ret���ֽ����ݡ�
    memcpy(rsp + 1 + sizeof(MSG_HEAD_T), data, ret);	  //����ȡ����ret�����ݣ����Ƶ�38 00 ����
	msg_head_ptr->len+=ret;   //���س��ȣ��������ݳ���8+��ȡ����ret���ֽ����ݳ��ȡ�
	}
	LOGD("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d",sizeof(MSG_HEAD_T),rsp[sizeof(MSG_HEAD_T)]);
	//���Э��β����7E
    rsp[msg_head_ptr->len + 2 - 1]=0x7E;  //��������β��־
    LOGD("dylib test :return len:%d",msg_head_ptr->len + 2);
	LOGD("engpc->pc:%x %x %x %x %x %x %x %x %x %x",rsp[0],rsp[1],rsp[2],rsp[3],rsp[4],rsp[5],rsp[6],rsp[7],rsp[8],rsp[9]); //78 xx xx xx xx _ _38 _ _ ��ӡ���ص�10������
	FUN_EXIT ; //��ӡ����������
   return msg_head_ptr->len + 2;
/*------------------------------��������֮�����Ϊͨ�ô��룬ֱ�Ӹ�ֵ����---------*/	
}

extern "C" void register_this_module_ext(struct eng_callback *reg, int *num)
{
int moudles_num = 0;   //���ڱ�ʾע��Ľڵ����Ŀ��
LOGD("register_this_module :dllibtest");

    //1st command
   reg->type = 0x38;  //38��ӦBBAT����
   reg->subtype = 0x0D;   //0x0D��ӦBBAT�е�FM���ԡ�
   reg->eng_diag_func = testFM; //testGpio��ӦBBAT��GPIO���Թ��ܺ��������ϡ�
   moudles_num++;

   /*************
   //2st command
   reg->type = 0x39;  //39��Ӧnativemmi����
   reg->subtype = 0x07;   //0x07��Ӧnativemmi�е�GPIO���ԡ�
   reg->eng_diag_func = testnativeGpio;
   moudles_num++;
   **********************/

   *num = moudles_num;
LOGD("register_this_module_ext: %d - %d",*num, moudles_num);
}