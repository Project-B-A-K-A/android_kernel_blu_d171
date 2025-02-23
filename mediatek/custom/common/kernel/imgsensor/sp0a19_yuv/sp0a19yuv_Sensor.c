/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sp0a19yuv_sub_Sensor.c
 *
 * Project:
 * --------
 *   MAUI
 *
 * Description:


 * ------------
 *   Image sensor driver function
 *   V1.2.3
 *
 * Author:
 * -------
 *   Leo
 *
 *=============================================================
 *             HISTORY
 * Below this line, this part is controlled by GCoreinc. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Log$
 * 2012.02.29  kill bugs
 *   
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by GCoreinc. DO NOT MODIFY!!
 *=============================================================
 ******************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "sp0a19yuv_Sensor.h"
#include "sp0a19yuv_Camera_Sensor_para.h"
#include "sp0a19yuv_CameraCustomized.h"


#define SP0A19YUV_DEBUG
#ifdef SP0A19YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

kal_bool   SP0A19_MPEG4_encode_mode = KAL_FALSE;
kal_uint16 SP0A19_dummy_pixels = 0, SP0A19_dummy_lines = 0;
kal_bool   SP0A19_NIGHT_MODE = KAL_FALSE;

kal_uint32 SP0A19_isp_master_clock= 0;
static kal_uint32 SP0A19_PCLK = 24000000;

kal_uint8 sp0a19_isBanding = 0; // 0: 50hz  1:60hz
static kal_uint32 zoom_factor = 0; 

//extern kal_bool searchMainSensor;


MSDK_SENSOR_CONFIG_STRUCT SP0A19SensorConfigData;
MSDK_SCENARIO_ID_ENUM SP0A19CurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;

#define SP0A19_SET_PAGE0 	SP0A19_write_cmos_sensor(0xfd, 0x00)
#define SP0A19_SET_PAGE1 	SP0A19_write_cmos_sensor(0xfd, 0x01)


extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

kal_uint16 SP0A19_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
    char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 2, SP0A19_WRITE_ID);

}
kal_uint16 SP0A19_read_cmos_sensor(kal_uint8 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd = { (char)(addr & 0xFF) };
	iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte, 1, SP0A19_WRITE_ID);
	
    return get_byte;
}


static void SP0A19_InitialPara(void)
{
  SP0A19_CurrentStatus.iNightMode = 0;
  SP0A19_CurrentStatus.iWB = AWB_MODE_AUTO;
  SP0A19_CurrentStatus.iEffect = MEFFECT_OFF;
  SP0A19_CurrentStatus.iBanding = AE_FLICKER_MODE_50HZ;
  SP0A19_CurrentStatus.iEV = AE_EV_COMP_00;
  SP0A19_CurrentStatus.iMirror = IMAGE_NORMAL;
  SP0A19_CurrentStatus.iFrameRate = 0;
}





/*************************************************************************
 * FUNCTION
 *	SP0A19_SetShutter
 *
 * DESCRIPTION
 *	This function set e-shutter of SP0A19 to change exposure time.
 *
 * PARAMETERS
 *   iShutter : exposured lines
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void SP0A19_Set_Shutter(kal_uint16 iShutter)
{
} 


/*************************************************************************
 * FUNCTION
 *	SP0A19_read_Shutter
 *
 * DESCRIPTION
 *	This function read e-shutter of SP0A19 .
 *
 * PARAMETERS
 *  None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint16 SP0A19_Read_Shutter(void)
{
    kal_uint8 temp_reg1, temp_reg2;
	kal_uint16 shutter;

	temp_reg1 = SP0A19_read_cmos_sensor(0x04);
	temp_reg2 = SP0A19_read_cmos_sensor(0x03);

	shutter = (temp_reg1 & 0xFF) | (temp_reg2 << 8);

	return shutter;
} /* SP0A19_read_shutter */


/*************************************************************************
 * FUNCTION
 *	SP0A19_write_reg
 *
 * DESCRIPTION
 *	This function set the register of SP0A19.
 *
 * PARAMETERS
 *	addr : the register index of SP0A19
 *  para : setting parameter of the specified register of SP0A19
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void SP0A19_write_reg(kal_uint32 addr, kal_uint32 para)
{
	SP0A19_write_cmos_sensor(addr, para);
}


/*************************************************************************
 * FUNCTION
 *	SP0A19_read_reg
 *
 * DESCRIPTION
 *	This function read parameter of specified register from SP0A19.
 *
 * PARAMETERS
 *	addr : the register index of SP0A19
 *
 * RETURNS
 *	the data that read from SP0A19
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint32 SP0A19_read_reg(kal_uint32 addr)
{
	return SP0A19_read_cmos_sensor(addr);
}

/*************************************************************************
* FUNCTION
*	SP0A19_awb_enable
*
* DESCRIPTION
*	This function enable or disable the awb (Auto White Balance).
*
* PARAMETERS
*	1. kal_bool : KAL_TRUE - enable awb, KAL_FALSE - disable awb.
*
* RETURNS
*	kal_bool : It means set awb right or not.
*
*************************************************************************/
static void SP0A19_awb_enable(kal_bool enalbe)
{	 
	kal_uint16 temp_AWB_reg = 0;

	temp_AWB_reg = SP0A19_read_cmos_sensor(0x42);
	
	if (enalbe)
	{
		SP0A19_write_cmos_sensor(0x42, (temp_AWB_reg |0x02));
	}
	else
	{
		SP0A19_write_cmos_sensor(0x42, (temp_AWB_reg & (~0x02)));
	}

}


