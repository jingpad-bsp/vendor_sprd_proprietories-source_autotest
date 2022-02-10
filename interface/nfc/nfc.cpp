#include "nfc_interface.h"

int openNfc() {
    int ret = -1;
    int hal_reset_nfc = 0;
    ALOGD("hw_get_module....");
    ret = hw_get_module (NFC_NCI_HARDWARE_MODULE_ID, &hw_module);
    if(ret == 0) {
        ALOGD("open nfc module OK\n");
        ret = nfc_nci_open(hw_module, &nfc_device);
        if(ret != 0) {
            ALOGD("nfc nci open fail\n");
            ret = -1;
            return ret;
        }
    } else {
        ALOGE("hw_get_module fail\n");
        return ret;
    }
    ALOGD("open nfc device...\n");
    nfc_device->open (nfc_device, hal_context_callback, hal_context_data_callback);
 
    max_try = 0;
    last_event = HAL_NFC_ERROR_EVT;
    last_evt_status = HAL_NFC_STATUS_FAILED;
    last_recv_data_len = 0;
    memset(last_rx_buff, 0, sizeof(last_rx_buff));
    while(last_event != HAL_NFC_OPEN_CPLT_EVT && last_recv_data_len != 6  ){
        OSI_delay(100);
        if(max_try ++ > 150){//FW will be downloaded if not forced, might take up to 15 seconds?
            ALOGD("open device timeout!\n");
            nfc_nci_close(nfc_device);
            ret = -1;
            return ret;
        }
    }

    if(last_recv_data_len == 6) { //No FW was downloaded, HAL sent reset but did not report HAL_NFC_OPEN_CPLT_EVT
        if(last_rx_buff[0] == 0x40 &&
            last_rx_buff[1] == 0x00 &&
            last_rx_buff[2] == 0x03 &&
            last_rx_buff[3] == 0x00 &&
            last_rx_buff[5] == 0x01) {
            ALOGD("HAL reset nfc device during open.\n");
            hal_reset_nfc = 1;
            ret = 0;
        } else {
            nfc_nci_close(nfc_device);
            ALOGD("HAL reported error reset response!\n");
            ret = -1;
            return ret;
        }
    } else if(last_evt_status == HAL_NFC_STATUS_OK) {
        ALOGD("HAL reported open completed event!\n");
    } else {
        nfc_nci_close(nfc_device);
        ALOGD("open device failed\n");
        ret = -1;
        return ret;
    }
    ALOGD("nfc open OK");

    //reset device
    if(hal_reset_nfc == 0){
        ALOGD("reset nfc device...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_core_reset_cmd), nci_core_reset_cmd);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 500){
                ALOGD("reset device timeout\n");
                break;
            }
        }

    if(last_recv_data_len == 6 &&
            last_rx_buff[0] == 0x40 &&
            last_rx_buff[1] == 0x00 &&
            last_rx_buff[2] == 0x03 &&
            last_rx_buff[3] == 0x00 &&
            last_rx_buff[5] == 0x01) {
                ALOGD("reset nfc device OK \n");
                ret = 0;
        } else {
                ALOGD("failed to reset nfc device.\n");
                ret = -1;
        }
    }

    //Initialize device
    if(ret == 0) {
        ALOGD("Initialize nfc device...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_initializ_cmd), nci_initializ_cmd);
        while(last_recv_data_len == 0) {
            OSI_delay(10);
            if(max_try ++ > 500){
                ALOGD("initiaze device timeout\n");
                break;
            }
        }

        if(last_recv_data_len == 23 &&
            last_rx_buff[0] == 0x40 &&
            last_rx_buff[1] == 0x01 &&
            last_rx_buff[2] == 0x14 &&
            last_rx_buff[3] == 0x00) {
            ALOGD("initiaze nfc device OK\n");
            ret = 0;
        } else {
            ALOGD("failed to initialze nfc device.\n");
            ret = -1;
        }
    }

    if(ret == 0) {
        uint8_t major_ver = (uint8_t)((last_rx_buff[19]>>4)&0x0f);
        uint8_t minor_ver = (uint8_t)((last_rx_buff[19])&0x0f);
        uint16_t build_info_high = (uint16_t)(last_rx_buff[20]);
        uint16_t build_info_low = (uint16_t)(last_rx_buff[21]);
        ALOGD("F/W version: %d.%d.%d\n", major_ver, minor_ver, ((build_info_high*0x100)+build_info_low));

        //Nofity HAL for device being initialized
        ALOGD("notifiy nfc device initialized...\n");
        nfc_device->core_initialized (nfc_device, last_rx_buff);
        max_try = 0;
        last_event = HAL_NFC_ERROR_EVT;
        last_evt_status = HAL_NFC_STATUS_FAILED;
        while(last_event != HAL_NFC_POST_INIT_CPLT_EVT) {
            OSI_delay(100);
            if(max_try ++ > 150){//FW will be downloaded if not exit or newer, might take up to 15 seconds?
                ALOGD("notify device timeout\n");
                break;
            }
        }
        if(last_evt_status == HAL_NFC_STATUS_OK){
            ALOGD("notify OK\n");
        } else {
            ALOGD("notify fail\n");
            ret = -1;
        }
    }
    return ret;
}

