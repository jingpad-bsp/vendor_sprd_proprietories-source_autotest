// 
// Spreadtrum Auto Tester
//
// anli   2012-11-12
//
#ifndef _TCARD_20121112_H__
#define _TCARD_20121112_H__

#define ID_BIT      9
#define SDCARD1     0
#define SDCARD2     1

#define bool int
#define true 1
#define false 0

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_tcard {
//-----------------------------------------------------------------------------
int get_sdcard_path(char **sd1_path, char **sd2_path);
bool is_sdcard_exist(char * path);
//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _TCARD_20121112_H__
