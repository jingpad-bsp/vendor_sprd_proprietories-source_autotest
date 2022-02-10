#ifndef WIFI_INTERFACE_H
#define WIFI_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "wifinew.h"
#include "sprd_fts_type.h"
#include "sprd_fts_log.h"
#include "sprd_fts_diag.h"

#define WIFI_MAX_AP      20
#define MAX_SSID_LEN 32

enum {
    WIFI_AP_BSSID_ONLY = 0,
    WIFI_AP_BSSID_RSSI,
    WIFI_AP_BSSID_RSSI_FREQ,
    WIFI_AP_BSSID_FREQ,
    WIFI_AP_INFO_ALL
};

typedef struct wifi_ap_t {
    char smac[32];  // string mac
    unsigned char bssid[10];
    char name[256];  //wifi name
    int  sig_level;
    int frequency;
}wifi_ap_t;

int wifinum;
wifi_ap_t sAPs[WIFI_MAX_AP]={0};
int wifiOpen( void );
int wifi_ScanAP( void );
int wifi_parse_result(void);
int wifi_show_result(char* data, unsigned int type);
int wifi_ap_type_check(void);
int wifiClose(void );
#ifdef __cplusplus
}
#endif // __cplusplus

#endif
