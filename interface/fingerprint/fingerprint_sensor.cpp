#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <hardware/hardware.h>
#include <cutils/log.h>
#include "sprd_fts_type.h"
#include "sprd_fts_log.h"

#define FINGERPRINT_LIB_Default "libfactorylib.so"             //default fingerprint lib path
#define LOG_TAG "BBAT_FINGER"


static const char *fp_lib[] = {
    "libgf_factory_test.so",
    "libfactorylib_silead.so",
};

static bool is_goodix = false;

static const int FP_LIB_COUNT =
    (sizeof(fp_lib)/sizeof(fp_lib[0]));

int testAutotestFingerprint(char *buf, int len, char *rsp, int rsplen) {

    FUN_ENTER ;
    int ret = -1;
    void *handle;
    typedef int (*CAC_FUNC)(void);
    ALOGD("buf[9] = 0x%02x\n", buf[9]);
    if(0x0C != buf[9]){
        ALOGD("not fingerprint test cmd,just return");
        ret = ENG_DIAG_RET_UNSUPPORT;
        return ret;
    }

	is_goodix = false;
	ALOGD("is_goodix111: %d", is_goodix);

	FILE* goodixFp = fopen("/sys/devices/platform/soc/soc:ap-apb/70800000.spi/spi_master/spi0/spi0.0/debug/debug", "r");
	if(goodixFp) {
		is_goodix = true;
		fclose(goodixFp);
	} 
    ALOGD("is_goodix222: %d", is_goodix);


    for(int i = 0; i < FP_LIB_COUNT; i++) {
        ALOGD("dlopen fingerprint lib: %s", fp_lib[i]);
        handle = dlopen(fp_lib[i], RTLD_LAZY);
        if (!handle)
        {
            ALOGD("fingersor lib dlopen failed! %s, exist fingerprint test!\n", dlerror());
            ret = -1;
            continue;
        }
        CAC_FUNC factory_init = NULL;
        CAC_FUNC spi_test = NULL;
        CAC_FUNC factory_exit = NULL;

        ALOGD("do factory_init");

		if(is_goodix) {
	        factory_init = (CAC_FUNC)dlsym(handle, "gf_factory_test");
		} else {
	        factory_init = (CAC_FUNC)dlsym(handle, "factory_init");
		}

        if(!factory_init)
        {
            ALOGD("could not find symbol 'factory_init', %d IN\n", __LINE__);
            ret = -1;
            continue;
        }
        else
        {
            ret = (*factory_init)();
            if (ret != 0) {
                ALOGD("factory_init fail, ret = %d\n", ret);
                continue;
            }
        }

        ALOGD("do spi_test ret = %d\n", ret);
		if(is_goodix) {
			//return ret;
			goto exit;
		}

        spi_test = (CAC_FUNC)dlsym(handle, "spi_test");
        if(!spi_test)
        {
            ALOGD("could not find symbol 'spi_test', %d IN\n", __LINE__);
            ret = -1;
            continue;
        }
        else
        {
            ret = (*spi_test)();
            if (ret != 0) {
                ALOGD("spi_test fail, ret = %d\n", ret);
                continue;
            }
        }

        ALOGD("do factory_exit");
        factory_exit = (CAC_FUNC)dlsym(handle, "factory_exit");
        if(!factory_exit)
        {
            ALOGD("could not find symbol 'factory_exit', %d IN\n", __LINE__);
            ret = -1;
            continue;
        }
        else
        {
            ret = (*factory_exit)();
            if (ret != 0) {
            ALOGD("factory_exit fail, ret = %d\n", ret);
            continue;
            }
        }
    }


exit:
    MSG_HEAD_T *msg_head_ptr;
    int value = 1;
    memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T)-1);
    msg_head_ptr = (MSG_HEAD_T *)(rsp + 1);
    msg_head_ptr->len = 8;
    ALOGD("msg_head_ptr,ret=%d",ret);
    if(ret<0)
    {
        rsp[sizeof(MSG_HEAD_T)] = 0;
        memcpy(rsp + 1 + sizeof(MSG_HEAD_T), &value, 1);
        msg_head_ptr->len+=1;
     }else if (ret==0){
        value = 0;
        rsp[sizeof(MSG_HEAD_T)] = 0;
        memcpy(rsp + 1 + sizeof(MSG_HEAD_T), &value, 1);
        msg_head_ptr->len+=1;
    }else if(ret >0){
        rsp[sizeof(MSG_HEAD_T)] = 0;
        memcpy(rsp + 1 + sizeof(MSG_HEAD_T), &value, 1);
        msg_head_ptr->len+=1;
    }
    ALOGD("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d",sizeof(MSG_HEAD_T),rsp[sizeof(MSG_HEAD_T)]);

    rsp[msg_head_ptr->len + 2 - 1]=0x7E;
    ALOGD("dllib_fp_nfc test :return len:%d",msg_head_ptr->len + 2);
    ALOGD("engpc->pc:%x %x %x %x %x %x %x %x %x %x",rsp[0],rsp[1],rsp[2],rsp[3],rsp[4],rsp[5],rsp[6],rsp[7],rsp[8],rsp[9]);
    FUN_EXIT ;
    return msg_head_ptr->len + 2;
}


extern "C" void register_this_module_ext(struct eng_callback *reg, int *num) {

    int moudles_num = 0;
    ALOGD("register_this_module :fingerprint test");

    reg->type = 0x38;  //38 bbat
    reg->subtype = 0x15;   //fingerprint & nfc test
    reg->eng_diag_func = testAutotestFingerprint; //功能测试回调

    moudles_num++;
    *num = moudles_num;
    ALOGD("register_this_module_ext: %d - %d",*num, moudles_num);
}
