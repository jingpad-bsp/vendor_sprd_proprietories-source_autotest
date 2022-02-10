
#ifndef __LIGHT_20180420_H__
#define __LIGHT_20180420_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_light {
//-----------------------------------------------------------------------------

typedef enum LIGHT_IDX_E {
    LIGHT_INDEX_BACKLIGHT = 0,
    LIGHT_INDEX_KEYBOARD,
    LIGHT_INDEX_BUTTONS,
    LIGHT_INDEX_BATTERY,
    LIGHT_INDEX_NOTIFICATIONS,
    LIGHT_INDEX_ATTENTION,
    LIGHT_INDEX_BLUETOOTH,
    LIGHT_INDEX_WIFI,
    LIGHT_COUNT,
} LIGHT_IDX;

int set_light(int idx, int brightness);
int lightOpen( void );

int lightSetLCD( int brightness );
//int lightSetKeyBoard( int brightness );
//int lightSetButtons( int brightness );

int lightSetKeypad( int brightness );

int lightSetRgb(int index , int brightness);

int lightClose( void );

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // __LIGHT_20180420_H__
