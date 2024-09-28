/****************************************
History:

2. lidong.zhou@ragentek.com    2013.4.2
   add lcm compare by adc	

1. lidong.zhou@ragentek.com    2013.3.22
   Description: add mipi  djn otm8018b

zhoulidong  add
---------------------------------------------------------------------------------
VENDOR	|   Driver IC   |    LCD_ID	|  Module N/O	        |   Range	|
---------------------------------------------------------------------------------
DJN	|   ILI9806C	|	0.9V	|90-24025-4177A 	|   0.8-1.0	|	
---------------------------------------------------------------------------------
TRULY   |   OTM8018	|	0.6V	|BSHMDS9401B	        |   0.5-0.7	|
---------------------------------------------------------------------------------
TRUST   |   OTM8018	|	0V	|LCM-T040SWV636MH       |		|
---------------------------------------------------------------------------------
AZET    |   OTM8018	|	1.2V	|MS-A40213N00QO-A       |		|
---------------------------------------------------------------------------------
QJ      |   ILI9806C	|	1.5V	|     			|  1.4-1.6	|
---------------------------------------------------------------------------------
****************************************/

#ifdef BUILD_LK
#include "platform/mt_gpio.h"
#else
#include <linux/string.h>
#if defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_gpio.h>
#endif
#endif
#include "lcm_drv.h"
// ---------------------------------------------------------------------------
//RGK add
// ---------------------------------------------------------------------------
#include <cust_adc.h>    	// zhoulidong  add for lcm detect
#define MIN_VOLTAGE (800)     // zhoulidong  add for lcm detect
#define MAX_VOLTAGE (1000)     // zhoulidong  add for lcm detect
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(480)
#define FRAME_HEIGHT 										(800)

#define REGFLAG_DELAY             							0XAB
#define REGFLAG_END_OF_TABLE      							0xAA  // END OF REGISTERS MARKER

#define LCM_ID_OTM8018B									0x8009

#define LCM_DSI_CMD_MODE									0

#ifndef TRUE
    #define   TRUE     1
#endif
 
#ifndef FALSE
    #define   FALSE    0
#endif
static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