int checkTag() {
    int ret = 0;
    //Set nfc device to poll
    ALOGD("discover map...\n");
    max_try = 0;
    last_recv_data_len = 0;
    memset(last_rx_buff, 0, sizeof(last_rx_buff));
    nfc_device->write(nfc_device, sizeof(discover_map_cmd), discover_map_cmd);
    while(last_recv_data_len == 0){
        OSI_delay(10);
        if(max_try ++ > 500){
            ALOGD("discover map timeout\n");
            break;
        }
    }
    if(last_recv_data_len == 4 &&
        last_rx_buff[0] == 0x41 &&
        last_rx_buff[1] == 0x00 &&
        last_rx_buff[2] == 0x01 &&
        last_rx_buff[3] == 0x00) {
        ALOGD("discover map OK\n");
        ret = 0;
    } else {
        ALOGD("discover map failed.\n");
        ret = -1;
    }

    if(ret == 0) {
        ALOGD("rf discover...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(rf_discover_cmd_all), rf_discover_cmd_all);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 100){
                ALOGD("discover cmd timeout\n");
                break;
            }
        }
        if(last_recv_data_len == 4 &&
            last_rx_buff[0] == 0x41 &&
            last_rx_buff[1] == 0x03 &&
            last_rx_buff[2] == 0x01 &&
            last_rx_buff[3] == 0x00) {
            ALOGD("rf discover...OK\n");
            ret = 0;
        } else {
            ALOGD("failed to set nfc to poll.\n");
            ret = -1;
        }
    }

    //Ready to detect the tags from antenna
    if(ret == 0) {
        ALOGD("nfc is polling, please put a type 1/2/4 tag to the antenna...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));

        while(last_recv_data_len == 0){
            OSI_delay(100);
            if(max_try ++ > 30){ //Wait max 30 seconds
                ALOGD("waiting tag timeout\n");
                break;
            }
        }
        if(last_recv_data_len >= 10 &&
            last_rx_buff[0] == 0x61 &&
            last_rx_buff[1] == 0x05 &&
            last_rx_buff[4] == 0x02 &&
            last_rx_buff[5] == 0x04) {
            ALOGD("Type 4 Tag detected.\n");
            ret = 0;
        } else if(last_recv_data_len >= 10 &&
                last_rx_buff[0] == 0x61 &&
                last_rx_buff[1] == 0x05 &&
                last_rx_buff[4] == 0x01 &&
                last_rx_buff[5] == 0x01) {
            ALOGD("Type 1 Tag detected.\n");
            ret = 0;
        } else if(last_recv_data_len >= 10 &&
                last_rx_buff[0] == 0x61 &&
                last_rx_buff[1] == 0x05 &&
                last_rx_buff[4] == 0x01 &&
                last_rx_buff[5] == 0x02) {
            ALOGD("Type 2 Tag detected.\n");
            ret = 0;
        } else {
            ALOGD("failed to detect known tags.\n");
            ret = -1;
        }
    }
    return ret;
}

int closeNfc() {
    int ret = 0;
    ALOGD("close nfc device\n");
    ret = nfc_device->close (nfc_device);
    ret = nfc_nci_close(nfc_device);
    return ret;
}

void OSI_delay(uint32_t timeout) {
    struct timespec delay;
    int err;

    delay.tv_sec = timeout / 1000;
    delay.tv_nsec = 1000 * 1000 *(timeout % 1000);

    do {
        err = nanosleep(&delay, &delay);
    } while (err < 0 && errno == EINTR);
}



