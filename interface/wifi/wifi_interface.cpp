#include "wifi_interface.h"
#include "wifinew.h"
#include <sys/wait.h>

#ifndef WIFI_DRIVER_MODULE_PATH
#define  WIFI_DRIVER_MODULE_PATH  "/mnt/product/socko/sprdwl_ng.ko"
#endif

const char *get_cap_ch = "IFNAME=wlan0 GET_CAPABILITY channels";
const char *remove_network = "IFNAME=wlan0 REMOVE_NETWORK all";
const char *scan = "IFNAME=wlan0 SCAN";
const char *scan_result = "IFNAME=wlan0 SCAN_RESULTS";
const char *terminate = "IFNAME=wlan0 TERMINATE";
const char *results_event = "CTRL-EVENT-SCAN-RESULTS";

const int buffer_len = 4096;
static char wifiinfo[20][200]={0};
static int line = 0;
int wifi5G_flag = 0;

//------------------------------------------------------------------------------
int wifiOpen(void) {
FUN_ENTER;
    //load driver
    int ret = 0;
    int flag=-1;
    char cmd[100] = {0};

    sprintf(cmd, "insmod %s", WIFI_DRIVER_MODULE_PATH);
    flag = system(cmd);
    if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
      LOGE("%s:insmod sprdwl_ng.ko, cmd: %s. flag=%d,err:%s\n",
              __FUNCTION__, cmd, flag, strerror(errno));
      return -1;
    }
    LOGD("load wifi driver success\n");
    ret = wifi_start_supplicant(0);
    if (ret < 0) {
        LOGE("wifi_start_supplicant failed\n");
    }
    sleep(2);
    wifi_connect_to_supplicant();
FUN_EXIT;
    return 0;
}

