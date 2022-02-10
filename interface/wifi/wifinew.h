// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#ifndef _WIFINEW_20121110_H__
#define _WIFINEW_20121110_H__


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>

int wifi_start_supplicant(int );
int wifi_stop_supplicant(int );
int wifi_command(const char *command, char *reply, size_t *reply_len);
int wifi_connect_to_supplicant();
void wifi_close_supplicant_connection();
int wifi_ctrl_recv_event(char *reply, int buffer_len, size_t *reply_len, const char *event, int timeout);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _WIFI_20121110_H__