/*************************************************************************
 * FUNCTION
 *	SP0A19_SetGain
 *
 * DESCRIPTION
 *	This function is to set global gain to sensor.
 *
 * PARAMETERS
 *   iGain : sensor global gain(base: 0x40)
 *
 * RETURNS
 *	the actually gain set to sensor.
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint16 SP0A19_SetGain(kal_uint16 iGain)//hkm
{
	return iGain;
}


/*************************************************************************
 * FUNCTION
 *	SP0A19_NightMode
 *
 * DESCRIPTION
 *	This function night mode of SP0A19.
 *
 * PARAMETERS
 *	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void SP0A19NightMode(kal_bool bEnable)//hkm
{
    // kal_uint8 temp = SP0A19_read_cmos_sensor(0x3B);

	if(bEnable)//night mode
	{ 
		SP0A19_write_cmos_sensor(0xfd,0x00);
		SP0A19_write_cmos_sensor(0xb2,0x25);
		SP0A19_write_cmos_sensor(0xb3,0x1f);
					
	   if(SP0A19_MPEG4_encode_mode == KAL_TRUE)
		{
				if(sp0a19_isBanding== 0)
				{
				//Video record night 24M 50hz 12-12FPS maxgain:0x8c
					SP0A19_write_cmos_sensor(0xfd,0x00);
					SP0A19_write_cmos_sensor(0x03,0x00);
					SP0A19_write_cmos_sensor(0x04,0x7b);
					SP0A19_write_cmos_sensor(0x06,0x00);
					SP0A19_write_cmos_sensor(0x09,0x08);
					SP0A19_write_cmos_sensor(0x0a,0x1d);
					SP0A19_write_cmos_sensor(0xf0,0x29);
					SP0A19_write_cmos_sensor(0xf1,0x00);
					SP0A19_write_cmos_sensor(0xfd,0x01);
					SP0A19_write_cmos_sensor(0x90,0x0c);
					SP0A19_write_cmos_sensor(0x92,0x01);
					SP0A19_write_cmos_sensor(0x98,0x29);
					SP0A19_write_cmos_sensor(0x99,0x00);
					SP0A19_write_cmos_sensor(0x9a,0x01);
					SP0A19_write_cmos_sensor(0x9b,0x00);
					SP0A19_write_cmos_sensor(0xfd,0x01);
					SP0A19_write_cmos_sensor(0xce,0xec);
					SP0A19_write_cmos_sensor(0xcf,0x01);
					SP0A19_write_cmos_sensor(0xd0,0xec);
					SP0A19_write_cmos_sensor(0xd1,0x01);
					SP0A19_write_cmos_sensor(0xfd,0x00);
				    SENSORDB(" video 50Hz night\r\n");
				}
				else if(sp0a19_isBanding == 1)
				{
				//SI13_SP0A19 24M 1分频 50Hz 8.0392-8fps AE_Parameters_ maxgain 80
					//dbg_print("SP0A19_VID_NIGTHT_60HZ  \r\n");
					SP0A19_write_cmos_sensor(0xfd,0x00);
					SP0A19_write_cmos_sensor(0x03,0x00);
					SP0A19_write_cmos_sensor(0x04,0x69);
					SP0A19_write_cmos_sensor(0x06,0x00);
					SP0A19_write_cmos_sensor(0x09,0x07);
					SP0A19_write_cmos_sensor(0x0a,0xd7);
					SP0A19_write_cmos_sensor(0xf0,0x23);
					SP0A19_write_cmos_sensor(0xf1,0x00);
					SP0A19_write_cmos_sensor(0xfd,0x01);
					SP0A19_write_cmos_sensor(0x90,0x0f);
					SP0A19_write_cmos_sensor(0x92,0x01);
					SP0A19_write_cmos_sensor(0x98,0x23);
					SP0A19_write_cmos_sensor(0x99,0x00);
					SP0A19_write_cmos_sensor(0x9a,0x01);
					SP0A19_write_cmos_sensor(0x9b,0x00);
					SP0A19_write_cmos_sensor(0xfd,0x01);
					SP0A19_write_cmos_sensor(0xce,0x0d);
					SP0A19_write_cmos_sensor(0xcf,0x02);
					SP0A19_write_cmos_sensor(0xd0,0x0d);
					SP0A19_write_cmos_sensor(0xd1,0x02);
					SP0A19_write_cmos_sensor(0xfd,0x00);	
				     SENSORDB(" video 60Hz night\r\n");
				}
   		  	}	
	    else 
	    {  
				
			       if(sp0a19_isBanding== 0)
					{
					//capture preview night 24M 50hz 20-6FPS maxgain:0x78	 
					SP0A19_write_cmos_sensor(0xfd,0x00);
					SP0A19_write_cmos_sensor(0x03,0x01);
					SP0A19_write_cmos_sensor(0x04,0x32);
					SP0A19_write_cmos_sensor(0x06,0x00);
					SP0A19_write_cmos_sensor(0x09,0x01);
					SP0A19_write_cmos_sensor(0x0a,0x46);
					SP0A19_write_cmos_sensor(0xf0,0x66);
					SP0A19_write_cmos_sensor(0xf1,0x00);
					SP0A19_write_cmos_sensor(0xfd,0x01);
					SP0A19_write_cmos_sensor(0x90,0x10);
					SP0A19_write_cmos_sensor(0x92,0x01);
					SP0A19_write_cmos_sensor(0x98,0x66);
					SP0A19_write_cmos_sensor(0x99,0x00);
					SP0A19_write_cmos_sensor(0x9a,0x01);
					SP0A19_write_cmos_sensor(0x9b,0x00);
					SP0A19_write_cmos_sensor(0xfd,0x01);
					SP0A19_write_cmos_sensor(0xce,0x60);
					SP0A19_write_cmos_sensor(0xcf,0x06);
					SP0A19_write_cmos_sensor(0xd0,0x60);
					SP0A19_write_cmos_sensor(0xd1,0x06);
					SP0A19_write_cmos_sensor(0xfd,0x00);	
				    SENSORDB(" priview 50Hz night\r\n");	
				}  
				else if(sp0a19_isBanding== 1)
				{
				//capture preview night 24M 60hz 20-6FPS maxgain:0x78
					SP0A19_write_cmos_sensor(0xfd,0x00);
					SP0A19_write_cmos_sensor(0x03,0x00);
					SP0A19_write_cmos_sensor(0x04,0xff);
					SP0A19_write_cmos_sensor(0x06,0x00);
					SP0A19_write_cmos_sensor(0x09,0x01);
					SP0A19_write_cmos_sensor(0x0a,0x46);
					SP0A19_write_cmos_sensor(0xf0,0x55);
					SP0A19_write_cmos_sensor(0xf1,0x00);
					SP0A19_write_cmos_sensor(0xfd,0x01);
					SP0A19_write_cmos_sensor(0x90,0x14);
					SP0A19_write_cmos_sensor(0x92,0x01);
					SP0A19_write_cmos_sensor(0x98,0x55);
					SP0A19_write_cmos_sensor(0x99,0x00);
					SP0A19_write_cmos_sensor(0x9a,0x01);
					SP0A19_write_cmos_sensor(0x9b,0x00);
					SP0A19_write_cmos_sensor(0xfd,0x01);
					SP0A19_write_cmos_sensor(0xce,0xa4);
					SP0A19_write_cmos_sensor(0xcf,0x06);
					SP0A19_write_cmos_sensor(0xd0,0xa4);
					SP0A19_write_cmos_sensor(0xd1,0x06);
					SP0A19_write_cmos_sensor(0xfd,0x00);
				    SENSORDB(" priview 60Hz night\r\n");	
				}
			       } 		
	}
	else    // daylight mode
	{
		
				SP0A19_write_cmos_sensor(0xfd,0x0 );
				SP0A19_write_cmos_sensor(0xb2,0x08);
				SP0A19_write_cmos_sensor(0xb3,0x1f);
					
	         if(SP0A19_MPEG4_encode_mode == KAL_TRUE)
	          {
				
				if(sp0a19_isBanding== 0)
				{
				//Video record daylight 24M 50hz 14-14FPS maxgain:0x80
					SP0A19_write_cmos_sensor(0xfd,0x00);
					SP0A19_write_cmos_sensor(0x03,0x00);
					SP0A19_write_cmos_sensor(0x04,0xd8);
					SP0A19_write_cmos_sensor(0x06,0x00);
					SP0A19_write_cmos_sensor(0x09,0x03);
					SP0A19_write_cmos_sensor(0x0a,0x31);
					SP0A19_write_cmos_sensor(0xf0,0x48);
					SP0A19_write_cmos_sensor(0xf1,0x00);
					SP0A19_write_cmos_sensor(0xfd,0x01);
					SP0A19_write_cmos_sensor(0x90,0x07);
					SP0A19_write_cmos_sensor(0x92,0x01);
					SP0A19_write_cmos_sensor(0x98,0x48);
					SP0A19_write_cmos_sensor(0x99,0x00);
					SP0A19_write_cmos_sensor(0x9a,0x01);
					SP0A19_write_cmos_sensor(0x9b,0x00);
					SP0A19_write_cmos_sensor(0xfd,0x01);
					SP0A19_write_cmos_sensor(0xce,0xf8);
					SP0A19_write_cmos_sensor(0xcf,0x01);
					SP0A19_write_cmos_sensor(0xd0,0xf8);
					SP0A19_write_cmos_sensor(0xd1,0x01);
					SP0A19_write_cmos_sensor(0xfd,0x00);        			
				       SENSORDB(" video 50Hz normal\r\n");				
				}
				else if(sp0a19_isBanding == 1)
				{
				//Video record daylight 24M 60Hz 14-14FPS maxgain:0x80
					SP0A19_write_cmos_sensor(0xfd,0x00);
					SP0A19_write_cmos_sensor(0x03,0x00);
					SP0A19_write_cmos_sensor(0x04,0xb4);
					SP0A19_write_cmos_sensor(0x06,0x00);
					SP0A19_write_cmos_sensor(0x09,0x03);
					SP0A19_write_cmos_sensor(0x0a,0x31);
					SP0A19_write_cmos_sensor(0xf0,0x3c);
					SP0A19_write_cmos_sensor(0xf1,0x00);
					SP0A19_write_cmos_sensor(0xfd,0x01);
					SP0A19_write_cmos_sensor(0x90,0x08);
					SP0A19_write_cmos_sensor(0x92,0x01);
					SP0A19_write_cmos_sensor(0x98,0x3c);
					SP0A19_write_cmos_sensor(0x99,0x00);
					SP0A19_write_cmos_sensor(0x9a,0x01);
					SP0A19_write_cmos_sensor(0x9b,0x00);
					SP0A19_write_cmos_sensor(0xfd,0x01);
					SP0A19_write_cmos_sensor(0xce,0xe0);
					SP0A19_write_cmos_sensor(0xcf,0x01);
					SP0A19_write_cmos_sensor(0xd0,0xe0);
					SP0A19_write_cmos_sensor(0xd1,0x01);
					SP0A19_write_cmos_sensor(0xfd,0x00);			
				    SENSORDB(" video 60Hz normal \n");	
				}
			   }
		else 
			{
			   //	SENSORDB(" SP0A19_banding=%x\r\n",SP0A19_banding);
			       if(sp0a19_isBanding== 0)
				{
				//capture preview daylight 24M 50hz 20-8FPS maxgain:0x70   
					SP0A19_write_cmos_sensor(0xfd,0x00);
					SP0A19_write_cmos_sensor(0x03,0x01);
					SP0A19_write_cmos_sensor(0x04,0x32);
					SP0A19_write_cmos_sensor(0x06,0x00);
					SP0A19_write_cmos_sensor(0x09,0x01);
					SP0A19_write_cmos_sensor(0x0a,0x46);
					SP0A19_write_cmos_sensor(0xf0,0x66);
					SP0A19_write_cmos_sensor(0xf1,0x00);
					SP0A19_write_cmos_sensor(0xfd,0x01);
					SP0A19_write_cmos_sensor(0x90,0x0c);
					SP0A19_write_cmos_sensor(0x92,0x01);
					SP0A19_write_cmos_sensor(0x98,0x66);
					SP0A19_write_cmos_sensor(0x99,0x00);
					SP0A19_write_cmos_sensor(0x9a,0x01);
					SP0A19_write_cmos_sensor(0x9b,0x00);
					SP0A19_write_cmos_sensor(0xfd,0x01);
					SP0A19_write_cmos_sensor(0xce,0xc8);
					SP0A19_write_cmos_sensor(0xcf,0x04);
					SP0A19_write_cmos_sensor(0xd0,0xc8);
					SP0A19_write_cmos_sensor(0xd1,0x04);
					SP0A19_write_cmos_sensor(0xfd,0x00);	
				    SENSORDB(" priview 50Hz normal\r\n");
				}
				else if(sp0a19_isBanding== 1)
				{
				//capture preview daylight 24M 60hz 20-8FPS maxgain:0x70   
					SP0A19_write_cmos_sensor(0xfd,0x00);
					SP0A19_write_cmos_sensor(0x03,0x00);
					SP0A19_write_cmos_sensor(0x04,0xff);
					SP0A19_write_cmos_sensor(0x06,0x00);
					SP0A19_write_cmos_sensor(0x09,0x01);
					SP0A19_write_cmos_sensor(0x0a,0x46);
					SP0A19_write_cmos_sensor(0xf0,0x55);
					SP0A19_write_cmos_sensor(0xf1,0x00);
					SP0A19_write_cmos_sensor(0xfd,0x01);
					SP0A19_write_cmos_sensor(0x90,0x0f);
					SP0A19_write_cmos_sensor(0x92,0x01);
					SP0A19_write_cmos_sensor(0x98,0x55);
					SP0A19_write_cmos_sensor(0x99,0x00);
					SP0A19_write_cmos_sensor(0x9a,0x01);
					SP0A19_write_cmos_sensor(0x9b,0x00);
					SP0A19_write_cmos_sensor(0xfd,0x01);
					SP0A19_write_cmos_sensor(0xce,0xfb);
					SP0A19_write_cmos_sensor(0xcf,0x04);
					SP0A19_write_cmos_sensor(0xd0,0xfb);
					SP0A19_write_cmos_sensor(0xd1,0x04);
					SP0A19_write_cmos_sensor(0xfd,0x00);	
				    SENSORDB(" priview 60Hz normal\r\n");
				}
			       }
	   
	}  
	SP0A19_CurrentStatus.iNightMode = bEnable;
}

/*************************************************************************
* FUNCTION
*	SP0A19_Sensor_Init
*
* DESCRIPTION
*	This function apply all of the initial setting to sensor.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
*************************************************************************/
void SP0A19_Sensor_Init(void)
{  
	SENSORDB("SP0A19_Sensor_Init enter \n");

	SP0A19_write_cmos_sensor(0xfd,0x00);
	SP0A19_write_cmos_sensor(0x1C,0x28);
	SP0A19_write_cmos_sensor(0x32,0x00);
	SP0A19_write_cmos_sensor(0x0f,0x2f);
	SP0A19_write_cmos_sensor(0x10,0x2e);
	SP0A19_write_cmos_sensor(0x11,0x00);
	SP0A19_write_cmos_sensor(0x12,0x18);
	SP0A19_write_cmos_sensor(0x13,0x2f);
	SP0A19_write_cmos_sensor(0x14,0x00);
	SP0A19_write_cmos_sensor(0x15,0x3f);
	SP0A19_write_cmos_sensor(0x16,0x00);
	SP0A19_write_cmos_sensor(0x17,0x18);
	SP0A19_write_cmos_sensor(0x25,0x40);
	SP0A19_write_cmos_sensor(0x1a,0x0b);
	SP0A19_write_cmos_sensor(0x1b,0x0c);
	SP0A19_write_cmos_sensor(0x1e,0x0b);
	SP0A19_write_cmos_sensor(0x20,0x3f); // add
	SP0A19_write_cmos_sensor(0x21,0x13); // 0x0c 24
	SP0A19_write_cmos_sensor(0x22,0x19);
	SP0A19_write_cmos_sensor(0x26,0x1a);
	SP0A19_write_cmos_sensor(0x27,0xab);
	SP0A19_write_cmos_sensor(0x28,0xfd);
	SP0A19_write_cmos_sensor(0x30,0x00);
	#if defined(PROJECT_CUSTOMER_J221)
	printk("PROJECT_CUSTOMER_J221\n");
	SP0A19_write_cmos_sensor(0x31,0x20);//0x10
	#elif defined(PROJECT_CUSTOMER_D207)
		printk("PROJECT_CUSTOMER_D207\n");
	SP0A19_write_cmos_sensor(0x31,0x00);//0x10
	#else
		printk("PROJECT_CUSTOMER_XXX\n");
	SP0A19_write_cmos_sensor(0x31,0x60);//0x10
	#endif
	SP0A19_write_cmos_sensor(0xfb,0x30); // 0x33
	SP0A19_write_cmos_sensor(0x1f,0x10);

	//Blacklevel
	SP0A19_write_cmos_sensor(0xfd,0x00);
	SP0A19_write_cmos_sensor(0x65,0x06);//blue_suboffset
	SP0A19_write_cmos_sensor(0x66,0x06);//red_suboffset
	SP0A19_write_cmos_sensor(0x67,0x06);//gr_suboffset
	SP0A19_write_cmos_sensor(0x68,0x06);//gb_suboffset
	SP0A19_write_cmos_sensor(0x45,0x00);
	SP0A19_write_cmos_sensor(0x46,0x0f);
	//ae setting
	SP0A19_write_cmos_sensor(0xfd,0x00);
	SP0A19_write_cmos_sensor(0x03,0x01);
	SP0A19_write_cmos_sensor(0x04,0x32);
	SP0A19_write_cmos_sensor(0x06,0x00);
	SP0A19_write_cmos_sensor(0x09,0x01);
	SP0A19_write_cmos_sensor(0x0a,0x46);
	SP0A19_write_cmos_sensor(0xf0,0x66);
	SP0A19_write_cmos_sensor(0xf1,0x00);
	SP0A19_write_cmos_sensor(0xfd,0x01);
	SP0A19_write_cmos_sensor(0x90,0x0c);
	SP0A19_write_cmos_sensor(0x92,0x01);
	SP0A19_write_cmos_sensor(0x98,0x66);
	SP0A19_write_cmos_sensor(0x99,0x00);
	SP0A19_write_cmos_sensor(0x9a,0x01);
	SP0A19_write_cmos_sensor(0x9b,0x00);

	//Status
	SP0A19_write_cmos_sensor(0xfd,0x01);
	SP0A19_write_cmos_sensor(0xce,0x98);
	SP0A19_write_cmos_sensor(0xcf,0x04);
	SP0A19_write_cmos_sensor(0xd0,0x98);
	SP0A19_write_cmos_sensor(0xd1,0x04); 

	SP0A19_write_cmos_sensor(0xfd,0x01);
	SP0A19_write_cmos_sensor(0xc4,0x56);//70
	SP0A19_write_cmos_sensor(0xc5,0x8f);//74
	SP0A19_write_cmos_sensor(0xca,0x30);
	SP0A19_write_cmos_sensor(0xcb,0x45);
	SP0A19_write_cmos_sensor(0xcc,0x70);//rpc_heq_low
	SP0A19_write_cmos_sensor(0xcd,0x48);//rpc_heq_dummy
	SP0A19_write_cmos_sensor(0xfd,0x00);

	//lsc  for st 
	SP0A19_write_cmos_sensor(0xfd,0x01);
	SP0A19_write_cmos_sensor(0x35,0x15);
	SP0A19_write_cmos_sensor(0x36,0x15); //20
	SP0A19_write_cmos_sensor(0x37,0x15);
	SP0A19_write_cmos_sensor(0x38,0x15);
	SP0A19_write_cmos_sensor(0x39,0x15);
	SP0A19_write_cmos_sensor(0x3a,0x15); //15
	SP0A19_write_cmos_sensor(0x3b,0x13);
	SP0A19_write_cmos_sensor(0x3c,0x15);
	SP0A19_write_cmos_sensor(0x3d,0x15);
	SP0A19_write_cmos_sensor(0x3e,0x15); //12
	SP0A19_write_cmos_sensor(0x3f,0x15);
	SP0A19_write_cmos_sensor(0x40,0x18);
	SP0A19_write_cmos_sensor(0x41,0x00);
	SP0A19_write_cmos_sensor(0x42,0x04);
	SP0A19_write_cmos_sensor(0x43,0x04);
	SP0A19_write_cmos_sensor(0x44,0x00);
	SP0A19_write_cmos_sensor(0x45,0x00);
	SP0A19_write_cmos_sensor(0x46,0x00);
	SP0A19_write_cmos_sensor(0x47,0x00);
	SP0A19_write_cmos_sensor(0x48,0x00);
	SP0A19_write_cmos_sensor(0x49,0xfd);
	SP0A19_write_cmos_sensor(0x4a,0x00);
	SP0A19_write_cmos_sensor(0x4b,0x00);
	SP0A19_write_cmos_sensor(0x4c,0xfd);
	SP0A19_write_cmos_sensor(0xfd,0x00);

	//awb 1
	SP0A19_write_cmos_sensor(0xfd,0x01);
	SP0A19_write_cmos_sensor(0x28,0xc5);
	SP0A19_write_cmos_sensor(0x29,0x9b);
	//SP0A19_write_cmos_sensor(0x10,0x08);
	//SP0A19_write_cmos_sensor(0x11,0x14);	
	//SP0A19_write_cmos_sensor(0x12,0x14);
	SP0A19_write_cmos_sensor(0x2e,0x02);	
	SP0A19_write_cmos_sensor(0x2f,0x16);
	SP0A19_write_cmos_sensor(0x17,0x17);
	SP0A19_write_cmos_sensor(0x18,0x19);	//0x29	 0813
	SP0A19_write_cmos_sensor(0x19,0x45);	

	//SP0A19_write_cmos_sensor(0x1a,0x9e);//a1;a5   
	//SP0A19_write_cmos_sensor(0x1b,0xae);//b0;9a
	//SP0A19_write_cmos_sensor(0x33,0xef);
	SP0A19_write_cmos_sensor(0x2a,0xef);
	SP0A19_write_cmos_sensor(0x2b,0x15);

	//awb2
	SP0A19_write_cmos_sensor(0xfd,0x01);
	SP0A19_write_cmos_sensor(0x73,0x80);
	SP0A19_write_cmos_sensor(0x1a,0x80);
	SP0A19_write_cmos_sensor(0x1b,0x80); 
	//d65
	SP0A19_write_cmos_sensor(0x65,0xd5); //d6
	SP0A19_write_cmos_sensor(0x66,0xfa); //f0
	SP0A19_write_cmos_sensor(0x67,0x72); //7a
	SP0A19_write_cmos_sensor(0x68,0x8a); //9a
	//indoor
	SP0A19_write_cmos_sensor(0x69,0xc6); //ab
	SP0A19_write_cmos_sensor(0x6a,0xee); //ca
	SP0A19_write_cmos_sensor(0x6b,0x94); //a3
	SP0A19_write_cmos_sensor(0x6c,0xab); //c1
	//f 
	SP0A19_write_cmos_sensor(0x61,0x7a); //82
	SP0A19_write_cmos_sensor(0x62,0x98); //a5
	SP0A19_write_cmos_sensor(0x63,0xc5); //d6
	SP0A19_write_cmos_sensor(0x64,0xe6); //ec
	//cwf
	SP0A19_write_cmos_sensor(0x6d,0xb9); //a5
	SP0A19_write_cmos_sensor(0x6e,0xde); //c2
	SP0A19_write_cmos_sensor(0x6f,0xb2); //a7
	SP0A19_write_cmos_sensor(0x70,0xd5); //c5

	//skin detect
	SP0A19_write_cmos_sensor(0xfd,0x01);
	SP0A19_write_cmos_sensor(0x08,0x15);
	SP0A19_write_cmos_sensor(0x09,0x04);
	SP0A19_write_cmos_sensor(0x0a,0x20);
	SP0A19_write_cmos_sensor(0x0b,0x12);
	SP0A19_write_cmos_sensor(0x0c,0x27);
	SP0A19_write_cmos_sensor(0x0d,0x06);
	SP0A19_write_cmos_sensor(0x0f,0x63);//0x5f	0813

	//BPC_grad
	SP0A19_write_cmos_sensor(0xfd,0x00);
	SP0A19_write_cmos_sensor(0x79,0xf0);
	SP0A19_write_cmos_sensor(0x7a,0x80);  //f0
	SP0A19_write_cmos_sensor(0x7b,0x80);  //f0
	SP0A19_write_cmos_sensor(0x7c,0x20); //f0

	//smooth
	SP0A19_write_cmos_sensor(0xfd,0x00);

	//
	SP0A19_write_cmos_sensor(0x57,0x06); //raw_dif_thr_outdoor
	SP0A19_write_cmos_sensor(0x58,0x0d); //raw_dif_thr_normal
	SP0A19_write_cmos_sensor(0x56,0x10); //raw_dif_thr_dummy
	SP0A19_write_cmos_sensor(0x59,0x10); //raw_dif_thr_lowlight
	//GrGb
	SP0A19_write_cmos_sensor(0x89,0x06); //raw_grgb_thr_outdoor 
	SP0A19_write_cmos_sensor(0x8a,0x0d); //raw_grgb_thr_normal	
	SP0A19_write_cmos_sensor(0x9c,0x10); //raw_grgb_thr_dummy	
	SP0A19_write_cmos_sensor(0x9d,0x10); //raw_grgb_thr_lowlight
	//Gr\Gb
	SP0A19_write_cmos_sensor(0x81,0xe0); //raw_gflt_fac_outdoor
	SP0A19_write_cmos_sensor(0x82,0xe0); //raw_gflt_fac_normal
	SP0A19_write_cmos_sensor(0x83,0x80); //raw_gflt_fac_dummy
	SP0A19_write_cmos_sensor(0x84,0x40); //raw_gflt_fac_lowlight
	//GrGb
	SP0A19_write_cmos_sensor(0x85,0xe0); //raw_gf_fac_outdoor  
	SP0A19_write_cmos_sensor(0x86,0xc0); //raw_gf_fac_normal  
	SP0A19_write_cmos_sensor(0x87,0x80); //raw_gf_fac_dummy   
	SP0A19_write_cmos_sensor(0x88,0x40); //raw_gf_fac_lowlight
	//
	SP0A19_write_cmos_sensor(0x5a,0xff);  //raw_rb_fac_outdoor
	SP0A19_write_cmos_sensor(0x5b,0xe0);  //raw_rb_fac_normal
	SP0A19_write_cmos_sensor(0x5c,0x80);  //raw_rb_fac_dummy
	SP0A19_write_cmos_sensor(0x5d,0x00);  //raw_rb_fac_lowlight

	//sharpen 
	SP0A19_write_cmos_sensor(0xfd,0x01); 
	SP0A19_write_cmos_sensor(0xe2,0x30); //sharpen_y_base
	SP0A19_write_cmos_sensor(0xe4,0xa0); //sharpen_y_max

	SP0A19_write_cmos_sensor(0xe5,0x04); //rangek_neg_outdoor  //0x08
	SP0A19_write_cmos_sensor(0xd3,0x04); //rangek_pos_outdoor	//0x08
	SP0A19_write_cmos_sensor(0xd7,0x04); //range_base_outdoor	//0x08

	SP0A19_write_cmos_sensor(0xe6,0x04); //rangek_neg_normal   // 0x08
	SP0A19_write_cmos_sensor(0xd4,0x04); //rangek_pos_normal   // 0x08
	SP0A19_write_cmos_sensor(0xd8,0x04); //range_base_normal   // 0x08

	SP0A19_write_cmos_sensor(0xe7,0x08); //rangek_neg_dummy   // 0x10
	SP0A19_write_cmos_sensor(0xd5,0x08); //rangek_pos_dummy   // 0x10
	SP0A19_write_cmos_sensor(0xd9,0x08); //range_base_dummy    // 0x10

	SP0A19_write_cmos_sensor(0xd2,0x10); //rangek_neg_lowlight
	SP0A19_write_cmos_sensor(0xd6,0x10); //rangek_pos_lowlight
	SP0A19_write_cmos_sensor(0xda,0x10); //range_base_lowlight

	SP0A19_write_cmos_sensor(0xe8,0x20);  //sharp_fac_pos_outdoor  // 0x35
	SP0A19_write_cmos_sensor(0xec,0x35);  //sharp_fac_neg_outdoor
	SP0A19_write_cmos_sensor(0xe9,0x20);  //sharp_fac_pos_nr	  // 0x35
	SP0A19_write_cmos_sensor(0xed,0x35);  //sharp_fac_neg_nr
	SP0A19_write_cmos_sensor(0xea,0x20);  //sharp_fac_pos_dummy   // 0x30
	SP0A19_write_cmos_sensor(0xef,0x30);  //sharp_fac_neg_dummy   // 0x20
	SP0A19_write_cmos_sensor(0xeb,0x10);  //sharp_fac_pos_low
	SP0A19_write_cmos_sensor(0xf0,0x20);  //sharp_fac_neg_low 

	//CCM
	SP0A19_write_cmos_sensor(0xfd,0x01);
	SP0A19_write_cmos_sensor(0xa0,0x85);  //0x80  //zouyu 20121009
	SP0A19_write_cmos_sensor(0xa1,0x00);
	SP0A19_write_cmos_sensor(0xa2,0x00);
	SP0A19_write_cmos_sensor(0xa3,0xf3);  // 0xf6
	SP0A19_write_cmos_sensor(0xa4,0x8e);  // 0x99
	SP0A19_write_cmos_sensor(0xa5,0x00);  // 0xf2
	SP0A19_write_cmos_sensor(0xa6,0x00);  // 0x0d
	SP0A19_write_cmos_sensor(0xa7,0xe6);   // 0xda
	SP0A19_write_cmos_sensor(0xa8,0x9a);//0xa0 0813  // 0x98
	SP0A19_write_cmos_sensor(0xa9,0x00);
	SP0A19_write_cmos_sensor(0xaa,0x03);  // 0x33
	SP0A19_write_cmos_sensor(0xab,0x0c);
	SP0A19_write_cmos_sensor(0xfd,0x00);

	//gamma  
	SP0A19_write_cmos_sensor(0xfd,0x00);
	SP0A19_write_cmos_sensor(0x8b,0x0 );  // 00;0 ;0 
	SP0A19_write_cmos_sensor(0x8c,0xC );  // 0f;C ;11
	SP0A19_write_cmos_sensor(0x8d,0x19);  // 1e;19;19
	SP0A19_write_cmos_sensor(0x8e,0x2C);  // 3d;2C;28
	SP0A19_write_cmos_sensor(0x8f,0x49);  // 6c;49;46
	SP0A19_write_cmos_sensor(0x90,0x61);  // 92;61;61
	SP0A19_write_cmos_sensor(0x91,0x77);  // aa;77;78
	SP0A19_write_cmos_sensor(0x92,0x8A);  // b9;8A;8A
	SP0A19_write_cmos_sensor(0x93,0x9B);  // c4;9B;9B
	SP0A19_write_cmos_sensor(0x94,0xA9);  // cf;A9;A9
	SP0A19_write_cmos_sensor(0x95,0xB5);  // d4;B5;B5
	SP0A19_write_cmos_sensor(0x96,0xC0);  // da;C0;C0
	SP0A19_write_cmos_sensor(0x97,0xCA);  // e0;CA;CA
	SP0A19_write_cmos_sensor(0x98,0xD4);  // e4;D4;D4
	SP0A19_write_cmos_sensor(0x99,0xDD);  // e8;DD;DD
	SP0A19_write_cmos_sensor(0x9a,0xE6);  // ec;E6;E6
	SP0A19_write_cmos_sensor(0x9b,0xEF);  // f1;EF;EF
	SP0A19_write_cmos_sensor(0xfd,0x01);  // 01;01;01
	SP0A19_write_cmos_sensor(0x8d,0xF7);  // f7;F7;F7
	SP0A19_write_cmos_sensor(0x8e,0xFF);  // ff;FF;FF		 
	SP0A19_write_cmos_sensor(0xfd,0x00);  //

	//rpc
	SP0A19_write_cmos_sensor(0xfd,0x00); 
	SP0A19_write_cmos_sensor(0xe0,0x4c); //  4c;44;4c;3e;3c;3a;38;rpc_1base_max
	SP0A19_write_cmos_sensor(0xe1,0x3c); //  3c;36;3c;30;2e;2c;2a;rpc_2base_max
	SP0A19_write_cmos_sensor(0xe2,0x34); //  34;2e;34;2a;28;26;26;rpc_3base_max
	SP0A19_write_cmos_sensor(0xe3,0x2e); //  2e;2a;2e;26;24;22;rpc_4base_max
	SP0A19_write_cmos_sensor(0xe4,0x2e); //  2e;2a;2e;26;24;22;rpc_5base_max
	SP0A19_write_cmos_sensor(0xe5,0x2c); //  2c;28;2c;24;22;20;rpc_6base_max
	SP0A19_write_cmos_sensor(0xe6,0x2c); //  2c;28;2c;24;22;20;rpc_7base_max
	SP0A19_write_cmos_sensor(0xe8,0x2a); //  2a;26;2a;22;20;20;1e;rpc_8base_max
	SP0A19_write_cmos_sensor(0xe9,0x2a); //  2a;26;2a;22;20;20;1e;rpc_9base_max 
	SP0A19_write_cmos_sensor(0xea,0x2a); //  2a;26;2a;22;20;20;1e;rpc_10base_max
	SP0A19_write_cmos_sensor(0xeb,0x28); //  28;24;28;20;1f;1e;1d;rpc_11base_max
	SP0A19_write_cmos_sensor(0xf5,0x28); //  28;24;28;20;1f;1e;1d;rpc_12base_max
	SP0A19_write_cmos_sensor(0xf6,0x28); //  28;24;28;20;1f;1e;1d;rpc_13base_max	

	//ae min gain  
	SP0A19_write_cmos_sensor(0xfd,0x01);
	SP0A19_write_cmos_sensor(0x94,0xa0);  //rpc_max_indr
	SP0A19_write_cmos_sensor(0x95,0x28);   // 1e rpc_min_indr 
	SP0A19_write_cmos_sensor(0x9c,0xa0);  //rpc_max_outdr
	SP0A19_write_cmos_sensor(0x9d,0x28);  //rpc_min_outdr	 
	//ae target
	SP0A19_write_cmos_sensor(0xfd,0x00); 
	SP0A19_write_cmos_sensor(0xed,0x8c); //80 
	SP0A19_write_cmos_sensor(0xf7,0x88); //7c 
	SP0A19_write_cmos_sensor(0xf8,0x80); //70 
	SP0A19_write_cmos_sensor(0xec,0x7c); //6c  

	SP0A19_write_cmos_sensor(0xef,0x74); //99
	SP0A19_write_cmos_sensor(0xf9,0x70); //90
	SP0A19_write_cmos_sensor(0xfa,0x68); //80
	SP0A19_write_cmos_sensor(0xee,0x64); //78

	//gray detect
	SP0A19_write_cmos_sensor(0xfd,0x01);
	SP0A19_write_cmos_sensor(0x30,0x40);
	//add 0813 
	SP0A19_write_cmos_sensor(0x31,0x70);
	SP0A19_write_cmos_sensor(0x32,0x40);
	SP0A19_write_cmos_sensor(0x33,0xef);
	SP0A19_write_cmos_sensor(0x34,0x05);
	SP0A19_write_cmos_sensor(0x4d,0x2f);
	SP0A19_write_cmos_sensor(0x4e,0x20);
	SP0A19_write_cmos_sensor(0x4f,0x16);

	//lowlight lum
	SP0A19_write_cmos_sensor(0xfd,0x00); //
	SP0A19_write_cmos_sensor(0xb2,0x20); //lum_limit  // 0x10
	SP0A19_write_cmos_sensor(0xb3,0x1f); //lum_set
	SP0A19_write_cmos_sensor(0xb4,0x30); //black_vt  // 0x20
	SP0A19_write_cmos_sensor(0xb5,0x45); //white_vt

	//saturation
	SP0A19_write_cmos_sensor(0xfd,0x00); 
	SP0A19_write_cmos_sensor(0xbe,0xff); 
	SP0A19_write_cmos_sensor(0xbf,0x01); 
	SP0A19_write_cmos_sensor(0xc0,0xff); 
	SP0A19_write_cmos_sensor(0xc1,0xd8); 
	SP0A19_write_cmos_sensor(0xd3,0x88); //0x78
	SP0A19_write_cmos_sensor(0xd4,0x88); //0x78
	SP0A19_write_cmos_sensor(0xd6,0x70); //0x78 	   (0xd7,0x60); //0x78
	SP0A19_write_cmos_sensor(0xd7,0x60);

	//HEQ
	SP0A19_write_cmos_sensor(0xfd,0x00); 
	SP0A19_write_cmos_sensor(0xdc,0x00); 
	SP0A19_write_cmos_sensor(0xdd,0x80); //0x80 0813  // 0x78
	SP0A19_write_cmos_sensor(0xde,0xa8); //80  0x88  0813
	SP0A19_write_cmos_sensor(0xdf,0x80); 

	//func enable
	SP0A19_write_cmos_sensor(0xfd,0x00);  
	SP0A19_write_cmos_sensor(0x32,0x15);  //0x0d
	SP0A19_write_cmos_sensor(0x34,0x76);  //16
	SP0A19_write_cmos_sensor(0x35,0x00);  
	SP0A19_write_cmos_sensor(0x33,0xef);  
	SP0A19_write_cmos_sensor(0x5f,0x51); 
	SENSORDB("SP0A19_Sensor_Init out \n");
}