//------------------------------------------------------------------------------
int wifi_ScanAP( void ) {
FUN_ENTER;
    int ret = 0;
    static char buffer[buffer_len] = {0};
    unsigned int len = buffer_len;
    char delims1[] = "\n";
    char *result1 = NULL;
    int ii = 0;
    line = 0;

   /* 1. Get channels capability */
    ret = wifi_command(get_cap_ch, buffer, &len);
    if (ret < 0) {
        LOGE("wifi_command - get_cap_ch failed ret= %d\n", ret);
    } else {
        buffer[len] = '\0';
        LOGD("wifi_command - get_cap_ch, buffer0 = %s,len=%d\n", buffer, len);
        if (NULL != strstr(buffer, "Mode[A]")) {
            wifi5G_flag = 1;
        }
        LOGD("wifi5G_flag=%d\n",wifi5G_flag);
    }

     /* 2. Remove all network */
    memset(buffer, 0, buffer_len);
    len = buffer_len;
    ret = wifi_command(remove_network, buffer, &len);
    if (ret < 0) {
        LOGE("wifi_command - remove_network failed ret= %d\n", ret);
    } else {
        buffer[len] = '\0';
        LOGD("wifi_command - remove_network, buffer1 = %s,len=%d\n", buffer, len);
    }

    /* 3. Start to scan */
    memset(buffer, 0, buffer_len);
    len = buffer_len;
    ret = wifi_command(scan, buffer, &len);
    if (ret < 0) {
        LOGE("wifi_command - scan failed ret= %d\n", ret);
        return ret;  // avoid waiting scan results_event
    } else {
        buffer[len] = '\0';
        LOGD("wifi_command - scan, buffer2 = %s,len=%d\n", buffer, len);
    }

    /* 4. Wait for scan results event */
    ret = wifi_ctrl_recv_event(buffer, buffer_len, &len, results_event, 10000);
    if (ret < 0) {
        LOGE("wifi_ctrl_recv_event fail,  ret=%d\n", ret);
    } else {
        /* 5. Get scan results */
        memset(buffer, 0, buffer_len);
        len = buffer_len;
        ret = wifi_command(scan_result, buffer, &len);
        if (ret < 0) {
            LOGE("wifi_command  - scan_result failed, ret= %d\n", ret);
        } else {
            buffer[len] = '\0';
            LOGD("wifi_command - scan_result, buffer3 = %s,len=%d\n", buffer, len);
            result1 = strtok(buffer, delims1);
            while (result1 != NULL ) {
                LOGD( "line = %d ,result is \"%s\"\n", line, result1 );
                line++;
                if ((line >1) && (line < sizeof(wifiinfo) / sizeof(wifiinfo[0]))) {
                    strcpy(wifiinfo[ii++], result1);
                    LOGD( "wifiinfo[%d]=%s\n", ii - 1, wifiinfo[ii - 1]);
                }
                result1 = strtok(NULL, delims1);
            }
        }
    }

    wifi_parse_result();
FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
int wifi_parse_result(void) {
    int ret = 0;
    int tbline=0;
    char delims2[] = "\t";
    char *result2 = NULL;

    wifinum = WIFI_MAX_AP >  (line - 1) ? (line - 1) : WIFI_MAX_AP;
    LOGD("wifinum = %d\n", wifinum);

    for (int i = 0; i < wifinum; i++) {
        result2 = strtok(wifiinfo[i], delims2);
        while(result2 != NULL) {
            tbline++;
            if (tbline==1) {
                strcpy(sAPs[i].smac, result2);
            } else if(tbline==2) {
                sAPs[i].frequency = atoi(result2);
            } else if(tbline==3) {
                sAPs[i].sig_level = atoi(result2);
            } else if(tbline==5) {
                strcpy(sAPs[i].name, result2);
            }
            result2 = strtok(NULL, delims2);
        }
        tbline = 0;
    }

    /* Converting sAPs[i].smac from character data to integar data */
    for (int i = 0; i < wifinum; i++) {
        char delims[] = ":";
        char *result = NULL;
        int pbline = 0;
        char wifismac[32] = {0};
        strcpy(wifismac, sAPs[i].smac);
        result = strtok(wifismac, delims);
        while (result != NULL) {
            sAPs[i].bssid[pbline++] = (unsigned char)strtol(result, NULL, 16);
            //LOGD( "sAPs[%d].bssid[%d]=%2x\n",i,pbline-1,(unsigned int)sAPs[i].bssid[pbline-1]);
            result = strtok(NULL, delims);
        }
    }

    return ret;
}

/* return:
 * >=0: scanned APs
 * -1: scanned none AP
 */
int wifi_show_result(char* data, unsigned int type) {
    int ret = 0;
    int num = 0;
    int len = 0;
    const int bssid_num = 6;
    const int sig_level_num = 4;
    const int freq_num = 4;

    switch(type) {
        case WIFI_AP_BSSID_ONLY:
            len = bssid_num;
            break;
        case WIFI_AP_BSSID_RSSI_FREQ:
            len = (bssid_num + sig_level_num + freq_num);
            break;
        case WIFI_AP_BSSID_RSSI:
            len = (bssid_num + sig_level_num);
            break;
        case WIFI_AP_BSSID_FREQ:
            len = (bssid_num + freq_num);
            break;
        case WIFI_AP_INFO_ALL:
            break;
         default:
            len = bssid_num;
            break;
    }

    num = wifinum;
    LOGD("num = %d\n", num);

    if (num > 0) {
        if (type == WIFI_AP_BSSID_ONLY ||
            type == WIFI_AP_BSSID_RSSI ||
            type == WIFI_AP_BSSID_RSSI_FREQ ||
            type == WIFI_AP_BSSID_FREQ) {
            unsigned char * pb = (unsigned char *)data;
            ret = num * len;
            for(int i = 0; i < num; i++) {
                pb[0]=sAPs[i].bssid[0];
                pb[1]=sAPs[i].bssid[1];
                pb[2]=sAPs[i].bssid[2];
                pb[3]=sAPs[i].bssid[3];
                pb[4]=sAPs[i].bssid[4];
                pb[5]=sAPs[i].bssid[5];
                LOGD("bssid %2x,%2x,%2x,%2x,%2x,%2x",sAPs[i].bssid[0],sAPs[i].bssid[1],sAPs[i].bssid[2],
                                                                     sAPs[i].bssid[3],sAPs[i].bssid[4],sAPs[i].bssid[5]);
                //LOGD("%2x,%2x,%2x,%2x,%2x,%2x",pb[0],pb[1],pb[2],pb[3],pb[4],pb[5]);
                pb += bssid_num;

                if (type == WIFI_AP_BSSID_RSSI) {
                    int sig = sAPs[i].sig_level;
                    pb[0] = (sig >> 0)  & 0xFF;
                    pb[1] = (sig >> 8)  & 0xFF;
                    pb[2] = (sig >> 16) & 0xFF;
                    pb[3] = (sig >> 24) & 0xFF;
                    LOGD("sig_level %02x,%02x,%02x,%02x,",pb[0],pb[1],pb[2],pb[3]);
                    pb += sig_level_num;
                } else if (type == WIFI_AP_BSSID_FREQ) {
                    int freq = sAPs[i].frequency;
                    pb[0] = (freq >> 0)  & 0xFF;
                    pb[1] = (freq >> 8)  & 0xFF;
                    pb[2] = (freq >> 16) & 0xFF;
                    pb[3] = (freq >> 24) & 0xFF;
                    LOGD("frequency %02x,%02x,%02x,%02x,",pb[0],pb[1],pb[2],pb[3]);
                    pb += freq_num;
                } else if (type == WIFI_AP_BSSID_RSSI_FREQ) { /* ugly but works... */
                    int sig = sAPs[i].sig_level;
                    int freq = sAPs[i].frequency;

                    pb[0] = (freq >> 0)  & 0xFF;
                    pb[1] = (freq >> 8)  & 0xFF;
                    pb[2] = (freq >> 16) & 0xFF;
                    pb[3] = (freq >> 24) & 0xFF;
                    LOGD("frequency+sig %02x,%02x,%02x,%02x,",pb[0],pb[1],pb[2],pb[3]);
                    pb += freq_num;

                    pb[0] = (sig >> 0)  & 0xFF;
                    pb[1] = (sig >> 8)  & 0xFF;
                    pb[2] = (sig >> 16) & 0xFF;
                    pb[3] = (sig >> 24) & 0xFF;
                    LOGD("frequency+sig %02x,%02x,%02x,%02x,",pb[0],pb[1],pb[2],pb[3]);
                    pb += sig_level_num;
		}
            }
        } else if (type == WIFI_AP_INFO_ALL) {
            char *wifi_infop = data;
            for (int i = 0; i < num; i++) {
                if (strlen(sAPs[i].name) > MAX_SSID_LEN) {
                    LOGE(" ssid is invalid, skip it, ssid:%s\n", sAPs[i].name);
                } else {
                    sprintf(wifi_infop,"%s %d ", sAPs[i].smac, sAPs[i].frequency);
                    LOGD("wifi_infop=%s", wifi_infop);
                    wifi_infop += strlen(wifi_infop);

                    sprintf(wifi_infop,"%d %s\n", sAPs[i].sig_level, sAPs[i].name);
                    LOGD("wifi_infop=%s", wifi_infop);
                    wifi_infop += strlen(wifi_infop);
                }
            }
            ret = strlen(data);
            LOGD("NativeMMI test, buffwifi=%s, ret=%d\n", data, ret);
        }
    } else {
        ret = -1;
    }

    return ret;
}

//------------------------------------------------------------------------------
int wifi_ap_type_check(void) {
    int ret = 0;
    int wifi5g_num =0;
    int wifi2g_num = 0;
    int num = wifinum;

    LOGD("wifi5G_flag=%d", wifi5G_flag);
    if (wifi5G_flag == 1) {
        for (int j = 0; j < num; j++) {
            if (sAPs[j].frequency > 4000) {
                wifi5g_num++;
            } else {
                wifi2g_num++;
            }
        }
        LOGD("wifi5g_num=%d, wifi2g_num=%d", wifi5g_num, wifi2g_num);
        if ((wifi5g_num < 1)  || (wifi2g_num < 1)) {
            ret = -1;
            LOGD("wifi5G test failed\n");
        }
    }

    return ret;
}

//------------------------------------------------------------------------------
int wifiClose(void) {
FUN_ENTER;
    int ret = 0;
    char cmd[100] = {0};
    int flag = -1;

    ret = wifi_stop_supplicant(0);
    if  (ret < 0) {
        LOGE("wifi_stop_supplicant failed, ret=%d\n", ret);
    }
    wifi_close_supplicant_connection();

    sprintf(cmd, "rmmod %s", WIFI_DRIVER_MODULE_PATH);
    flag = system(cmd);
    if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
        LOGE("%s:insmod sprdwl_ng.ko. flag=%d,err:%s\n",
                __FUNCTION__, flag, strerror(errno));
        return -1;
    }
    LOGE("wificlose ret=%d\n", ret);
FUN_EXIT;
    return ret;
}

