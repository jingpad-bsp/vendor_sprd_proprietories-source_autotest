#include <stdio.h>
#include "sprd_fts_type.h"
#include "sprd_fts_log.h"
#include "eng_attok.h"
#include "fingerprint_sensor_mmi.h"

void *handle;

typedef int (*CAC_FUNC)(void);

CAC_FUNC factory_init = NULL;
CAC_FUNC spi_test = NULL;
CAC_FUNC interrupt_test = NULL;
CAC_FUNC deadpixel_test = NULL;
CAC_FUNC finger_detect = NULL;
CAC_FUNC factory_exit = NULL;

static const char *fp_lib[] = {
    "libfactorylib.so",
};

static const int FP_LIB_COUNT =
    (sizeof(fp_lib)/sizeof(fp_lib[0]));

static int lib_count = 0;

enum fp_test_step
{
    FACTORY_INIT,
    SPI_TEST,
    INTERRRUPT_TEST,
    DEADPIXEL_TEST,
    FINGER_DETECT,
    FACTORY_EXIT,
    STEP_NUMBER,
};

int fp_factory_init(){
    int ret = -1;
    ALOGD("do factory_init lib_count = %d, FP_LIB_COUNT = %d, fp_lib[lib_count] = %s\n", lib_count, FP_LIB_COUNT, fp_lib[lib_count]);
    if(lib_count < FP_LIB_COUNT){
        handle = dlopen(fp_lib[lib_count], RTLD_LAZY);
        if (!handle) {
            ALOGD("could not find lib %s \n", fp_lib[lib_count]);
        }else{
            factory_init = (CAC_FUNC)dlsym(handle, "factory_init");
            if(!factory_init)
            {
                ALOGD("could not find symbol 'factory_init', %d IN\n", __LINE__);
            }
            else
            {
                ret = (*factory_init)();
                if (ret != 0) {
                    ALOGD("factory_init fail, ret = %d\n", ret);
                    ret = -1;
                }
            }
        }

    }
    if (handle && ret == -1){
        ALOGD("factory_init fail, close handle");
        dlclose (handle);
    }
    lib_count++;
    return ret;
}

int fp_spi_test(){
    int ret = -1;
    spi_test = (CAC_FUNC)dlsym(handle, "spi_test");
    ALOGD("do spi_test");
    if(!spi_test)
    {
        ALOGD("could not find symbol 'spi_test', %d IN\n", __LINE__);
    }
    else
    {
        ret = (*spi_test)();
        if (ret != 0) {
            ALOGD("spi_test fail, ret = %d\n", ret);
            ret = -1;
        }
    }
    return ret;
}
int fp_deadpixel_test(){
    int ret = -1;
    deadpixel_test = (CAC_FUNC)dlsym(handle, "deadpixel_test");
    ALOGD("do deadpixel_test");
    if(!deadpixel_test)
    {
        ALOGD("could not find symbol 'deadpixel_test', %d IN\n", __LINE__);
    }
    else
    {
        ret = (*deadpixel_test)();
        if (ret != 0) {
            ALOGD("deadpixel_test fail, ret = %d\n", ret);
            ret = -1;
        }
    }
    return ret;
}
int fp_interrupt_test(){
    int ret = -1;
    interrupt_test = (CAC_FUNC)dlsym(handle, "interrupt_test");
    ALOGD("do interrupt_test");
    if(!interrupt_test)
    {
        ALOGD("could not find symbol 'interrupt_test', %d IN\n", __LINE__);
    }
    else
    {
        ret = (*interrupt_test)();
        if (ret != 0) {
            ALOGD("interrupt_test fail, ret = %d\n", ret);
            ret = -1;
        }
    }
    return ret;
}
int fp_finger_detect(){
    int ret = -1;
    finger_detect = (CAC_FUNC)dlsym(handle, "finger_detect");
    ALOGD("do finger_detect");
    if(!finger_detect)
    {
        ALOGD("could not find symbol 'finger_detect', %d IN\n", __LINE__);
    }
    else
    {
        ret = (*finger_detect)();
        if (ret != 0) {
            ALOGD("finger_detect fail, ret = %d\n", ret);
            ret = -1;
        }
    }
    return ret;
}
int fp_factory_exit(){
    int ret = -1;
    factory_exit = (CAC_FUNC)dlsym(handle, "factory_exit");
    ALOGD("do factory_exit");
    if(!factory_exit)
    {
        ALOGD("could not find symbol 'factory_exit', %d IN\n", __LINE__);
    }
    else
    {
        ret = (*factory_exit)();
        if (ret != 0) {
            ALOGD("factory_exit fail, ret = %d\n", ret);
            ret = -1;
        }
    }
    return ret;
}