/*************************************************************************
* FUNCTION
*	SP0A19_GAMMA_Select
*
* DESCRIPTION
*	This function is served for FAE to select the appropriate GAMMA curve.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 SP0A19GetSensorID(UINT32 *sensorID)
{
    kal_uint16 sensor_id=0;

	SENSORDB("SP0A19GetSensorID \n");	

#if 0  //sym modify
		//specical config for PowerOn sequence,other sensor don't follow this ;
		if(searchMainSensor == KAL_TRUE)
		{
			*sensorID=0xFFFFFFFF;
			SENSORDB("search main sensor loop, SP0A19 directly exit \r\n");
			return ERROR_SENSOR_CONNECT_FAIL;
		}
#endif


	SP0A19_write_cmos_sensor(0xfd,0x00);
	sensor_id=SP0A19_read_cmos_sensor(0x02);
	SENSORDB("Sensor Read SP0A19GetSensorID: %x \r\n",sensor_id);

	*sensorID=sensor_id;
	if (sensor_id != SP0A19_YUV_SENSOR_ID)
	{
		*sensorID=0xFFFFFFFF;
		SENSORDB("Sensor Read ByeBye \r\n");
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;	  
}


/*************************************************************************
 * FUNCTION
 *	SP0A19Open
 *
 * DESCRIPTION
 *	This function initialize the registers of CMOS sensor
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 SP0A19Open(void)
{
    kal_uint16 sensor_id=0;

     SENSORDB("SP0A19Open \n");

		
	SP0A19_write_cmos_sensor(0xfd,0x00);
    sensor_id=SP0A19_read_cmos_sensor(0x02);
    SENSORDB("SP0A19Open Sensor id = %x\n", sensor_id);
	
	if (sensor_id != SP0A19_YUV_SENSOR_ID) 
	   {
		  SENSORDB("Sensor Read ByeBye \r\n");
	      return ERROR_SENSOR_CONNECT_FAIL;
	   }

    mdelay(10);
    SP0A19_Sensor_Init();		
    SENSORDB("SP0A19Open end \n");
    return ERROR_NONE;
}


/*************************************************************************
 * FUNCTION
 *	SP0A19Close
 *
 * DESCRIPTION
 *	This function is to turn off sensor module power.
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 SP0A19Close(void)
{
   SENSORDB("SP0A19Close\n");
    return ERROR_NONE;
} 


static void SP0A19_set_dummy( const kal_uint32 iPixels, const kal_uint32 iLines )//hkm
{



}




/*************************************************************************
 * FUNCTION
 * SP0A19Preview
 *
 * DESCRIPTION
 *	This function start the sensor preview.
 *
 * PARAMETERS
 *	*image_window : address pointer of pixel numbers in one period of HSYNC
 *  *sensor_config_data : address pointer of line numbers in one period of VSYNC
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 SP0A19Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
	SENSORDB("SP0A19Preview enter \n");

    if(sensor_config_data->SensorOperationMode == MSDK_SENSOR_OPERATION_MODE_VIDEO)
	{
        SP0A19_MPEG4_encode_mode = KAL_TRUE;
    }
    else
    {
        SP0A19_MPEG4_encode_mode = KAL_FALSE;
    }

    image_window->GrabStartX= SP0A19_IMAGE_SENSOR_START_PIXELS;
    image_window->GrabStartY= SP0A19_IMAGE_SENSOR_START_LINES;
    image_window->ExposureWindowWidth = SP0A19_IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight = SP0A19_IMAGE_SENSOR_PV_HEIGHT;

    // copy sensor_config_data
    memcpy(&SP0A19SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	SENSORDB("SP0A19Preview out \n");
    return ERROR_NONE;
} 


/*************************************************************************
 * FUNCTION
 *	SP0A19Capture
 *
 * DESCRIPTION
 *	This function setup the CMOS sensor in capture MY_OUTPUT mode
 *
 * PARAMETERS
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 SP0A19Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
	SENSORDB("SP0A19Capture enter \n");

    image_window->GrabStartX = SP0A19_IMAGE_SENSOR_START_PIXELS;
    image_window->GrabStartY = SP0A19_IMAGE_SENSOR_START_LINES;
    image_window->ExposureWindowWidth=  SP0A19_IMAGE_SENSOR_FULL_WIDTH;
    image_window->ExposureWindowHeight = SP0A19_IMAGE_SENSOR_FULL_HEIGHT;

    // copy sensor_config_data
    memcpy(&SP0A19SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	SENSORDB("SP0A19Capture out \n");
	
    return ERROR_NONE;
} /* SP0A19_Capture() */



