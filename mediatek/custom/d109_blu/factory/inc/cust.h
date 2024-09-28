/************************************************************************
History:
5. chunming.tian@ragentek.com 20130503 BUG_ID:None
   Description:Modified according to plant needs
4. chuiguo.zeng@ragentek.com 2013.04.24 BUG_ID:None
   Description: Add the macro to controll whether g-sensor support z axis.
3. chuiguo.zeng@ragentek.com 2013.05.03 BUG_ID:None
   Description: Remove sub camera in D207.
2. chuiguo.zeng@ragentek.com 2013.04.22 BUG_ID:None
   Description: Remove GPS test item due to D206 has no GPS.
1. chuiguo.zeng@ragentek.com 2013.04.15 BUG_ID:None
   Description: Remove wave playback test as requirement request.
************************************************************************/

#ifndef FTM_CUST_H
#define FTM_CUST_H

#define FEATURE_FTM_AUDIO
//#define FEATURE_DUMMY_AUDIO
#define FEATURE_FTM_BATTERY
#define FEATURE_FTM_PMIC_632X
#define BATTERY_TYPE_Z3
#define FEATURE_FTM_VBAT_TEMP_CHECK
#define FEATURE_FTM_BT
#define FEATURE_FTM_FM
//#define FEATURE_FTM_FMTX
#define FEATURE_FTM_GPS	//add BUG_ID:DYLJ-88 zengchuiguo 20130722
#if defined(MTK_WLAN_SUPPORT)
#define FEATURE_FTM_WIFI
#endif
#define FEATURE_FTM_MAIN_CAMERA
//#define FEATURE_FTM_MAIN2_CAMERA
//#define FEATURE_FTM_SUB_CAMERA	//delete BUG_ID:None zengchuiguo 20130503 (start)
#ifdef MTK_EMMC_SUPPORT
#define FEATURE_FTM_EMMC
#define FEATURE_FTM_CLEAREMMC
#else
#define FEATURE_FTM_FLASH
#define FEATURE_FTM_CLEARFLASH
#endif
#define FEATURE_FTM_KEYS
#define FEATURE_FTM_LCD
#define FEATURE_FTM_LED
#define FEATURE_FTM_MEMCARD
//#define FEATURE_FTM_RTC
#define FEATURE_FTM_TOUCH
#define FEATURE_FTM_VIBRATOR
//#define FEATURE_FTM_IDLE
//#define FEATURE_FTM_CLEARFLASH
#define FEATURE_FTM_ACCDET
#define FEATURE_FTM_HEADSET
#define HEADSET_BUTTON_DETECTION
#ifdef HAVE_MATV_FEATURE
//#define FEATURE_FTM_MATV
#endif
//#define FEATURE_FTM_FONT_10x18
//#define FEATURE_FTM_OTG
#define FEATURE_FTM_SIM
//#define FEATURE_FTM_SPK_OC

//modify BUG_ID:None tianchunming 20130503(start)
//delete BUG_ID:None zengchuiguo 20130328 (start)
#define FEATURE_FTM_WAVE_PLAYBACK 
//delete BUG_ID:None zengchuiguo 20130328 (end)
//modify BUG_ID:None tianchunming 20130503(end)

//add BUG_ID:None zengchuiguo 20130424 (start)
#define FEATURE_GSENSOR_NOT_SUPPORT_Z_AXIS
//add BUG_ID:None zengchuiguo 20130424 (end)

#include "cust_font.h"		/* common part */
#include "cust_keys.h"		/* custom part */
#include "cust_lcd.h"		/* custom part */
#include "cust_led.h"		/* custom part */
#include "cust_touch.h"         /* custom part */

#endif /* FTM_CUST_H */
