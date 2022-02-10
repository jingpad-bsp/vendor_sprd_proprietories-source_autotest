/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "npi_rtc_test.h"

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <linux/rtc.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>

#define RTC_DEV_FILE		"/dev/rtc0"

#include "sprd_fts_type.h"
#include "sprd_fts_diag.h"
#include "sprd_fts_log.h"


int package_test_result_to_rsp(char *buf, char *rsp, int ret, char *data_to_pc)
{
    MSG_HEAD_T *msg_head_ptr;
    // string rsp is composed by 5 parts: 0x7e, struct MSG_HEAD_T without subtype member, ret, data_to_pc, 0x7e.
    memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T) - 1);
    msg_head_ptr = (MSG_HEAD_T *)(rsp + 1);

    msg_head_ptr->len = 8;
    LOGD("msg_head_ptr,ret=%d", ret);

    if (ret < 0) {
        rsp[sizeof(MSG_HEAD_T)] = 1;
    } else if (ret == 0) {
        rsp[sizeof(MSG_HEAD_T)] = 0;
    } else if (ret >0) {//data_to_pc_len mean data length after 38 00
        rsp[sizeof(MSG_HEAD_T)] = 0;
        memcpy(rsp + 1 + sizeof(MSG_HEAD_T), data_to_pc, strlen(data_to_pc));
        msg_head_ptr->len += strlen(data_to_pc);
    }
    LOGD("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d", sizeof(MSG_HEAD_T), rsp[sizeof(MSG_HEAD_T)]);
    rsp[msg_head_ptr->len + 2 - 1] = 0x7E;  //eng flag
    LOGD("dylib test :return len:%d",msg_head_ptr->len + 2);
    LOGD("engpc->pc:%x %x %x %x %x %x %x %x %x %x",rsp[0],rsp[1],rsp[2],rsp[3],rsp[4],rsp[5],rsp[6],rsp[7],rsp[8],rsp[9]); //78 xx xx xx xx _ _38 _ _

    return msg_head_ptr->len + 2;
}

static int testRTC(char *buff, int buff_len, char *rsp, int rsplen)
{
    uint8_t buf[100];
    MSG_HEAD_T *msg_head_ptr = NULL;
    char *data = (char *)(buff + 1 + sizeof(MSG_HEAD_T));
    int data_len = buff_len - 2 - sizeof(MSG_HEAD_T);
    int rsp_buffer_size = 0;
    char *rsp_buffer = NULL;

    int bytes,i,ret,command;
    int min_time=0;
    int sec_time=0;
    int hour_flag,min_flag;
    int new_min_time=0;
    int new_sec_time=0;
    hour_flag=0;
    min_flag=0;
    command=data[0];
    char data_to_pc[1];
    int fd = -1, rtctime_fd = -1;
    struct rtc_time tm = {0, 0, 0, 1, 1, 115};

    LOGD("testRTC data[0] = %d\n", *data);

    rtctime_fd = open(RTC_DEV_FILE, O_RDWR);
    if (rtctime_fd < 0) {
	LOGD("open rtc dev file failed.\n");
	return rtctime_fd;
    }

    ret = ioctl(rtctime_fd, RTC_SET_TIME, &tm);
    if (ret < 0) {
	LOGD("ioctl /dev/rtc0 failed %d\n", ret);
	close(rtctime_fd);
	return ret;
    }
    LOGD("npi rtc test set time successful.\n");

    fd = open(RTC_PATH, O_RDONLY);
    if(fd < 0){
        LOGD("testRTC open = %s fail", RTC_PATH);
    }
    LOGD("testRTC command = %d\n", command);
    memset(buf,0,sizeof(buf));
    switch(command)
    {
        case 1:
            lseek(fd, 0, SEEK_SET);
            bytes=read(fd,buf,sizeof(buf));
            LOGD("test open time:%s\r\n",buf);
            if(bytes<0)
            {
                LOGD("read failed\r\n");
                ret=-1;
                break;
            }
            else
                ret=0;
            break;
        case 3:
            ret = 1;
            bytes=read(fd,buf,sizeof(buf));
            LOGD("test count time:%s\r\n",buf);
            if(bytes<0){
                break;
            }
            else
            {
                for(i=0;i<bytes;i++)
                {
                     if((buf[i]==':')&&(buf[i+1]!=' ')&&(min_flag==0))
                    {
                        min_time=(buf[i+1]-48)*10+(buf[i+2]-48);
                        min_flag=1;
                        LOGD("min_time:%d\n",min_time);
                    }
                    else if((buf[i]==':')&&(min_flag==1))
                    {
                        sec_time=(buf[i+1]-48)*10+(buf[i+2]-48);
                        min_flag=0;
                        LOGD("sec_time:%d\n",sec_time);
                        break;
                    }
                }
            }
            memset(buf,0,sizeof(buf));
            sleep(3);
            lseek(fd, 0, SEEK_SET);
            bytes=read(fd,buf,sizeof(buf));
            LOGD("time:%s\r\n",buf);
            if(bytes<0)
            {
                LOGD("read failed\r\n");
                ret=-1;
                break;
            }
            else
            {
                for(i=0;i<bytes;i++)
                {
                     if((buf[i]==':')&&(buf[i+1]!=' ')&&(min_flag==0))
                    {
                        new_min_time=(buf[i+1]-48)*10+(buf[i+2]-48);
                        min_flag=1;
                        LOGD("new_min_time:%d\n",new_min_time);
                    }
                    else if((buf[i]==':')&&(min_flag==1))
                    {
                        new_sec_time=(buf[i+1]-48)*10+(buf[i+2]-48);
                        LOGD("new_sec_time:%d\n",new_sec_time);
                        min_flag=0;
                        break;
                    }
                }
            }
            if(new_sec_time==(sec_time+3))
                {
                rsp[0]=0;
                ret = 0;
                data_to_pc[0] = 0;
                break;
                }
            else if(sec_time>=57)
                {
                if(new_sec_time==((sec_time+3)%60))
                rsp[0]=0;
                ret = 0;
                data_to_pc[0] = 0;
                break;
                }
            else{
                rsp[0]=1;
                data_to_pc[0] = 1;
            }
            LOGD("testRTC 1 rsp[0]:%d\n",rsp[0]);
            break;
        case 4:
            lseek(fd, 0, SEEK_SET);
            bytes=read(fd,buf,sizeof(buf));
            LOGD("time:%s\r\n",buf);
            if(bytes<0)
                ret=-1;
            else
                ret=0;
            break;
        default:
            break;
    }
    close(rtctime_fd);
    close(fd);
    int rsplent = package_test_result_to_rsp(buff, rsp, ret, data_to_pc);
    LOGD("testRTC 22 rsplent:%d\n",rsplent);
    return rsplent;
}

extern "C" {
void register_this_module_ext(struct eng_callback *reg, int *num)
{
    unsigned int moudles_num = 0;

    (reg + moudles_num)->type = BBAT_TEST;
    (reg + moudles_num)->subtype = BBAT_RTC_TEST;
    (reg + moudles_num)->eng_diag_func = testRTC;
    LOGD("register module for Diag cmd testRTC");
    moudles_num ++;

    *num = moudles_num;
    return;
}
}