// zhoulidong  add for lcm detect ,read adc voltage
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);

 struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {

	{0xff, 3,{0x80,0x09,0x01}},

	{0x00, 1,{0x80}},
	{0xff, 2,{0x80,0x09}},

	{0x00, 1,{0x80}},
	{0xf5,12,{0x01,0x18,0x02,0x18,0x10,0x18,0x02,0x18,0x0e,0x18,0x0f,0x20}},

	{0x00, 1,{0x90}},
	{0xf5,10,{0x02,0x18,0x08,0x18,0x06,0x18,0x0d,0x18,0x0b,0x18}},

	{0x00, 1,{0xa0}},
	{0xf5, 8,{0x10,0x18,0x01,0x18,0x14,0x18,0x14,0x18}},

	{0x00, 1,{0xb0}},
	{0xf5,12,{0x14,0x18,0x12,0x18,0x13,0x18,0x11,0x18,0x13,0x18,0x00,0x00}},

	{0x00, 1,{0x80}},
	{0xc4, 1,{0x30}},

	{0x00, 1,{0x80}},
	{0xc0, 6,{0x00,0x5F,0x00,0x0A,0x0A,0x00}},

//14

	{0x00, 1,{0xb4}},
	{0xc0, 1,{0x10}},//column

	{0x00, 1,{0x82}},
	{0xc5, 1,{0xa3}},
//20
	{0x00, 1,{0x90}},
	{0xc5, 2,{0xd6,0x76}},  //2012.10.08 seting ,2x ,VGH=18,VGL=-10

	{0x00, 1,{0x00}},
	{0xd8, 2,{0x6F,0x6F}}, // GVDD+5v_97 / GVDD-5v

	{0x00, 1,{0x00}},
	{0xd9, 1,{0x5a}}, // Vcom  8B   5f

	{0x00, 1,{0x81}},
	{0xc1, 1,{0x66}}, //60hz_55/70hz_77


	{0x00, 1,{0xa1}},
	{0xc1, 1,{0x08}},

	{0x00, 1,{0xa3}},
	{0xc0, 1,{0x1B}},

	{0x00, 1,{0x81}},
	{0xc4, 1,{0x83}},


	{0x00, 1,{0x92}},
	{0xc5, 1,{0x00}},//01

	{0x00, 1,{0xb1}},
	{0xc5, 1,{0xa9}},

	{0x00, 1,{0x00}},
	{0x00, 1,{0x00}},


	{0x00, 1,{0x90}},
	{0xc0, 6,{0x00,0x56,0x00,0x00,0x00,0x03}},

	{0x00, 1,{0xa6}},
	{0xc1, 3,{0x00,0x00,0x00}},

	{0x00, 1,{0x80}},
	{0xce, 6,{0x89,0x03,0x1c,0x88,0x03,0x1c}},

	{0x00, 1,{0xa0}},
	{0xce,14,{0x38,0x07,0x03,0x24,0x00,0x1c,0x00,0x38,0x06,0x03,0x25,0x00,0x1c,0x00}},

	{0x00, 1,{0xb0}},
	{0xce,14,{0x38,0x05,0x03,0x26,0x00,0x1c,0x00,0x38,0x04,0x03,0x28,0x00,0x1c,0x00}},

	{0x00, 1,{0xc0}},
	{0xce,14,{0x38,0x03,0x03,0x20,0x00,0x1c,0x00,0x38,0x02,0x03,0x21,0x00,0x1c,0x00}},

	{0x00, 1,{0xd0}},
	{0xce,14,{0x38,0x01,0x03,0x22,0x00,0x1c,0x00,0x38,0x00,0x03,0x23,0x00,0x1c,0x00}},


	{0x00, 1,{0xc0}},
	{0xcf,10,{0x02,0x02,0x20,0x20,0x00,0x00,0x00,0x80,0x20,0x0A}},//GATE on

	{0x00, 1,{0xc1}},
	{0xcb, 4,{0x54,0x54,0x54,0x54}},
	{0x00, 1,{0xc6}},
	{0xcb, 3,{0x54,0x54,0x54}},
	{0x00, 1,{0xca}},
	{0xcb, 1,{0x54}},
	{0x00, 1,{0xd6}},
	{0xcb, 4,{0x54,0x54,0x54,0x54}},
	{0x00, 1,{0xdb}},
	{0xcb, 3,{0x54,0x54,0x54}},
	{0x00, 1,{0xe0}},
	{0xcb, 1,{0x54}},
	{0x00, 1,{0x81}},
	{0xcc, 4,{0x10,0x0e,0x0c,0x0a}},
	{0x00, 1,{0x86}},
	{0xcc, 3,{0x02,0x00,0x21}},
	{0x00, 1,{0x90}},
	{0xcc, 1,{0x22}},
	{0x00, 1,{0x9b}},
	{0xcc, 4,{0x0f,0x0d,0x0b,0x09}},
	{0x00, 1,{0xa1}},
	{0xcc, 3,{0x01,0x00,0x21}},
	{0x00, 1,{0xa5}},
	{0xcc, 1,{0x22}},
	{0x00, 1,{0xb1}},
	{0xcc, 4,{0x09,0x0b,0x0d,0x0f}},
	{0x00, 1,{0xb6}},
	{0xcc, 3,{0x00,0x01,0x21}},
	{0x00, 1,{0xc0}},
	{0xcc, 1,{0x22}},
	{0x00, 1,{0xcb}},
	{0xcc, 4,{0x0a,0x0c,0x0e,0x10}},
	{0x00, 1,{0xd1}},
	{0xcc, 3,{0x00,0x02,0x21}},
	{0x00, 1,{0xd5}},
	{0xcc, 1,{0x22}},


	{0x00, 1,{0x00}},
	{0xe1,16,{0x01,0x07,0x12,0x0F,0x09,0x1D,0x0F,0x0D,0x00,0x04,0x02,0x07,0x0F,0x24,0x20,0x0A}},


	{0x00, 1,{0x00}},
	{0xe2,16,{0x01,0x07,0x12,0x0F,0x09,0x1D,0x0F,0x0D,0x00,0x04,0x02,0x07,0x0F,0x24,0x20,0x0A}},

        {0x00, 1,{0x00}},
	{0xff, 3,{0xff,0xff,0xff}},

	///{0x00, 1,{0x00}},
	///{0x35, 1,{0x00}},
	{0x00, 1,{0x00}},
	{0x3a, 1,{0x77}},
	
//	{0x21,1,{0x00}},

	{0x11,1,{0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x29,1,{0x00}},
	{REGFLAG_DELAY, 50, {}},

};

/*

static struct LCM_setting_table lcm_set_window[] = {
	{0x2A,	4,	{0x00, 0x00, (FRAME_WIDTH>>8), (FRAME_WIDTH&0xFF)}},
	{0x2B,	4,	{0x00, 0x00, (FRAME_HEIGHT>>8), (FRAME_HEIGHT&0xFF)}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 1, {0x00}},
    {REGFLAG_DELAY, 200, {}},

    // Display ON
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 50, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
*/

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 1, {0x00}},
	{REGFLAG_DELAY, 50, {}},

    // Sleep Mode On
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 200, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

/*
static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{0x51, 1, {0xFF}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
*/

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
       	}
    }
	
}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{

    memset(params, 0, sizeof(LCM_PARAMS));
    
    params->type   = LCM_TYPE_DSI;
    
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

    // enable tearing-free
   // params->dbi.te_mode				= LCM_DBI_TE_MODE_VSYNC_ONLY;
    //params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
    
    params->dsi.mode   = SYNC_PULSE_VDO_MODE;
    
    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM				= LCM_TWO_LANE;
   
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq	= LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding 	= LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format	  = LCM_DSI_FORMAT_RGB888;
    
    // Video mode setting		
    params->dsi.intermediat_buffer_num = 2;
    
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    
    params->dsi.word_count=480*3;	//DSI CMD mode need set these two bellow params, different to 6577
    params->dsi.vertical_active_line=800;

    params->dsi.vertical_sync_active				= 4;
    params->dsi.vertical_backporch					= 16;
    params->dsi.vertical_frontporch					= 15;
    params->dsi.vertical_active_line				= FRAME_HEIGHT;
    
    params->dsi.horizontal_sync_active				= 6;
    params->dsi.horizontal_backporch				= 37;
    params->dsi.horizontal_frontporch				= 37;
   // params->dsi.horizontal_blanking_pixel				= 60;
    params->dsi.horizontal_active_pixel				= FRAME_WIDTH;
    
    // Bit rate calculation

    params->dsi.pll_div1=1;		// div1=0,1,2,3;div1_real=1,2,4,4
    params->dsi.pll_div2=1;		// div2=0,1,2,3;div2_real=1,2,4,4
    params->dsi.fbk_sel=1;		 // fbk_sel=0,1,2,3;fbk_sel_real=1,2,4,4
    params->dsi.fbk_div =0x1A;  		// fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)		



}