int testNfc(char *buf, int len, char *rsp, int rsplen) {
//FUN_ENTER;
    int ret = -1; //用于表示具体测试函数返回值
    /************************************************
    ret < 0, 表示函数返回失败
    ret = 0, 表示测试OK，无需返回数据
    ret > 0, 表示测试OK, 获取ret个数，此时，ret的大小表示data中保存的数据的个数
NFC
    pc-->engpc: 7E 9D 00 00 00 0A 00 38 15 0D 01 7E     打开NFC
    engpc-->pc: 7E 00 00 00 00 08 00 38 00 7E

    pc-->engpc: 7E 9D 00 00 00 0A 00 38 15 0D 02 7E     手机读取外部NFC标签信息
    engpc-->pc: 7E 00 00 00 00 09 00 38 00 xx 7E        读取到返回38 00 00 ，未读取到 返回 38 00 01

    pc-->engpc: 7E 9D 00 00 00 0A 00 38 15 0D 03 7E      关闭NFC
    engpc-->pc: 7E 00 00 00 00 08 00 38 00 7E
    ************************************************/

    //打印PC下发的数据
    ALOGD("pc->engpc:%d number--> %x %x %x %x %x %x %x %x %x %x %x %x",len,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11]);
    ALOGD("testNfc cmd = %x %x \n", buf[9],buf[10]);

    switch(buf[10]) {
        case NFC_OPEN: //open nfc
            if(openNfc() < 0) {
                ALOGE("testNfc: open nfc fail\n");
                ret = -1;
            } else {
                ret = 0;
            }
            break;
        case NFC_CHECKTAG: //check tag
            if(checkTag() < 0) {
                ALOGE("testNfc: check tag fail\n");
                ret = -1;
            } else {
                ret = 0;
            }
            break;
        case NFC_CLOSE:
            if(closeNfc() != 0) {
                ALOGE("testNfc: close nfc fail\n");
                ret = -1;
            } else {
                ret = 0;
            }
            break;
        default:
            break;
   }

    MSG_HEAD_T *msg_head_ptr;
    memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T) - 1);
    msg_head_ptr = (MSG_HEAD_T *)(rsp + 1);
    msg_head_ptr->len = 9;
    ALOGD("nfc msg_head_ptr,ret=%d", ret);
    rsp[sizeof(MSG_HEAD_T)] = 0;
    if (ret < 0) {
        rsp[sizeof(MSG_HEAD_T)+1] = 1; //38 01 表示测试失败
    } else if (ret ==0) {
        rsp[sizeof(MSG_HEAD_T)+1] = 0; //38 00表示测试OK
    }
    
    ALOGD("nfc test rsp[1 + sizeof(MSG_HEAD_T):%d]:%d", sizeof(MSG_HEAD_T), rsp[sizeof(MSG_HEAD_T)]);
    rsp[msg_head_ptr->len + 2 - 1] = 0x7E;  //加上数据尾标志
    ALOGD("nfc test :return len:%d",msg_head_ptr->len + 2);
    ALOGD("engpc->pc:%x %x %x %x %x %x %x %x %x %x %x",rsp[0],rsp[1],rsp[2],rsp[3],rsp[4],rsp[5],rsp[6],rsp[7],rsp[8],rsp[9],rsp[10]); //78 xx xx xx xx _ _38 _ _ 打印返回的10个数据
    return msg_head_ptr->len + 2;
}

void* nfc_init(void*) {
    int ret = -1;
    ret = openNfc();
    if(ret >= 0) {
        ALOGD("nfc_init, open success!");
        closeNfc();
    } else {
        ALOGE("nfc_init, open fail");
    }
    return NULL;
}

int testAutotest_nfc(char *buf, int len, char *rsp, int rsplen) {
    FUN_ENTER ;
    int ret = ENG_DIAG_RET_UNSUPPORT;
    ALOGD("buf[9] = 0x%02x\n", buf[9]);
    //0x0D is nfc test, Other functions will return ENG_DIAG_RET_UNSUPPORT
    if(buf[9] == 0x0D)
        ret = testNfc(buf, len, rsp, rsplen);
    return ret ;
}

extern "C" void register_this_module_ext(struct eng_callback *reg, int *num) {

    pthread_t nfc_init_thread;
    ALOGD("nfc init, download firmware in thread");
    pthread_create(&nfc_init_thread, NULL, nfc_init, NULL);
    int moudles_num = 0;
    ALOGD("register_this_module :nfc test");

    reg->type = 0x38;
    reg->subtype = 0x15;
    reg->eng_diag_func = testAutotest_nfc;

    moudles_num++;
    *num = moudles_num;
    ALOGD("register_this_module_ext: %d - %d",*num, moudles_num);
}