/****************************************************************
buf: wifi autotest msg from pc
len: buf length
rsp: response data msg to pc
rsplen: rsp length

Message context:
    pc-->engpc:7E F2 5E 00 00 09 00 38 10 01 7E    // Open wifi
    engpc-->pc:7E 00 00 00 00 08 00 38 00 7E        // Response if success; If fail, response is 7E 00 00 00 00 08 00 38 01 7E

    pc-->engpc:7E F2 5E 00 00 09 00 38 10 02 7E    // Wifi scan
    engpc-->pc:7E 00 00 00 00 08 00 38 00 7E

    pc-->engpc:7E F2 5E 00 00 09 00 38 10 03 7E    // Get wifi ap bssid
    engpc-->pc:7E 00 00 00 00 0f 00 38 00 xx xx xx xx xx xx 7E    // bssid:6 bytes

    pc-->engpc:7E F2 5E 00 00 09 00 38 10 04 7E    // Close wifi
    engpc-->pc:7E 00 00 00 00 08 00 38 00 7E

    pc-->engpc:7E F2 5E 00 00 09 00 38 10 05 7E    // Get wifi ap bssid+rssi
    engpc-->pc:7E 00 00 00 00 0F 00 38 00 xx xx xx xx xx xx xx xx xx xx 7E    // bssid:6 bytes, rssi:4 bytes

    pc-->engpc:7E F2 5E 00 00 09 00 38 10 06 7E    // Get wifi ap bssid+freq
    engpc-->pc:7E 00 00 00 00 0F 00 38 00 xx xx xx xx xx xx xx xx xx xx 7E    // bssid:6 bytes, freq:4 bytes
****************************************************************/
int testWIFI(char *buf, int len, char *rsp, int rsplen) {
FUN_ENTER;
    /***************
    ret < 0, // fail
    ret = 0, // success, no data returned
    ret > 0, // success, return data length
    ***************/
    int ret = 0;
    char data[1014] = {0};

    LOGD("pc->engpc:%d number--> %x %x %x %x %x %x %x %x %x %x %x ",
            len,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10]);
    LOGD("testWIFI cmd = %d\n", buf[9]);

    switch(buf[9]) {
        case 1: // open wifi
            if ( wifiOpen() < 0 ) {
                LOGE("testWIFI:wifiOpen fail\n");
                ret =  -1;
            }
            break;
        case 2: //wifi scan
            if (wifi_ScanAP() < 0) {
                LOGE("testWIFI:wifi_ScanAP fail\n");
                ret =  -1;
            }
            break;
        case 3: // get wifi ap bssid
            LOGD("show wifi ap info - case 3\n");
            ret = wifi_show_result(data, WIFI_AP_BSSID_ONLY);
            break;
        case 4: // close wifi
            ret = wifiClose();
            break;
        case 5: // get wifi ap bssid+rssi
            LOGD("show wifi ap info - case 5\n");
            ret = wifi_show_result(data, WIFI_AP_BSSID_RSSI);
            break;
        case 6: // get wifi ap bssid+freq
            if (wifi_ap_type_check() < 0) {
                ret = -1;
                break;
            }

            LOGD("show wifi ap info - case 6\n");
            ret = wifi_show_result(data, WIFI_AP_BSSID_RSSI_FREQ);
            break;
        default:
            break;
    }

    // Fill the protocol header
    MSG_HEAD_T *msg_head_ptr;
    memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T) - 1);  // copy 7E xx xx xx xx 08 00 38 to rsp
    msg_head_ptr = (MSG_HEAD_T *)(rsp + 1);  // skip "7E"
    msg_head_ptr->len = 8;  // 8 bytes: xx xx xx xx 08 00 38 xx

    LOGD("msg_head_ptr, ret=%d", ret);
    if (ret < 0) {
        rsp[sizeof(MSG_HEAD_T)] = 1;    // 38 01, test failed
    } else if (ret == 0) {
        rsp[sizeof(MSG_HEAD_T)] = 0;    // 38 00, test successed
    } else if (ret > 0) {
        rsp[sizeof(MSG_HEAD_T)] = 0;    // 38 00, test successed, get extend data
        memcpy(rsp + 1 + sizeof(MSG_HEAD_T), data, ret);
        msg_head_ptr->len += ret;
    }
    LOGD("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d", sizeof(MSG_HEAD_T), rsp[sizeof(MSG_HEAD_T)]);

    rsp[msg_head_ptr->len + 2 - 1] = 0x7E;  // fill the protocol end byte "7E"
    LOGD("dylib test :return len:%d",msg_head_ptr->len + 2);
    LOGD("engpc->pc:%x %x %x %x %x %x %x %x %x %x",
            rsp[0],rsp[1],rsp[2],rsp[3],rsp[4],rsp[5],rsp[6],rsp[7],rsp[8],rsp[9]);