UINT32 SP0A19GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    pSensorResolution->SensorFullWidth=		SP0A19_IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight=	SP0A19_IMAGE_SENSOR_FULL_HEIGHT;
    pSensorResolution->SensorPreviewWidth=	SP0A19_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight=	SP0A19_IMAGE_SENSOR_PV_HEIGHT;
    pSensorResolution->SensorVideoWidth=	SP0A19_IMAGE_SENSOR_VIDEO_WIDTH;
    pSensorResolution->SensorVideoHeight=	SP0A19_IMAGE_SENSOR_VIDEO_HEIGHT;
	
    return ERROR_NONE;
}


UINT32 SP0A19GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
        MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

    pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=10;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=1;
	
    //pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
	//pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_UYVY;
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_VYUY;
	
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_HIGH;
    pSensorInfo->SensorInterruptDelayLines = 1;
	
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;

    pSensorInfo->CaptureDelayFrame = 1;
    pSensorInfo->PreviewDelayFrame = 1;
    pSensorInfo->VideoDelayFrame = 4;
    pSensorInfo->SensorMasterClockSwitch = 0;
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;

	pSensorInfo->YUVAwbDelayFrame = 2;
	pSensorInfo->YUVEffectDelayFrame= 2;

    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        pSensorInfo->SensorClockFreq=24;
        pSensorInfo->SensorClockDividCount=	3;
        pSensorInfo->SensorClockRisingCount= 0;
        pSensorInfo->SensorClockFallingCount= 2;
        pSensorInfo->SensorPixelClockCount= 3;
        pSensorInfo->SensorDataLatchCount= 2;
        pSensorInfo->SensorGrabStartX = SP0A19_IMAGE_SENSOR_START_PIXELS;
        pSensorInfo->SensorGrabStartY = SP0A19_IMAGE_SENSOR_START_LINES;

        break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        pSensorInfo->SensorClockFreq=24;
        pSensorInfo->SensorClockDividCount= 3;
        pSensorInfo->SensorClockRisingCount=0;
        pSensorInfo->SensorClockFallingCount=2;
        pSensorInfo->SensorPixelClockCount=3;
        pSensorInfo->SensorDataLatchCount=2;
        pSensorInfo->SensorGrabStartX = SP0A19_IMAGE_SENSOR_START_PIXELS;
        pSensorInfo->SensorGrabStartY = SP0A19_IMAGE_SENSOR_START_LINES;
        break;
    default:
        pSensorInfo->SensorClockFreq=24;
        pSensorInfo->SensorClockDividCount= 3;
        pSensorInfo->SensorClockRisingCount=0;
        pSensorInfo->SensorClockFallingCount=2;
        pSensorInfo->SensorPixelClockCount=3;
        pSensorInfo->SensorDataLatchCount=2;
        pSensorInfo->SensorGrabStartX = SP0A19_IMAGE_SENSOR_START_PIXELS;
        pSensorInfo->SensorGrabStartY = SP0A19_IMAGE_SENSOR_START_LINES;
        break;
    }
    memcpy(pSensorConfigData, &SP0A19SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
}