int fp_test_fun(int cmd_type){
    int ret = -1;
    ALOGD("fp_test_fun : cmd_type:%d", cmd_type);
    switch(cmd_type){
        case FACTORY_INIT:
            ret = fp_factory_init();
            break;
        case SPI_TEST:
            ret = fp_spi_test();
            break;
        case INTERRRUPT_TEST:
            ret = fp_interrupt_test();
            break;
        case DEADPIXEL_TEST:
            ret = fp_deadpixel_test();
            break;
        case FINGER_DETECT:
            ret = fp_finger_detect();
            break;
        case FACTORY_EXIT:
            ret = fp_factory_exit();
            break;
        default:
            break;
    }
    ALOGD("fp_test_fun return, ret = %d\n", ret);
    return ret;
}

int fp_test(int cmd_type) {

    int ret = -1;
    ret = fp_test_fun(cmd_type);
//若是前一个库fail，应走到此处，此处将会遍历下一个库
next:
    if(cmd_type < INTERRRUPT_TEST && ret == -1 && lib_count < FP_LIB_COUNT){//cmd_type>=INTERRRUPT_TEST,means spi_test pass(read chipid ok).
        ALOGD("fp_test : try next lib, lib_count:%d", lib_count);
        for(int i = lib_count; i < FP_LIB_COUNT; i++){
            for(int j = 0; j <= cmd_type; j++){
                ret = fp_test_fun(j);
                if(ret == -1){//test fail,should break and try next lib
                    ALOGD("fp_test : try next lib fail,should break， j:%d", j);
                    break;
                }
            }
            if(ret != -1){//test pass,should break and return
                ALOGD("fp_test : try next lib sucess, ret:%d", ret);
                break;
            }

        }
    }

exit:
    ALOGD("fp_test return, ret = %d\n", ret);
    return ret;
}

int testMmiFingerprint(char *req, char *rsp)
{
    int cmd_type = 0;
    char *ptr = NULL;
    char *ptr_head = NULL;
    int ret = -1;

    if (NULL == req)
    {
        ALOGD("testMmiFingerprint:%s,null pointer", __FUNCTION__);
        sprintf(rsp, "\r\nERROR\r\n");
        return -1;
    }

    ptr = ptr_head = strdup(req) ;

    ALOGD("testMmiFingerprint:%s", ptr);
    ret = at_tok_equel_start(&ptr);
    ret |= at_tok_nextint(&ptr, &cmd_type);
    if(ret != 0){
        ALOGD("testMmiFingerprint :error cmd_type:%d", cmd_type);
        sprintf(rsp, "\r\nERROR\r\n");
        if (ptr_head) {
            free(ptr_head);
        }
        return -1;
    }
    if(cmd_type == FACTORY_INIT){
        lib_count = 0;
    }
    if (ptr_head) {
        free(ptr_head);
    }
    return fp_test(cmd_type);
}


extern "C" void register_this_module_ext(struct eng_callback *reg, int *num) {

    int moudles_num = 0;
    ALOGD("register_this_module :fingerprint mmi test");

    sprintf((reg + moudles_num)->at_cmd, "%s", "AT+FINGERTEST");
    (reg + moudles_num)->eng_linuxcmd_func = testMmiFingerprint;//功能测试回调

    moudles_num++;
    *num = moudles_num;
    ALOGD("register_this_module_ext: %d - %d",*num, moudles_num);
}
