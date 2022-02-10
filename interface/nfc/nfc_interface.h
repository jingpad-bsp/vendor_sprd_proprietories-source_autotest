#include <stdlib.h>
#include <hardware/hardware.h>
#include <hardware/nfc.h>
#include <cutils/log.h>
#include <dlfcn.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include "sprd_fts_type.h"
#include "sprd_fts_log.h"
#include "sprd_fts_diag.h"


#define POWER_DRIVER "/dev/sec-nfc"
#define TRANS_DRIVER "/dev/sec-nfc"

/* ioctl */
#define SEC_NFC_MAGIC 'S'
#define SEC_NFC_GET_MODE    _IOW(SEC_NFC_MAGIC, 0, unsigned int)
#define SEC_NFC_SET_MODE    _IOW(SEC_NFC_MAGIC, 1, unsigned int)
#define SEC_NFC_SLEEP       _IOW(SEC_NFC_MAGIC, 2, unsigned int)
#define SEC_NFC_WAKEUP      _IOW(SEC_NFC_MAGIC, 3, unsigned int)


enum {
    NFC_OPEN = 0x01,
    NFC_CHECKTAG = 0x02,
    NFC_CLOSE = 0x03
};

typedef enum {
    NFC_DEV_MODE_OFF = 0,
    NFC_DEV_MODE_ON,
    NFC_DEV_MODE_BOOTLOADER
}eNFC_DEV_MODE;

static nfc_event_t last_event = HAL_NFC_ERROR_EVT;
static nfc_status_t last_evt_status = HAL_NFC_STATUS_FAILED;
void hal_context_callback(nfc_event_t event, nfc_status_t event_status) {
    last_event = event;
    last_evt_status = event_status;
    ALOGD("nfc context callback event=%u, status=%u\n", event, event_status);
}

static uint16_t last_recv_data_len = 0;
static uint8_t last_rx_buff[256]={0};
void hal_context_data_callback (uint16_t data_len, uint8_t* p_data) {
    ALOGD("nfc data callback len=%d\n", data_len);
    last_recv_data_len = data_len;
    memcpy(last_rx_buff, p_data, data_len);
}

const struct hw_module_t* hw_module;
nfc_nci_device_t* nfc_device;

int max_try = 0;
uint8_t nci_core_reset_cmd[4]={0x20,0x00,0x01,0x01};
uint8_t nci_initializ_cmd[3]={0x20,0x01,0x00};
uint8_t discover_map_cmd[10]={0x21,0x00,0x07,0x02,0x04,0x03,0x02,0x05,0x03,0x03};
uint8_t rf_discover_cmd_all[30]={0x21,0x03,0x1b,0x0d,0x00,0x01,0x01,0x01,0x02,0x01,
                                 0x03,0x01,0x05,0x01,0x80,0x01,0x81,0x01,0x82,0x01,
                                 0x83,0x01,0x85,0x01,0x06,0x01,0x74,0x01,0x70,0x01};
uint8_t rf_discover_cmd_limit[8]={0x21,0x03,0x05,0x02,0x00,0x01,0x01,0x01};

int openClose(void);
int checkTag(void);
int closeNfc(void);

void OSI_delay(uint32_t timeout);
int device_set_mode(eNFC_DEV_MODE mode);