FUN_EXIT;
    return msg_head_ptr->len + 2;
}

/*************************************************
req: AT cmd from nativemmi, for example:"AT+WIFITEST"
rsp: response wifi data to nativemmi
**************************************************/
int testNativemmiwifi(char *req, char *rsp) {
FUN_ENTER ;
    int ret = 0;

    if (wifiOpen() < 0) {
        LOGE("wifiOpen fail\n");
        return -1;
    }

    if (wifi_ScanAP() < 0) {
        LOGE("wifi_ScanAP fail\n");
        ret = -1;
    } else {
        /* BBAT APP: if ret = -1, then failed; else success
         * and show scan results
         */
        ret = wifi_show_result(rsp, WIFI_AP_INFO_ALL);
        LOGD("testNativemmiwifi scan results: %d ret = %d\n", __LINE__, ret);

        if (ret >= 0 && wifi_ap_type_check() < 0) {
            LOGE("wifi_ap_type_check fail\n");
            ret = -1;
        }
    }

    if (wifiClose() < 0) {
        LOGE("wifiClose fail\n");
        ret = -1;
    }
    return ret;
}

//------------------------------------------------------------------------------
extern "C" void register_this_module_ext(struct eng_callback *reg, int *num) {
    int moudles_num = 0;   // number of registered nodes
    LOGD("register_this_module :dllibtest");

    //1st command
   reg->type = 0x38;  // 38: BBAT test
   reg->subtype = 0x10;   // 0x10: BBAT wifi test
   reg->eng_diag_func = testWIFI;
   moudles_num++;

   //2st command
   (reg+1)->type = 0x39;  // 39: nativemmi test
   (reg+1)->subtype = 0x10;   // 0x10: nativemmi wifi test
   sprintf((reg+1)->at_cmd, "%s", "AT+WIFITEST");  // AT cmd
   (reg+1)->eng_linuxcmd_func = testNativemmiwifi;
   moudles_num++;

   *num = moudles_num;
    LOGD("register_this_module_ext: %d - %d",*num, moudles_num);
}