UINT32 SP0A19Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

    SP0A19CurrentScenarioId = ScenarioId;

	SENSORDB("SP0A19Control:ScenarioId =%d \n",SP0A19CurrentScenarioId);
   
    switch (ScenarioId)
    {
	    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
	        SP0A19Preview(pImageWindow, pSensorConfigData);
	        break;
	    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
	        SP0A19Capture(pImageWindow, pSensorConfigData);
	        break;
	    default:
	        return ERROR_INVALID_SCENARIO_ID;
    }


    return ERROR_NONE;
}	/* SP0A19Control() */

BOOL SP0A19_set_param_wb(UINT16 para)
{

	if(SP0A19_CurrentStatus.iWB == para)
		return TRUE;
	
	switch (para)
	{            
		case AWB_MODE_AUTO:
		    //SP0A19_reg_WB_auto         自动
			SP0A19_write_cmos_sensor(0xfd,0x01);                                                          
			SP0A19_write_cmos_sensor(0x28,0xc4);		                                                       
			SP0A19_write_cmos_sensor(0x29,0x9e);
			SP0A19_write_cmos_sensor(0xfd,0x00);  // AUTO 3000K~7000K   	  
			SP0A19_write_cmos_sensor(0x32,0x0d);     				
		    break;
		case AWB_MODE_CLOUDY_DAYLIGHT:
		    // SP0A19_reg_WB_auto   阴天
			SP0A19_write_cmos_sensor(0xfd,0x00);   //7000K                                     
			SP0A19_write_cmos_sensor(0x32,0x05);                                                          
			SP0A19_write_cmos_sensor(0xfd,0x01);                                                          
			SP0A19_write_cmos_sensor(0x28,0xbf);		                                                       
			SP0A19_write_cmos_sensor(0x29,0x89);		                                                       
			SP0A19_write_cmos_sensor(0xfd,0x00);   
		    break;
		case AWB_MODE_DAYLIGHT:
	        // SP0A19_reg_WB_auto  白天 
			SP0A19_write_cmos_sensor(0xfd,0x00);  //6500K                                     
			SP0A19_write_cmos_sensor(0x32,0x05);                                                          
			SP0A19_write_cmos_sensor(0xfd,0x01);                                                          
			SP0A19_write_cmos_sensor(0x28,0xbc);		                                                       
			SP0A19_write_cmos_sensor(0x29,0x5d);		                                                       
			SP0A19_write_cmos_sensor(0xfd,0x00); 
		    break;
		case AWB_MODE_INCANDESCENT:	
		    // SP0A19_reg_WB_auto 白炽灯 
			SP0A19_write_cmos_sensor(0xfd,0x00);  //2800K~3000K                                     
			SP0A19_write_cmos_sensor(0x32,0x05);                                                          
			SP0A19_write_cmos_sensor(0xfd,0x01);                                                          
			SP0A19_write_cmos_sensor(0x28,0x89);		                                                       
			SP0A19_write_cmos_sensor(0x29,0xb8);		                                                       
			SP0A19_write_cmos_sensor(0xfd,0x00); 
		    break;  
		case AWB_MODE_FLUORESCENT:
	        //SP0A19_reg_WB_auto  荧光灯 
			SP0A19_write_cmos_sensor(0xfd,0x00);  //4200K~5000K                                     
			SP0A19_write_cmos_sensor(0x32,0x05);                                                          
			SP0A19_write_cmos_sensor(0xfd,0x01);                                                          
			SP0A19_write_cmos_sensor(0x28,0xb5);		                                                       
			SP0A19_write_cmos_sensor(0x29,0xa5);		                                                       
			SP0A19_write_cmos_sensor(0xfd,0x00); 
		    break;  
		case AWB_MODE_TUNGSTEN:
	        // SP0A19_reg_WB_auto 白热光
			SP0A19_write_cmos_sensor(0xfd,0x00);  //4000K                                   
			SP0A19_write_cmos_sensor(0x32,0x05);                                                          
			SP0A19_write_cmos_sensor(0xfd,0x01);                                                          
			SP0A19_write_cmos_sensor(0x28,0xaf);		                                                       
			SP0A19_write_cmos_sensor(0x29,0x99);		                                                       
			SP0A19_write_cmos_sensor(0xfd,0x00);  
		    break;
		default:
			return KAL_FALSE;
	}

	SP0A19_CurrentStatus.iWB = para;
	
	return KAL_TRUE;
}