static void lcm_init(void)
{
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	
	SET_RESET_PIN(1);     
        MDELAY(10);
        SET_RESET_PIN(0);
        MDELAY(10);
        SET_RESET_PIN(1);
        MDELAY(100);
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
         
}


static void lcm_resume(void)
{
	lcm_init();
	
//	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}

/*
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;

	dsi_set_cmdq(data_array, 7, 0);

}


static void lcm_setbacklight(unsigned int level)
{
	unsigned int default_level = 145;
	unsigned int mapped_level = 0;

	//for LGE backlight IC mapping table
	if(level > 255) 
			level = 255;

	if(level >0) 
			mapped_level = default_level+(level)*(255-default_level)/(255);
	else
			mapped_level=0;

	// Refresh value of backlight level.
	lcm_backlight_level_setting[0].para_list[0] = mapped_level;

	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}

*/

// zhoulidong  add for lcm detect (start)
static unsigned int lcm_compare_id(void)
{
	int array[4];
	char buffer[5];
	char id_high=0;
	char id_low=0;
	int id=0;

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(200);

	array[0] = 0x00053700;
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xa1, buffer, 5);

	id_high = buffer[2];
	id_low = buffer[3];
	id = (id_high<<8) | id_low;
	
	#ifdef BUILD_LK
		printf("OTM8018B uboot %s \n", __func__);
	        printf("%s id = 0x%08x \n", __func__, id);
	#else
		printk("OTM8018B kernel %s \n", __func__);
		printk("%s id = 0x%08x \n", __func__, id);
	#endif
	   
	return (LCM_ID_OTM8018B == id)?1:0;
}


static int rgk_lcm_compare_id(void)
{
    int data[4] = {0,0,0,0};
    int res = 0;
    int rawdata = 0;
    int lcm_vol = 0;

#ifdef AUXADC_LCM_VOLTAGE_CHANNEL
    res = IMM_GetOneChannelValue(AUXADC_LCM_VOLTAGE_CHANNEL,data,&rawdata);
    if(res < 0)
    { 
	#ifdef BUILD_LK
	printf("[adc_uboot]: get data error\n");
	#endif
	return 0;
		   
    }
#endif

    lcm_vol = data[0]*1000+data[1]*10;

    #ifdef BUILD_LK
    printf("[adc_uboot]: lcm_vol= %d\n",lcm_vol);
    #endif

    if (lcm_vol>=MIN_VOLTAGE &&lcm_vol <= MAX_VOLTAGE)
    {
	return 1;
    }

    return 0;
	
}

// zhoulidong  add for lcm detect (end)

// zhoulidong add for eds(start)
static unsigned int lcm_esd_check(void)
{
	#ifdef BUILD_LK
		//printf("lcm_esd_check()\n");
	#else
		//printk("lcm_esd_check()\n");
	#endif 
 #ifndef BUILD_LK
	char  buffer[3];
	int   array[4];

	if(lcm_esd_test)
	{
		lcm_esd_test = FALSE;
		return TRUE;
	}

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x0a, buffer, 1);
	if(buffer[0]==0x9c)
	{
		//#ifdef BUILD_LK
		//printf("%s %d\n FALSE", __func__, __LINE__);
		//#else
		//printk("%s %d\n FALSE", __func__, __LINE__);
		//#endif
		return FALSE;
	}
	else
	{	
		//#ifdef BUILD_LK
		//printf("%s %d\n FALSE", __func__, __LINE__);
		//#else
		//printk("%s %d\n FALSE", __func__, __LINE__);
		//#endif		 
		return TRUE;
	}
 #endif

}

static unsigned int lcm_esd_recover(void)
{
	
	#ifdef BUILD_LK
		printf("lcm_esd_recover()\n");
	#else
		printk("lcm_esd_recover()\n");
	#endif	
	
	lcm_init();	

	return TRUE;
}
// zhoulidong add for eds(end)
LCM_DRIVER otm8018b_dsi_vdo_azet_md215_lcm_drv = 
{
    .name			= "otm8018b_dsi_vdo_azet_md215",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id    =  rgk_lcm_compare_id,
    .esd_check     = lcm_esd_check,
 	.esd_recover   = lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
	//.set_backlight	= lcm_setbacklight,
    //.update         = lcm_update,
#endif
};