BOOL SP0A19_set_param_effect(UINT16 para)
{

	
	if(SP0A19_CurrentStatus.iEffect == para)
		return TRUE;

	switch (para)
	{
		case MEFFECT_OFF:
            SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0x62, 0x00);
			SP0A19_write_cmos_sensor(0x63, 0x80);
			SP0A19_write_cmos_sensor(0x64, 0x80);
	        break;
		case MEFFECT_SEPIA:
            SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0x62, 0x10);
			SP0A19_write_cmos_sensor(0x63, 0xc0);
			SP0A19_write_cmos_sensor(0x64, 0x20);
			break;  
		case MEFFECT_NEGATIVE:
            SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0x62, 0x04);
			SP0A19_write_cmos_sensor(0x63, 0x80);
			SP0A19_write_cmos_sensor(0x64, 0x80);
			break; 
		case MEFFECT_SEPIAGREEN:		
	        SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0x62, 0x10);
			SP0A19_write_cmos_sensor(0x63, 0x20);
			SP0A19_write_cmos_sensor(0x64, 0x20);
			break;
		case MEFFECT_SEPIABLUE:
		  	SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0x62, 0x10);
			SP0A19_write_cmos_sensor(0x63, 0x20);
			SP0A19_write_cmos_sensor(0x64, 0xf0);
			break;        
		case MEFFECT_MONO:			
			SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0x62, 0x20);
			SP0A19_write_cmos_sensor(0x63, 0x80);
			SP0A19_write_cmos_sensor(0x64, 0x80);
			break;
		default:
			return KAL_FALSE;
	}
	
	SP0A19_CurrentStatus.iEffect = para;

	return KAL_TRUE;
}


BOOL SP0A19_set_param_banding(UINT16 para)//hkm
{
	if(SP0A19_CurrentStatus.iBanding == para)
      return TRUE;

	switch (para)
	{
		case AE_FLICKER_MODE_50HZ:
			sp0a19_isBanding = 0;
			break;

		case AE_FLICKER_MODE_60HZ:
			sp0a19_isBanding = 1;
		break;
		default:
		return FALSE;
	}
	 SP0A19NightMode(KAL_FALSE);//hkm

	SP0A19_CurrentStatus.iBanding =para;

	return TRUE;
} /* SP0A19_set_param_banding */


BOOL SP0A19_set_param_exposure(UINT16 para)
{
	if(SP0A19_CurrentStatus.iEV == para)
      return TRUE;

	switch (para)
	{
		case AE_EV_COMP_10:	 //	+1 EV	
			SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0xdc, 0x10);
			break;    
		case AE_EV_COMP_00:  // +0 EV
		    SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0xdc, 0x00);
		case AE_EV_COMP_n10:  // -1 EV
			SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0xdc, 0xf0);
			break;    
		case AE_EV_COMP_n13:					
		case AE_EV_COMP_n07:				   
		case AE_EV_COMP_n03:				   
		case AE_EV_COMP_03: 			
		case AE_EV_COMP_07: 
		case AE_EV_COMP_13:
			break;

	   #if 0//no need so much, back up
		case AE_EV_COMP_13:  //+4 EV
			SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0xdc, 0x40);
			break;  
		case AE_EV_COMP_10:  //+3 EV
			SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0xdc, 0x30);
			break;    
		case AE_EV_COMP_07:  //+2 EV
			SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0xdc, 0x20);
			break;    
		case AE_EV_COMP_03:	 //	+1 EV	
			SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0xdc, 0x10);
			break;    
		case AE_EV_COMP_00:  // +0 EV
		    SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0xdc, 0x00);
		case AE_EV_COMP_n03:  // -1 EV
			SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0xdc, 0xf0);
			break;    
		case AE_EV_COMP_n07:	// -2 EV		
			SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0xdc, 0xe0);
			break;    
		case AE_EV_COMP_n10:   //-3 EV
			SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0xdc, 0xd0);
			break;
		case AE_EV_COMP_n13:  // -4 EV
			SP0A19_write_cmos_sensor(0xfd, 0x00);
			SP0A19_write_cmos_sensor(0xdc, 0xc0);
			break;
		#endif
		default:
			return KAL_FALSE;
	}
	SP0A19_CurrentStatus.iEV = para;
	return KAL_TRUE;
	
} 

static void SP0A19_set_AE_enable(kal_bool AE_enable)//hkm
{


}


UINT32 SP0A19YUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{
    switch (iCmd) {
    case FID_AWB_MODE:
        SP0A19_set_param_wb(iPara);
        break;
    case FID_COLOR_EFFECT:
        SP0A19_set_param_effect(iPara);
        break;
    case FID_AE_EV:
        SP0A19_set_param_exposure(iPara);
        break;
    case FID_AE_FLICKER:
        SP0A19_set_param_banding(iPara);
		break;
	case FID_SCENE_MODE:
		SP0A19NightMode(iPara);
        break;
	case FID_AE_SCENE_MODE:  
	     SP0A19_set_AE_enable(iPara);
	break; 
		
	case FID_ZOOM_FACTOR:
		zoom_factor = iPara; 
		break; 
    default:
        break;
    }
    return TRUE;
} /* SP0A19YUVSensorSetting */


UINT32 SP0A19YUVSetVideoMode(UINT16 u2FrameRate)//hkm
{

	if(SP0A19_CurrentStatus.iFrameRate == u2FrameRate)
	  return ERROR_NONE;

///
///
///

}


/*************************************************************************
* FUNCTION
*    AP0A19GetEvAwbRef
*
* DESCRIPTION
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void SP0A19GetEvAwbRef(UINT32 pSensorAEAWBRefStruct)//hkm
{
    PSENSOR_AE_AWB_REF_STRUCT Ref = (PSENSOR_AE_AWB_REF_STRUCT)pSensorAEAWBRefStruct;
    SENSORDB("AP0A19GetEvAwbRef  \n" );
    	
	Ref->SensorAERef.AeRefLV05Shutter = 5422;
    Ref->SensorAERef.AeRefLV05Gain = 478; /* 128 base */
    Ref->SensorAERef.AeRefLV13Shutter = 80;
    Ref->SensorAERef.AeRefLV13Gain = 128; /*  128 base */
    Ref->SensorAwbGainRef.AwbRefD65Rgain = 186; /* 128 base */
    Ref->SensorAwbGainRef.AwbRefD65Bgain = 158; /* 128 base */
    Ref->SensorAwbGainRef.AwbRefCWFRgain = 196; /* 1.25x, 128 base */
    Ref->SensorAwbGainRef.AwbRefCWFBgain = 278; /* 1.28125x, 128 base */
}

kal_uint16 SP0A19ReadGain(void)
{
    kal_uint16 temp=0x0000,Base_gain=64;

#if 0	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002c,0x7000);
    S5K8AAYX_MIPI_write_cmos_sensor(0x002e,0x20D0);//AGain
    
	temp=S5K8AAYX_MIPI_read_cmos_sensor(0x0f12);
	temp = Base_gain*temp/256;
#endif

	return temp;
}
kal_uint16 SP0A19ReadAwbRGain(void)
{
    kal_uint16 temp=0x0000,Base_gain=64;
#if 0	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002c,0x7000);
    S5K8AAYX_MIPI_write_cmos_sensor(0x002e,0x20DC);

	temp=S5K8AAYX_MIPI_read_cmos_sensor(0x0f12);
	temp = Base_gain*temp/1024;
#endif
	return temp;
}
kal_uint16 SP0A19ReadAwbBGain(void)
{
    kal_uint16 temp=0x0000,Base_gain=64;
#if 0	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002c,0x7000);
    S5K8AAYX_MIPI_write_cmos_sensor(0x002e,0x20E0);

	temp=S5K8AAYX_MIPI_read_cmos_sensor(0x0f12);
	temp = Base_gain*temp/1024;
#endif
	return temp;
}


/*************************************************************************
* FUNCTION
*    AP0A19GetCurAeAwbInfo
*
* DESCRIPTION
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void SP0A19GetCurAeAwbInfo(UINT32 pSensorAEAWBCurStruct)//hkm
{
    PSENSOR_AE_AWB_CUR_STRUCT Info = (PSENSOR_AE_AWB_CUR_STRUCT)pSensorAEAWBCurStruct;
    SENSORDB("AP0A19GetCurAeAwbInfo  \n" );

    Info->SensorAECur.AeCurShutter = SP0A19_Read_Shutter();
    Info->SensorAECur.AeCurGain = SP0A19ReadGain()<<1; /* 128 base */
    
    Info->SensorAwbGainCur.AwbCurRgain = SP0A19ReadAwbRGain()<< 1; /* 128 base */
    
    Info->SensorAwbGainCur.AwbCurBgain = SP0A19ReadAwbBGain()<< 1; /* 128 base */
}


void SP0A19GetAFMaxNumFocusAreas(UINT32 *pFeatureReturnPara32)
{	
    *pFeatureReturnPara32 = 0;    
    SENSORDB("SP0A19GetAFMaxNumFocusAreas, *pFeatureReturnPara32 = %d\n",*pFeatureReturnPara32);

}


void SP0A19GetAFMaxNumMeteringAreas(UINT32 *pFeatureReturnPara32)
{	
    *pFeatureReturnPara32 = 0;    
    SENSORDB("SP0A19GetAFMaxNumMeteringAreas,*pFeatureReturnPara32 = %d\n",*pFeatureReturnPara32);

}


void SP0A19GetExifInfo(UINT32 exifAddr)
{
    SENSOR_EXIF_INFO_STRUCT* pExifInfo = (SENSOR_EXIF_INFO_STRUCT*)exifAddr;
    pExifInfo->FNumber = 28;
    pExifInfo->AEISOSpeed = AE_ISO_100;
    pExifInfo->AWBMode = SP0A19_CurrentStatus.iWB;
    pExifInfo->CapExposureTime = 0;
    pExifInfo->FlashLightTimeus = 0;
    pExifInfo->RealISOValue = AE_ISO_100;
}


void SP0A19GetDelayInfo(UINT32 delayAddr)
{
    SENSOR_DELAY_INFO_STRUCT* pDelayInfo = (SENSOR_DELAY_INFO_STRUCT*)delayAddr;
    pDelayInfo->InitDelay = 3;
    pDelayInfo->EffectDelay = 0;
    pDelayInfo->AwbDelay = 4;
}


void SP0A19GetAEAWBLock(UINT32 *pAElockRet32,UINT32 *pAWBlockRet32)
{
    *pAElockRet32 = 1;
	*pAWBlockRet32 = 1;
    SENSORDB("SP0A19GetAEAWBLock,AE=%d ,AWB=%d\n,",*pAElockRet32,*pAWBlockRet32);
}


UINT32 SP0A19SetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;
		
	SENSORDB("SP0A19SetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk = SP0A19_PCLK;
			lineLength = SP0A19_VGA_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - SP0A19_VGA_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			SENSORDB("SP0A19SetMaxFramerateByScenario MSDK_SCENARIO_ID_CAMERA_PREVIEW: lineLength = %d, dummy=%d\n",lineLength,dummyLine);
			SP0A19_set_dummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pclk = SP0A19_PCLK;
			lineLength = SP0A19_VGA_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - SP0A19_VGA_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			SENSORDB("SP0A19SetMaxFramerateByScenario MSDK_SCENARIO_ID_VIDEO_PREVIEW: lineLength = %d, dummy=%d\n",lineLength,dummyLine);			
			SP0A19_set_dummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
			pclk = SP0A19_PCLK;
			lineLength = SP0A19_VGA_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - SP0A19_VGA_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			SENSORDB("SP0A19SetMaxFramerateByScenario MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG: lineLength = %d, dummy=%d\n",lineLength,dummyLine);			
			SP0A19_set_dummy(0, dummyLine);			
			break;		
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
			break;
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			break;		
		default:
			break;
	}	

	return ERROR_NONE;
}



UINT32 SP0A19GetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 *pframeRate = 200;//hkm
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 *pframeRate = 200;
			break;		
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			 *pframeRate = 200;
			break;		
		default:
			break;
	}

	return ERROR_NONE;
}


UINT32 SP0A19FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
        UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    UINT32 SP0A19SensorRegNumber;
    UINT32 i;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;


    switch (FeatureId)
    {
    case SENSOR_FEATURE_GET_RESOLUTION:
        *pFeatureReturnPara16++=SP0A19_IMAGE_SENSOR_FULL_WIDTH;
        *pFeatureReturnPara16=SP0A19_IMAGE_SENSOR_FULL_HEIGHT;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PERIOD:
        //*pFeatureReturnPara16++=SP0A19_VGA_PERIOD_PIXEL_NUMS+SP0A19_dummy_pixels;
        //*pFeatureReturnPara16=SP0A19_VGA_PERIOD_LINE_NUMS+SP0A19_dummy_lines;
        //*pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        *pFeatureReturnPara32 = SP0A19_PCLK;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_ESHUTTER:
        break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
        SP0A19NightMode((BOOL) *pFeatureData16);
        break;

	case SENSOR_FEATURE_SET_VIDEO_MODE:
		SP0A19YUVSetVideoMode(*pFeatureData16);//hkm
		break; 
		
    case SENSOR_FEATURE_SET_GAIN:
    case SENSOR_FEATURE_SET_FLASHLIGHT:
        break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
        SP0A19_isp_master_clock=*pFeatureData32;
        break;
    case SENSOR_FEATURE_SET_REGISTER:
        SP0A19_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
        break;
    case SENSOR_FEATURE_GET_REGISTER:
        pSensorRegData->RegData = SP0A19_read_cmos_sensor(pSensorRegData->RegAddr);
        break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
        memcpy(pSensorConfigData, &SP0A19SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
        *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
        break;
    case SENSOR_FEATURE_SET_CCT_REGISTER:
    case SENSOR_FEATURE_GET_CCT_REGISTER:
    case SENSOR_FEATURE_SET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
    case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
    case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
    case SENSOR_FEATURE_GET_GROUP_INFO:
    case SENSOR_FEATURE_GET_ITEM_INFO:
    case SENSOR_FEATURE_SET_ITEM_INFO:
    case SENSOR_FEATURE_GET_ENG_INFO:
        break;
	case SENSOR_FEATURE_GET_GROUP_COUNT:
        *pFeatureReturnPara32++=0;
        *pFeatureParaLen=4;	   
		break;
		
    case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
        // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
        // if EEPROM does not exist in camera module.
        *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_YUV_CMD:
        SP0A19YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
        break;
    case SENSOR_FEATURE_CHECK_SENSOR_ID:
		SP0A19GetSensorID(pFeatureData32);
		break;

	case SENSOR_FEATURE_GET_EV_AWB_REF:
		 SP0A19GetEvAwbRef(*pFeatureData32);
		break;

	case SENSOR_FEATURE_GET_SHUTTER_GAIN_AWB_GAIN:
		 SP0A19GetCurAeAwbInfo(*pFeatureData32);	
		 break;

	case SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS:
		 SP0A19GetAFMaxNumFocusAreas(pFeatureReturnPara32);		   
		 *pFeatureParaLen=4;
		 break;	   

	case SENSOR_FEATURE_GET_AE_MAX_NUM_METERING_AREAS:
		 SP0A19GetAFMaxNumMeteringAreas(pFeatureReturnPara32);			  
		 *pFeatureParaLen=4;
		 break;

	case SENSOR_FEATURE_GET_EXIF_INFO:
		SENSORDB("SENSOR_FEATURE_GET_EXIF_INFO\n");
		SENSORDB("EXIF addr = 0x%x\n",*pFeatureData32); 		 
		SP0A19GetExifInfo(*pFeatureData32);
		break;

	case SENSOR_FEATURE_GET_DELAY_INFO:
		SENSORDB("SENSOR_FEATURE_GET_DELAY_INFO\n");
		SP0A19GetDelayInfo(*pFeatureData32);
		break;

	case SENSOR_FEATURE_GET_AE_AWB_LOCK_INFO:
		SP0A19GetAEAWBLock((*pFeatureData32),*(pFeatureData32+1));
		break;

	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		SP0A19SetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		SP0A19GetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
		break;
    default:
        break;
	}
return ERROR_NONE;
}	/* SP0A19FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncSP0A19YUV=
{
	SP0A19Open,
	SP0A19GetInfo,
	SP0A19GetResolution,
	SP0A19FeatureControl,
	SP0A19Control,
	SP0A19Close
};


UINT32 SP0A19_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncSP0A19YUV;
	return ERROR_NONE;
} /* SensorInit() */
