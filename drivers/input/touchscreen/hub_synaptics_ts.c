/* drivers/input/keyboard/synaptics_i2c_rmi.c
 *
 * Copyright (C) 2007 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <mach/gpio.h>
#include <linux/jiffies.h>
#include <linux/slab.h>

#include <linux/workqueue.h>	//20101221 seven.kim@lge.com to use real time work queue
#include <linux/delay.h> //20101221 seven.kim@lge.com to use mdelay
#include <linux/wakelock.h> 	//20102121 seven.kim@lge.com to use wake_lock

#ifdef CONFIG_LGE_DVFS
#include <linux/dvs_suite.h>
#endif	// CONFIG_LGE_DVFS

#include "synaptics_ts_firmware.h"
#include "synaptics_ts_firmware_lgit.h"
#define SYNAPTICS_SUPPORT_FW_UPGRADE

/*<sunggyun.yu@lge.com> enable this for printk message*/
//#define DEBUG

#if 0 //seven for debugging
#define pr_debug(fmt, ...) \
	printk(KERN_DEBUG pr_fmt(fmt), ##__VA_ARGS__)
#endif

#if 0 //seven open for test
#define FEATURE_LGE_TOUCH_MOVING_IMPROVE
#define FEATURE_LGE_TOUCH_JITTERING_IMPROVE
#endif //end of seven
#define FEATURE_LGE_TOUCH_GHOST_FINGER_IMPROVE
#define FEATURE_LGE_TOUCH_GRIP_SUPPRESSION //20101215 seven.kim@lge.com

#define FEATURE_LGE_TOUCH_REAL_TIME_WORK_QUEUE	//20101221 seven.kim@lge.com to use real time work queue
#define FEATURE_LGE_TOUCH_ESD_DETECT					//20101221 seven.kim@lge.com to detect Register change by ESD
/*===========================================================================
                DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains definitions for constants, macros, types, variables
and other items needed by this module.
===========================================================================*/

static struct workqueue_struct *synaptics_wq;
// 20100826 jh.koo@lge.com, for stable initialization [START_LGE]
static struct i2c_client *hub_ts_client = NULL;
// 20100826 jh.koo@lge.com, for stable initialization [END_LGE]

struct synaptics_ts_data {
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	int use_irq;
	bool has_relative_report;
	struct hrtimer timer;
	struct work_struct  work;
	uint16_t max[2];

	uint32_t flags;
	int reported_finger_count;
	int8_t sensitivity_adjust;
	int (*power)(int on);
// 20100504 jh.koo@lge.com, correction of finger space [START_LGE]
	unsigned int count;
	int x_lastpt;
	int y_lastpt;
// 20100504 jh.koo@lge.com, correction of finger space [END_LGE]	
	struct early_suspend early_suspend;
// 20100826 jh.koo@lge.com, for stable initialization [START_LGE]
	struct delayed_work init_delayed_work;
// 20100826 jh.koo@lge.com, for stable initialization [END_LGE]
	unsigned char product_value; //product_value=0:misung panel  product_value=1 : LGIT panel
};

// 20101203 kyungyoon.kim@lge.com, Touch KEY LED [START_LGE]
enum key_leds {
	MENU,
	HOME,
	BACK,
	SEARCH,
};
// 20101203 kyungyoon.kim@lge.com, Touch KEY LED [END_LGE]
static int init_stabled = -1;

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_ts_early_suspend(struct early_suspend *h);
static void synaptics_ts_late_resume(struct early_suspend *h);
#endif

/* [START] kim.youngmin@lge.com to avoid touch lockup in case of HW_Rev_B and C */
static uint32_t  Synaptics_Check_Touch_Interrupt_Status();
/* [END] kim.youngmin@lge.com to avoid touch lockup in case of HW_Rev_B and C */

// 20101203 kyungyoon.kim@lge.com, Touch KEY LED [START_LGE]
extern void touchkey_pressed(enum key_leds id);
extern unsigned int system_rev;
// 20101203 kyungyoon.kim@lge.com, Touch KEY LED [END_LGE]
#define TOUCH_INT_N_GPIO						35

/* 20110317 seven.kim@lge.com for mass product issue [START] */
extern int lcd_off_boot;
/* 20110317 seven.kim@lge.com for mass product issue [END] */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*                                                                         */
/*                                 Macros                                  */
/*                                                                         */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define TS_SNTS_GET_FINGER_STATE_0(finger_status_reg) \
		(finger_status_reg&0x03)
#define TS_SNTS_GET_FINGER_STATE_1(finger_status_reg) \
		((finger_status_reg&0x0C)>>2)
#define TS_SNTS_GET_FINGER_STATE_2(finger_status_reg) \
		((finger_status_reg&0x30)>>4)
#define TS_SNTS_GET_FINGER_STATE_3(finger_status_reg) \
      ((finger_status_reg&0xC0)>>6)
#define TS_SNTS_GET_FINGER_STATE_4(finger_status_reg) \
      (finger_status_reg&0x03)

#define TS_SNTS_GET_X_POSITION(high_reg, low_reg) \
		((int)(high_reg*0x10) + (int)(low_reg&0x0F))
#define TS_SNTS_GET_Y_POSITION(high_reg, low_reg) \
		((int)(high_reg*0x10) + (int)((low_reg&0xF0)/0x10))

#define TS_SNTS_HAS_PINCH(gesture_reg) \
		((gesture_reg&0x40)>>6)
#define TS_SNTS_HAS_FLICK(gesture_reg) \
		((gesture_reg&0x10)>>4)
#define TS_SNTS_HAS_DOUBLE_TAP(gesture_reg) \
		((gesture_reg&0x04)>>2)

#define TS_SNTS_GET_REPORT_RATE(device_control_reg) \
		((device_control_reg&0x40)>>6)
// 1st bit : '0' - Allow sleep mode, '1' - Full power without sleeping
// 2nd and 3rd bit : 0x00 - Normal Operation, 0x01 - Sensor Sleep
#define TS_SNTS_GET_SLEEP_MODE(device_control_reg) \
		(device_control_reg&0x07)


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*                                                                         */
/*                       CONSTANTS DATA DEFINITIONS                        */
/*                                                                         */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define TOUCH_EVENT_NULL						0
#define TOUCH_EVENT_BUTTON						1
#define TOUCH_EVENT_ABS							2



#define SYNAPTICS_FINGER_MAX					5  	// [20101111:geayoung.baek@lge.com] Max Touch 5EA


#define SYNAPTICS_TM1576_PRODUCT_ID				"TM1576"
#define SYNAPTICS_TM1576_RESOLUTION_X			1036
#define SYNAPTICS_TM1576_RESOLUTION_Y			1976
#define SYNAPTICS_TM1576_LCD_ACTIVE_AREA		1728
#define SYNAPTICS_TM1576_BUTTON_ACTIVE_AREA		1828

#define SYNAPTICS_TM1702_PRODUCT_ID				"TM1702"
#define SYNAPTICS_TM1702_RESOLUTION_X			1036
#define SYNAPTICS_TM1702_RESOLUTION_Y			1896
#define SYNAPTICS_TM1702_LCD_ACTIVE_AREA		1728
#define SYNAPTICS_TM1702_BUTTON_ACTIVE_AREA		1805

#define SYNAPTICS_TM1738_PRODUCT_ID				"TM1738"
#define SYNAPTICS_TM1738_RESOLUTION_X			1036
#define SYNAPTICS_TM1738_RESOLUTION_Y			1896
#define SYNAPTICS_TM1738_LCD_ACTIVE_AREA			1728
#define SYNAPTICS_TM1738_BUTTON_ACTIVE_AREA		1805



/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*                                                                         */
/*                    REGISTER ADDR & SETTING VALUE                        */
/*                                                                         */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define SYNAPTICS_FLASH_CONTROL_REG				0x12
#define SYNAPTICS_DATA_BASE_REG					0x13
#define SYNAPTICS_INT_STATUS_REG				0x14

#define SYNAPTICS_2D_GESTURE_FINGER_0			0x4B	//20110211 seven added to prevent palm gesture
#define SYNAPTICS_CONTROL_REG						0x4C	//20101221 seven changed 0x4F -> 0x4C
#define SYNAPTICS_RIM_CONTROL_INTERRUPT_ENABLE	0x4D	//20101227 seven added to prevent interrupt in booting time

#define SYNAPTICS_DELTA_X_THRES_REG				0x50 	//20101221 seven changed 0x53 -> 0x50
#define SYNAPTICS_DELTA_Y_THRES_REG				0x51 	//20101221 seven changed 0x53 -> 0x51

#define SYNAPTICS_2D_GESTURE_ENABLES_2			0x59	//20110211 seven added to prevent palm gesture

#define SYNAPTICS_SENSITIVITY_ADJUSTMENT			0x9A	//20110110 seven added for touch sensing test

#define SYNAPTICS_FW_REVISION_REG				0xAD

#define SYNAPTICS_RMI_QUERY_BASE_REG			0xE3
#define SYNAPTICS_RMI_CMD_BASE_REG				0xE4
#define SYNAPTICS_FLASH_QUERY_BASE_REG			0xE9
#define SYNAPTICS_FLASH_DATA_BASE_REG			0xEC

#define SYNAPTICS_INT_FLASH						1<<0
#define SYNAPTICS_INT_STATUS					1<<1
#define SYNAPTICS_INT_ABS0						1<<2

#define SYNAPTICS_CONTROL_SLEEP					1<<0
#define SYNAPTICS_CONTROL_NOSLEEP				1<<2

#ifdef FEATURE_LGE_TOUCH_ESD_DETECT //20101221 seven.kim@lge.com to detect Register change by ESD
#define SYNAPTICS_CONTROL_CONFIGURED				1<<7
#define SYNAPTICS_RIM_DEVICE_RESET				1<<0
#endif /*FEATURE_LGE_TOUCH_ESD_DETECT*/

#ifdef SYNAPTICS_SUPPORT_FW_UPGRADE
#define SYNAPTICS_FLASH_CMD_FW_CRC				0x01
#define SYNAPTICS_FLASH_CMD_FW_WRITE			0x02
#define SYNAPTICS_FLASH_CMD_ERASEALL			0x03
#define SYNAPTICS_FLASH_CMD_CONFIG_READ			0x05
#define SYNAPTICS_FLASH_CMD_CONFIG_WRITE		0x06
#define SYNAPTICS_FLASH_CMD_CONFIG_ERASE		0x07
#define SYNAPTICS_FLASH_CMD_ENABLE				0x0F
#define SYNAPTICS_FLASH_NORMAL_RESULT			0x80
#define FW_IMAGE_SIZE 	28929
unsigned char SynapticsFirmware[FW_IMAGE_SIZE];
#endif /* SYNAPTICS_SUPPORT_FW_UPGRADE */


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*                                                                         */
/*                         DATA DEFINITIONS                                */
/*                                                                         */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

typedef struct {
	unsigned char m_QueryBase;
	unsigned char m_CommandBase;
	unsigned char m_ControlBase;
	unsigned char m_DataBase;
	unsigned char m_IntSourceCount;
	unsigned char m_FunctionExists;
} T_RMI4FuncDescriptor;




typedef struct
{
	unsigned char device_status_reg;						//0x13
	unsigned char interrupt_status_reg;					//0x14
	unsigned char finger_state_reg[3];					//0x15~0x17

	unsigned char fingers_data[SYNAPTICS_FINGER_MAX][5];	//0x18 ~ 0x49
	/* 5 data per 1 finger, support 10 fingers data
	fingers_data[x][0] : xth finger's X high position
	fingers_data[x][1] : xth finger's Y high position
	fingers_data[x][2] : xth finger's XY low position
	fingers_data[x][3] : xth finger's XY width
	fingers_data[x][4] : xth finger's Z (pressure)
	*/
	// Etc...
#if 0		// [20101027:geayoung.baek@lge.com] Not Used
	unsigned char gesture_flag0;							//0x4A
	unsigned char gesture_flag1;							//0x4B
	unsigned char pinch_motion_X_flick_distance;			//0x4C
	unsigned char rotation_motion_Y_flick_distance;		//0x4D
	unsigned char finger_separation_flick_time;			//0x4E
#endif	
} ts_sensor_data;

typedef struct {
	unsigned char touch_status[SYNAPTICS_FINGER_MAX];
	unsigned int X_position[SYNAPTICS_FINGER_MAX];
	unsigned int Y_position[SYNAPTICS_FINGER_MAX];
	unsigned char width[SYNAPTICS_FINGER_MAX];
	unsigned char pressure[SYNAPTICS_FINGER_MAX];
} ts_finger_data;


static ts_sensor_data ts_reg_data;
static ts_finger_data prev_ts_data;
static ts_finger_data curr_ts_data;


static uint8_t curr_event_type = TOUCH_EVENT_NULL;
static uint8_t prev_event_type = TOUCH_EVENT_NULL;

static uint16_t pressed_button_type = KEY_REJECT;

static uint16_t SYNAPTICS_PANEL_MAX_X;
static uint16_t SYNAPTICS_PANEL_MAX_Y;
static uint16_t SYNAPTICS_PANEL_LCD_MAX_Y;
static uint16_t SYNAPTICS_PANEL_BUTTON_MIN_Y;

unsigned char  touch_fw_version = 0;

struct wake_lock ts_wake_lock; //20101221 seven.kim@lge.com for ts recovery

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*                                                                         */
/*                           Local Functions                               */
/*                                                                         */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

// 20101022 joseph.jung@lge.com touch smooth moving improve [START]
#ifdef FEATURE_LGE_TOUCH_MOVING_IMPROVE
#define ADJUST_VAL				4
#define ADJUST_BASIS_VALUE		20
#define Square(x) ((x) * (x))

static void touch_adjust_position(int finger_num)
{
	if(prev_ts_data.touch_status[finger_num])
	{
		if( (Square(prev_ts_data.X_position[finger_num] - curr_ts_data.X_position[finger_num]) + Square(prev_ts_data.Y_position[finger_num] - curr_ts_data.Y_position[finger_num])) <= Square(ADJUST_BASIS_VALUE) )
		{
			curr_ts_data.X_position[finger_num] = (prev_ts_data.X_position[finger_num] * ADJUST_VAL + curr_ts_data.X_position[finger_num]) / (ADJUST_VAL + 1);
			curr_ts_data.Y_position[finger_num] = (prev_ts_data.Y_position[finger_num] * ADJUST_VAL + curr_ts_data.Y_position[finger_num]) / (ADJUST_VAL + 1);
		}
	}
}
#endif /* FEATURE_LGE_TOUCH_MOVING_IMPROVE */
// 20101022 joseph.jung@lge.com touch smooth moving improve [END]

#ifdef FEATURE_LGE_TOUCH_GHOST_FINGER_IMPROVE
#define MELT_CONTROL	0xF0
#define NO_MELT 				0x00
#define MELT					0x01
#define AUTO_MELT				0x10

static int melt_mode = 0;

//static int ts_pre_state = 0; /* for checking the touch state */
static int ghost_finger_1 = 0; // remove for ghost finger
static int ghost_finger_2 = 0;
static int pressed = 0;
static unsigned long pressed_time;
/* 20110225 seven.kim@lge.com to prevent ghost finger for HW Drop TEST [START] */
static int ghost_count = 0;
/* 20110225 seven.kim@lge.com to prevent ghost finger for HW Drop TEST [END] */
#endif

#ifdef FEATURE_LGE_TOUCH_ESD_DETECT //20101221 seven.kim@lge.com to detect Register change by ESD
extern  struct i2c_client *aat2870_i2c_client;
extern  void aat2870_touch_ldo_enable(struct i2c_client *client, int on);
static bool  Synatics_ts_touch_Recovery();
#endif /*FEATURE_LGE_TOUCH_ESD_DETECT*/

/* 20110216 seven.kim@lge.com to follow android sleep/resume flow [START] */
//static atomic_t g_synaptics_ts_resume_flag;
//static atomic_t g_synaptics_ts_suspend_flag;
/* 20110216 seven.kim@lge.com to follow android sleep/resume flow [END] */

/* [START] seven.kim@lge.com to recover in case of I2C Fail */
int g_touch_read_cnt = 0; 		//20110407 ATS TEST FAIL
int g_touch_restart_flag = 0;		//20110407 ATS TEST FAIL
s32 synaptics_ts_i2c_read_block_data(struct i2c_client *client, u8 command,
				  u8 length, u8 *values)
{
	s32 status, retry_count = 5;
	status = i2c_smbus_read_i2c_block_data(client, command, length, values);

	if (status < 0)
	{
		struct synaptics_ts_data *ts = i2c_get_clientdata(client);

#if 1 //20110309 seven for late_resume_lcd
		g_touch_read_cnt++;
		if(g_touch_restart_flag ==0 && g_touch_read_cnt > 3)
		{
			//printk("%s : Int Pin : %s \n",__func__, Synaptics_Check_Touch_Interrupt_Status()?"LOW":"HIGH");

			g_touch_restart_flag = 1;
		Synatics_ts_touch_Recovery();
			g_touch_restart_flag = 0;
			g_touch_read_cnt = 0;
			schedule_delayed_work(&ts->init_delayed_work, msecs_to_jiffies(400));
		}
#endif //20110309 seven for late_resume_lcd

	}
	else
		g_touch_read_cnt = 0;

	return status;
}

s32 synaptics_ts_i2c_read_byte_data(struct i2c_client *client, u8 command)
{
	s32 status, retry_count = 5;
	status = i2c_smbus_read_byte_data(client, command);

	if (status < 0)
	{
		struct synaptics_ts_data *ts = i2c_get_clientdata(client);
		printk("%s : I2C read byte fail ,  RECOVER !!\n",__func__);

#if 0 //20110309 seven for late_resume_lcd
		/* sensor recover when i2c fail */
		wake_lock(&ts_wake_lock);		
		Synatics_ts_touch_Recovery();
		wake_unlock(&ts_wake_lock);	

		schedule_delayed_work(&ts->init_delayed_work, msecs_to_jiffies(400));		
#endif //20110309 seven for late_resume_lcd				
	}
	
	return status;
}

s32 synaptics_ts_i2c_write_block_data(struct i2c_client *client, u8 command,
			       u8 length, const u8 *values)
{
	s32 status;
	
	status = i2c_smbus_write_block_data(client , command, length, values );
	
	if (status < 0)
	{
		struct synaptics_ts_data *ts = i2c_get_clientdata(client);
		printk("%s : I2C write block fail ,  RECOVER !!\n",__func__);

#if 0 //20110309 seven for late_resume_lcd
		/* sensor recover when i2c fail */
		wake_lock(&ts_wake_lock);		
		Synatics_ts_touch_Recovery();
		wake_unlock(&ts_wake_lock);	

		schedule_delayed_work(&ts->init_delayed_work, msecs_to_jiffies(400));		
#endif //20110309 seven for late_resume_lcd			
	}
	
	return status;
}
s32 synaptics_ts_i2c_write_byte_data(struct i2c_client *client, u8 command, u8 value)
{
	s32 status;
	
	status = i2c_smbus_write_byte_data(client, command, value); /* wake up */

	if (status < 0)
	{
		struct synaptics_ts_data *ts = i2c_get_clientdata(client);
		printk("%s : I2C read byte fail ,  RECOVER !!\n",__func__);

#if 0 //20110309 seven for late_resume_lcd
		/* sensor recover when i2c fail */
		wake_lock(&ts_wake_lock);		
		Synatics_ts_touch_Recovery();
		wake_unlock(&ts_wake_lock);	

		schedule_delayed_work(&ts->init_delayed_work, msecs_to_jiffies(400));		
#endif //20110309 seven for late_resume_lcd	
	}
	
	return status;
}
/* [END] kim.youngmin@lge.com to recover in case of I2C Fail */

// 20100826 jh.koo@lge.com, for stable initialization [START_LGE]
static void synaptics_ts_init_delayed_work(struct work_struct *work)
{
	struct synaptics_ts_data *ts = container_of(work, struct synaptics_ts_data, work);
	int ret;

//For_Resume_Speed	pr_warning("%s() : Touch Delayed Work : Start!!\n", __func__);

	/* [START] seven.kim@lge.com to avoid touch lockup in case of HW_Rev_B and C */
	//if (system_rev < 4)
	{
		uint32_t  pinValue = 0;
		pinValue = Synaptics_Check_Touch_Interrupt_Status();
		if(pinValue)
		{
			uint8_t dummy_read = 0;	
			ts_sensor_data tmp_ts_reg_data;

			synaptics_ts_i2c_read_block_data(hub_ts_client, SYNAPTICS_DATA_BASE_REG, sizeof(tmp_ts_reg_data), (u8 *)&tmp_ts_reg_data);
		}
	}
	/* [END] seven.kim@lge.com to avoid touch lockup in case of HW_Rev_B and C */
	
	//[START] 20101226 seven.kim@lge.com to prevent touch interrupt in booting time
	synaptics_ts_i2c_write_byte_data(hub_ts_client, SYNAPTICS_RIM_CONTROL_INTERRUPT_ENABLE, 0x00); //interrupt disable
	
	disable_irq(hub_ts_client->irq);
	//[END] 20101226 seven.kim@lge.com to prevent touch interrupt in booting time

	synaptics_ts_i2c_read_block_data(hub_ts_client, SYNAPTICS_DATA_BASE_REG, sizeof(ts_reg_data), (u8 *)&ts_reg_data);
	
	/* [START] 20110110 seven.kim@lge.com touch sensing test F/Up */
	if(system_rev < 4)
	{
	synaptics_ts_i2c_write_byte_data(hub_ts_client, SYNAPTICS_CONTROL_REG, SYNAPTICS_CONTROL_NOSLEEP); /* wake up */
		synaptics_ts_i2c_write_byte_data(hub_ts_client, SYNAPTICS_CONTROL_REG, SYNAPTICS_CONTROL_CONFIGURED );
	}
	else
	{
	synaptics_ts_i2c_write_byte_data(hub_ts_client, SYNAPTICS_CONTROL_REG, (SYNAPTICS_CONTROL_CONFIGURED | SYNAPTICS_CONTROL_NOSLEEP));
	//20110211 seven.kim@lge.com to disable palm detection [START]
	synaptics_ts_i2c_write_byte_data(hub_ts_client, SYNAPTICS_2D_GESTURE_ENABLES_2, 0x00);
	//20110211 seven.kim@lge.com to disable palm detection [end]
	}
	/* [START] 20110110 seven.kim@lge.com touch sensing test F/Up */

// 20101022 joseph.jung@lge.com touch smooth moving improve [START]
#ifdef FEATURE_LGE_TOUCH_MOVING_IMPROVE
	ret = synaptics_ts_i2c_write_byte_data(hub_ts_client, SYNAPTICS_DELTA_X_THRES_REG, 0x01);
	ret = synaptics_ts_i2c_write_byte_data(hub_ts_client, SYNAPTICS_DELTA_Y_THRES_REG, 0x01);
#else
	ret = synaptics_ts_i2c_write_byte_data(hub_ts_client, SYNAPTICS_DELTA_X_THRES_REG, /*0x03*/0x01);
	ret = synaptics_ts_i2c_write_byte_data(hub_ts_client, SYNAPTICS_DELTA_Y_THRES_REG, /*0x03*/0x01);
#endif /* FEATURE_LGE_TOUCH_MOVING_IMPROVE */
// 20101022 joseph.jung@lge.com touch smooth moving improve [END]
#ifdef FEATURE_LGE_TOUCH_GHOST_FINGER_IMPROVE
	synaptics_ts_i2c_write_byte_data(hub_ts_client, MELT_CONTROL, MELT);
//	pr_warning("Touch MELT\n");
#endif

	init_stabled = 1;

	//[START] 20101226 seven.kim@lge.com to prevent touch interrupt in booting time
	enable_irq(hub_ts_client->irq);
	synaptics_ts_i2c_write_byte_data(hub_ts_client, SYNAPTICS_RIM_CONTROL_INTERRUPT_ENABLE, 0x07); //interrupt enable
	//[END] 20101226 seven.kim@lge.com to prevent touch interrupt in booting time	

//For_Resume_Speed	pr_warning("%s() : Touch Delayed Work : End!!\n", __func__);
}
// 20100826 jh.koo@lge.com, for stable initialization [END_LGE]

// 20101215 seven@lge.com grip suppression [START]
#ifdef FEATURE_LGE_TOUCH_GRIP_SUPPRESSION
static int g_gripIgnoreRangeValue = 0;
static int g_receivedPixelValue = 0;

static int touch_ConvertPixelToRawData(int pixel)
{
	int result = 0;

	result = (SYNAPTICS_PANEL_MAX_X * pixel) /480;

	return result;
}

ssize_t touch_gripsuppression_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	sprintf(buf, "%d\n", g_receivedPixelValue);	
	pr_debug("[KERNEL] [TOUCH] SHOW (%d) \n", g_receivedPixelValue);

	return (ssize_t)(strlen(buf)+1);

}

ssize_t touch_gripsuppression_store(struct device *dev, struct device_attribute *attr, const char *buffer, size_t count)
{
	sscanf(buffer, "%d", &g_receivedPixelValue);
	g_gripIgnoreRangeValue = touch_ConvertPixelToRawData(g_receivedPixelValue);
	pr_debug("[KERNEL] [TOUCH] STORE  pixel(%d) Convet (%d) \n", g_receivedPixelValue, g_gripIgnoreRangeValue);
	
	return count;
}

DEVICE_ATTR(gripsuppression, 0644, touch_gripsuppression_show, touch_gripsuppression_store);
#endif /* FEATURE_LGE_TOUCH_GRIP_SUPPRESSION */
// 20101215 seven@lge.com grip suppression [END]

/* [START] kim.youngmin@lge.com To avoid touch sensor lock-up & data missing in touch release*/
static /*uint8_t*/uint32_t  Synaptics_Check_Touch_Interrupt_Status()
{
		uint32_t pinValue = 0;
		pinValue = gpio_get_value(TOUCH_INT_N_GPIO);

		return !pinValue;
}
/* [END] kim.youngmin@lge.com To avoid touch sensor lock-up & data missing in touch release*/

static void synaptics_ts_work_func(struct work_struct *work)
{
	struct synaptics_ts_data *ts = container_of(work, struct synaptics_ts_data, work);
	int i;
	int finger_count = 0;

	if(init_stabled != 1)
		return;

	pr_debug("[TOUCH] synaptics_ts_work_func() : START \n" );
	
/* [START] kim.youngmin@lge.com To avoid touch sensor lock-up & data missing in touch release */
do{	
/* [END] kim.youngmin@lge.com To avoid touch sensor lock-up & data missing in touch release */	
	synaptics_ts_i2c_read_block_data(ts->client, SYNAPTICS_DATA_BASE_REG, sizeof(ts_reg_data), (u8 *)&ts_reg_data);

	/*  [START] 20110211 kim.youngmin@lge.com to detect palm */
	#if 0 //seven blocked 20110225
	{
		s32 dummy_read_byte;
		dummy_read_byte = synaptics_ts_i2c_read_byte_data(ts->client, SYNAPTICS_2D_GESTURE_FINGER_0 );
	}
	#endif
	/*  [END] 20110211 kim.youngmin@lge.com to detect palm */
	
	if(ts_reg_data.interrupt_status_reg & SYNAPTICS_INT_ABS0)
	{
		for(i = 0; i < SYNAPTICS_FINGER_MAX; i++)
		{
			int check = 1 << ((i%4)*2);

			prev_ts_data.touch_status[i] = curr_ts_data.touch_status[i];
			prev_ts_data.X_position[i] = curr_ts_data.X_position[i];
			prev_ts_data.Y_position[i] = curr_ts_data.Y_position[i];
			prev_ts_data.width[i] = curr_ts_data.width[i];
			prev_ts_data.pressure[i] = curr_ts_data.pressure[i];
			
			if((ts_reg_data.finger_state_reg[i/4] & check) == check)
			{
				curr_ts_data.X_position[i] = (int)TS_SNTS_GET_X_POSITION(ts_reg_data.fingers_data[i][0], ts_reg_data.fingers_data[i][2]);
				curr_ts_data.Y_position[i] = (int)TS_SNTS_GET_Y_POSITION(ts_reg_data.fingers_data[i][1], ts_reg_data.fingers_data[i][2]);

// 20101022 joseph.jung@lge.com touch smooth moving improve [START]
#ifdef FEATURE_LGE_TOUCH_MOVING_IMPROVE
				touch_adjust_position(i);
#endif

#ifdef FEATURE_LGE_TOUCH_JITTERING_IMPROVE
				if(!(abs(curr_ts_data.X_position[i]-prev_ts_data.X_position[i]) > 1 && abs(curr_ts_data.Y_position[i]-prev_ts_data.Y_position[i]) > 1))
				{
					curr_ts_data.X_position[i] = prev_ts_data.X_position[i];
					curr_ts_data.Y_position[i] = prev_ts_data.Y_position[i];
				}
#endif
// 20101022 joseph.jung@lge.com touch smooth moving improve [END]

				#ifdef FEATURE_LGE_TOUCH_GRIP_SUPPRESSION // 20101215 seven@lge.com grip suppression [start]
				if ( (g_gripIgnoreRangeValue > 0) && ( (curr_ts_data.X_position[i] <= g_gripIgnoreRangeValue ) || 
															(curr_ts_data.X_position[i] >= (SYNAPTICS_PANEL_MAX_X - g_gripIgnoreRangeValue) )) )
				{
					pr_debug("[TOUCH] Girp Region Pressed. IGNORE!!!\n" );
				}
				else
				{
				#endif // 20101215 seven@lge.com grip suppression [start]

					if ((((ts_reg_data.fingers_data[i][3] & 0xf0) >> 4) - (ts_reg_data.fingers_data[i][3] & 0x0f)) > 0)
						curr_ts_data.width[i] = (ts_reg_data.fingers_data[i][3] & 0xf0) >> 4;
					else
						curr_ts_data.width[i] = ts_reg_data.fingers_data[i][3] & 0x0f;

					curr_ts_data.pressure[i] = ts_reg_data.fingers_data[i][4];

					curr_ts_data.touch_status[i] = 1;

					finger_count++;
				}
			}
			else
			{
				curr_ts_data.touch_status[i] = 0;
			}
		}
		
		for(i = 0; i < SYNAPTICS_FINGER_MAX; i++)
		{
			if(curr_ts_data.touch_status[i])
			{
				if(finger_count == 1 && i == 0)
				{

#ifdef FEATURE_LGE_TOUCH_GHOST_FINGER_IMPROVE
					 if(pressed == 0) {
					         pressed_time = jiffies;
					         ghost_finger_1 = 1;
					         pressed++;
					 }
#endif
					if((curr_ts_data.Y_position[i] < SYNAPTICS_PANEL_LCD_MAX_Y && prev_event_type == TOUCH_EVENT_NULL) || prev_event_type == TOUCH_EVENT_ABS)
						curr_event_type = TOUCH_EVENT_ABS;
					else if((curr_ts_data.Y_position[i] >= SYNAPTICS_PANEL_LCD_MAX_Y && prev_event_type == TOUCH_EVENT_NULL) || prev_event_type == TOUCH_EVENT_BUTTON)
						curr_event_type = TOUCH_EVENT_BUTTON;

					if(curr_event_type == TOUCH_EVENT_ABS)
					{
						if(curr_ts_data.Y_position[i] < SYNAPTICS_PANEL_LCD_MAX_Y)
						{
							input_report_abs(ts->input_dev, ABS_MT_POSITION_X, curr_ts_data.X_position[i]);
							input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, curr_ts_data.Y_position[i]);
							input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, curr_ts_data.pressure[i]);
							input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, curr_ts_data.width[i]);
							
							input_mt_sync(ts->input_dev);
							pr_debug("[TOUCH-1] (X, Y) = (%d, %d), z = %d, w = %d\n", curr_ts_data.X_position[i], curr_ts_data.Y_position[i], curr_ts_data.pressure[i], curr_ts_data.width[i]);
						}
					}
					else if(curr_event_type == TOUCH_EVENT_BUTTON)
					{
						if(curr_ts_data.Y_position[i] > SYNAPTICS_PANEL_BUTTON_MIN_Y)
						{
							if(curr_ts_data.X_position[i] > 35 && curr_ts_data.X_position[i] < 245) //center 75
							{
								if(!prev_ts_data.touch_status[i])
								{
									input_report_key(ts->input_dev, KEY_MENU, 1); //seven blocked for key drag action
									pr_debug("[TOUCH-2] Key Event KEY = %d, PRESS = %d\n", KEY_MENU, 1);
									pressed_button_type = KEY_MENU;
								}
								else
								{
									if(pressed_button_type != KEY_MENU && pressed_button_type != KEY_REJECT)
									{
										input_report_key(ts->input_dev, KEY_REJECT, 1);
										pr_debug("[TOUCH-3] Key Event KEY = %d, PRESS = %d\n", KEY_REJECT, 1);
										input_report_key(ts->input_dev, KEY_REJECT, 0);
										pr_debug("[TOUCH] Key Event KEY = %d, PRESS = %d\n", KEY_REJECT, 0);
										input_report_key(ts->input_dev, pressed_button_type, 0);
										pr_debug("[TOUCH-4] Key Event KEY = %d, PRESS = %d\n", pressed_button_type, 0);
										pressed_button_type = KEY_REJECT;
									}
								}
							}
							else if(curr_ts_data.X_position[i] > 287 && curr_ts_data.X_position[i] < 497) //center 185
							{
								if(!prev_ts_data.touch_status[i])
								{
									input_report_key(ts->input_dev, KEY_HOME, 1); //seven blocked for key drag action
									pr_debug("[TOUCH-5] Key Event KEY = %d, PRESS = %d\n", KEY_HOME, 1);
									pressed_button_type = KEY_HOME;
								}
								else
								{
									if(pressed_button_type != KEY_HOME && pressed_button_type != KEY_REJECT)
									{
										input_report_key(ts->input_dev, KEY_REJECT, 1);
										pr_debug("[TOUCH-6] Key Event KEY = %d, PRESS = %d\n", KEY_REJECT, 1);
										input_report_key(ts->input_dev, KEY_REJECT, 0);
										pr_debug("[TOUCH-6] Key Event KEY = %d, PRESS = %d\n", KEY_REJECT, 0);
										input_report_key(ts->input_dev, pressed_button_type, 0);
										pr_debug("[TOUCH-8] Key Event KEY = %d, PRESS = %d\n", pressed_button_type, 0);
										pressed_button_type = KEY_REJECT;
									}
								}
							}
							else if(curr_ts_data.X_position[i] > 539 && curr_ts_data.X_position[i] < 749) //center 295
							{
								if(!prev_ts_data.touch_status[i])
								{
									input_report_key(ts->input_dev, KEY_BACK, 1); //seven blocked for key drag action
									pr_debug("[TOUCH-9] Key Event KEY = %d, PRESS = %d\n", KEY_BACK, 1);
									pressed_button_type = KEY_BACK;
								}
								else
								{
									if(pressed_button_type != KEY_BACK && pressed_button_type != KEY_REJECT)
									{
										input_report_key(ts->input_dev, KEY_REJECT, 1);
										pr_debug("[TOUCH-10] Key Event KEY = %d, PRESS = %d\n", KEY_REJECT, 1);
										input_report_key(ts->input_dev, KEY_REJECT, 0);
										pr_debug("[TOUCH-11] Key Event KEY = %d, PRESS = %d\n", KEY_REJECT, 0);
										input_report_key(ts->input_dev, pressed_button_type, 0);
										pr_debug("[TOUCH-12] Key Event KEY = %d, PRESS = %d\n", pressed_button_type, 0);
										pressed_button_type = KEY_REJECT;
									}
								}
							}
							else if(curr_ts_data.X_position[i] > 791 && curr_ts_data.X_position[i] < 1001) //center 405
							{
								if(!prev_ts_data.touch_status[i])
								{
									input_report_key(ts->input_dev, KEY_SEARCH, 1); //seven blocked for key drag action
									pr_debug("[TOUCH-13] Key Event KEY = %d, PRESS = %d\n", KEY_SEARCH, 1);
									pressed_button_type = KEY_SEARCH;
								}
								else
								{
									if(pressed_button_type != KEY_SEARCH && pressed_button_type != KEY_REJECT)
									{
										input_report_key(ts->input_dev, KEY_REJECT, 1);
										pr_debug("[TOUCH-14] Key Event KEY = %d, PRESS = %d\n", KEY_REJECT, 1);
										input_report_key(ts->input_dev, KEY_REJECT, 0);
										pr_debug("[TOUCH-15] Key Event KEY = %d, PRESS = %d\n", KEY_REJECT, 0);
										input_report_key(ts->input_dev, pressed_button_type, 0);
										pr_debug("[TOUCH-16] Key Event KEY = %d, PRESS = %d\n", pressed_button_type, 0);
										pressed_button_type = KEY_REJECT;
									}
								}
							}
							else
							{
								if(!prev_ts_data.touch_status[i])
								{
									pressed_button_type = KEY_REJECT;
								}
								else
								{
									if(pressed_button_type != KEY_REJECT)
									{
										input_report_key(ts->input_dev, KEY_REJECT, 1);
										pr_debug("[TOUCH-17] Key Event KEY = %d, PRESS = %d\n", KEY_REJECT, 1);
										input_report_key(ts->input_dev, KEY_REJECT, 0);
										pr_debug("[TOUCH-18] Key Event KEY = %d, PRESS = %d\n", KEY_REJECT, 0);
										input_report_key(ts->input_dev, pressed_button_type, 0);
										pr_debug("[TOUCH-19] Key Event KEY = %d, PRESS = %d\n", pressed_button_type, 0);
										pressed_button_type = KEY_REJECT;
									}
								}
							}

						}
						else
						{
							if(!prev_ts_data.touch_status[i])
							{
								pressed_button_type = KEY_REJECT;
							}
							else
							{
								if(pressed_button_type != KEY_REJECT)
								{
									input_report_key(ts->input_dev, KEY_REJECT, 1);
									pr_debug("[TOUCH-20] Key Event KEY = %d, PRESS = %d\n", KEY_REJECT, 1);
									input_report_key(ts->input_dev, KEY_REJECT, 0);
									pr_debug("[TOUCH-21] Key Event KEY = %d, PRESS = %d\n", KEY_REJECT, 0);
									input_report_key(ts->input_dev, pressed_button_type, 0);
									pr_debug("[TOUCH-22] Key Event KEY = %d, PRESS = %d\n", pressed_button_type, 0);
									pressed_button_type = KEY_REJECT;
								}
							}

// 20101021 joseph.jung@lge.com lcd-button area touch scenario change [START]
								input_report_abs(ts->input_dev, ABS_MT_POSITION_X, curr_ts_data.X_position[i]);
								input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, curr_ts_data.Y_position[i]);
								input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, curr_ts_data.pressure[i]);
								input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, curr_ts_data.width[i]);
								
								input_mt_sync(ts->input_dev);
								pr_debug("[TOUCH-23] (X, Y) = (%d, %d), z = %d, w = %d\n", curr_ts_data.X_position[i], curr_ts_data.Y_position[i], curr_ts_data.pressure[i], curr_ts_data.width[i]);

								curr_event_type = TOUCH_EVENT_ABS;
// 20101021 joseph.jung@lge.com lcd-button area touch scenario change [END]
						}
					}
					else
					{
						curr_event_type = TOUCH_EVENT_NULL;
						pressed_button_type = KEY_REJECT;
					}
					//break; /* 20101231 seven.kim@lge.com do not need any more */
				}
				else // multi-finger
				{
					curr_event_type = TOUCH_EVENT_ABS;

					if(pressed_button_type != KEY_REJECT)
					{
						input_report_key(ts->input_dev, KEY_REJECT, 1);
						pr_debug("[TOUCH-24] Key Event KEY = %d, PRESS = %d\n", KEY_REJECT, 1);
						input_report_key(ts->input_dev, KEY_REJECT, 0);
						pr_debug("[TOUCH-25] Key Event KEY = %d, PRESS = %d\n", KEY_REJECT, 0);
						input_report_key(ts->input_dev, pressed_button_type, 0);
						pr_debug("[TOUCH-26] Key Event KEY = %d, PRESS = %d\n", pressed_button_type, 0);
						pressed_button_type = KEY_REJECT;
					}

					if(curr_ts_data.Y_position[i] < SYNAPTICS_PANEL_LCD_MAX_Y)
					{
						input_report_abs(ts->input_dev, ABS_MT_POSITION_X, curr_ts_data.X_position[i]);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, curr_ts_data.Y_position[i]);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, curr_ts_data.pressure[i]);
						input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, curr_ts_data.width[i]);

						input_mt_sync(ts->input_dev);

						/*20110227 seven.kim@lge.com to remove ghost finger for HW Drop Test [START] */
						if(curr_ts_data.X_position[1] || curr_ts_data.Y_position[1])
							ghost_count=0; 
						/*20110227 seven.kim@lge.com to remove ghost finger for HW Drop Test [START] */
						
						pr_debug("[TOUCH-27] (X, Y) = (%d, %d), z = %d, w = %d\n", curr_ts_data.X_position[i], curr_ts_data.Y_position[i], curr_ts_data.pressure[i], curr_ts_data.width[i]);
					}
				}
			}
			else
			{
				if(pressed_button_type != KEY_REJECT && i == 0)
				{					
					input_report_key(ts->input_dev, pressed_button_type, 0);
                                        // 20101203 kyungyoon.kim@lge.com, Touch KEY LED [START_LGE]
					if (system_rev >= 3)
					{
						switch(pressed_button_type)
						{
							case KEY_MENU:
								touchkey_pressed(MENU);
								break;
							case KEY_HOME:
								touchkey_pressed(HOME);
								break;
							case KEY_BACK:
								touchkey_pressed(BACK);
								break;
							case KEY_SEARCH:
								touchkey_pressed(SEARCH);
								break;
						}
					}
					else
					{
							pr_debug("Touch Key LED is working at over Rev.C\n");
					}
                                        // 20101203 kyungyoon.kim@lge.com, Touch KEY LED [END_LGE]
					pr_debug("[TOUCH-28] Key Event KEY = %d, PRESS = %d\n", pressed_button_type, 0);
					pressed_button_type = KEY_REJECT;
				}
			}
		}
		
		if(finger_count == 0)
		{
			prev_event_type = TOUCH_EVENT_NULL;
		}
		else
		{
			prev_event_type = curr_event_type;
		}

#ifdef FEATURE_LGE_TOUCH_GHOST_FINGER_IMPROVE
// 20101110 jh.koo@lge.com, check and change mode for remove ghost fingers [START_LGE]
		if(melt_mode == 0) {			

				if(pressed) {

						if( TS_SNTS_GET_FINGER_STATE_1(ts_reg_data.finger_state_reg[0]) == 1 ||
								TS_SNTS_GET_FINGER_STATE_2(ts_reg_data.finger_state_reg[0]) == 1 ||
								TS_SNTS_GET_FINGER_STATE_3(ts_reg_data.finger_state_reg[0]) == 1 ||
								TS_SNTS_GET_FINGER_STATE_0(ts_reg_data.finger_state_reg[1]) == 1 ||
								TS_SNTS_GET_FINGER_STATE_1(ts_reg_data.finger_state_reg[1]) == 1 ||
								TS_SNTS_GET_FINGER_STATE_2(ts_reg_data.finger_state_reg[1]) == 1 ||
								TS_SNTS_GET_FINGER_STATE_3(ts_reg_data.finger_state_reg[1]) == 1 ||
								TS_SNTS_GET_FINGER_STATE_0(ts_reg_data.finger_state_reg[2]) == 1 ||
								TS_SNTS_GET_FINGER_STATE_1(ts_reg_data.finger_state_reg[2]) == 1 ) {

										ghost_finger_2 = 1; 					
						}
				}

				if((TS_SNTS_GET_FINGER_STATE_0(ts_reg_data.finger_state_reg[0]) == 0) && ghost_finger_1 == 1 && ghost_finger_2 == 0 && pressed == 1) {
				
						if(jiffies - pressed_time < 2 * HZ) {
							ghost_count++;
							if (ghost_count > 0/*3*/) //20110503 kim.youngmin@lge.com.. 3->0
							{
								synaptics_ts_i2c_write_byte_data(ts->client, MELT_CONTROL, NO_MELT);
								pr_warning("Touch NO_MELT\n");
								ghost_count = 0;
								melt_mode++;
							}
						}

						ghost_finger_1 = 0;
						pressed = 0;
				}		
		
				if( TS_SNTS_GET_FINGER_STATE_0(ts_reg_data.finger_state_reg[0]) == 0 &&
						TS_SNTS_GET_FINGER_STATE_1(ts_reg_data.finger_state_reg[0]) == 0 &&
						TS_SNTS_GET_FINGER_STATE_2(ts_reg_data.finger_state_reg[0]) == 0 &&
						TS_SNTS_GET_FINGER_STATE_3(ts_reg_data.finger_state_reg[0]) == 0 &&
						TS_SNTS_GET_FINGER_STATE_0(ts_reg_data.finger_state_reg[1]) == 0 &&
						TS_SNTS_GET_FINGER_STATE_1(ts_reg_data.finger_state_reg[1]) == 0 &&
						TS_SNTS_GET_FINGER_STATE_2(ts_reg_data.finger_state_reg[1]) == 0 &&
						TS_SNTS_GET_FINGER_STATE_3(ts_reg_data.finger_state_reg[1]) == 0 &&
						TS_SNTS_GET_FINGER_STATE_0(ts_reg_data.finger_state_reg[2]) == 0 &&
						TS_SNTS_GET_FINGER_STATE_1(ts_reg_data.finger_state_reg[2]) == 0 ) {

								ghost_finger_1 = 0;
								ghost_finger_2 = 0;
								pressed = 0;
				}				
		}						
// 20101110 jh.koo@lge.com, check and change mode for remove ghost fingers [END_LGE]
#endif

	}

	input_mt_sync(ts->input_dev);
	input_sync(ts->input_dev);
/*[START] kim.youngmin@lge.com To avoid touch sensor lock-up & data missing in touch release*/	
}while(Synaptics_Check_Touch_Interrupt_Status());
/*[END] kim.youngmin@lge.com To avoid touch sensor lock-up & data missing in touch release*/
SYNAPTICS_TS_IDLE:
	if (ts->use_irq) {		
		enable_irq(ts->client->irq);
	}
}

static enum hrtimer_restart synaptics_ts_timer_func(struct hrtimer *timer)
{
	struct synaptics_ts_data *ts = container_of(timer, struct synaptics_ts_data, timer);

	queue_work(synaptics_wq, &ts->work);
	hrtimer_start(&ts->timer, ktime_set(0, 12500000), HRTIMER_MODE_REL); /* 12.5 msec */

	return HRTIMER_NORESTART;
}

static irqreturn_t synaptics_ts_irq_handler(int irq, void *dev_id)
{
	struct synaptics_ts_data *ts = dev_id;
#ifdef CONFIG_LGE_DVFS
	int ds_cpu = smp_processor_id();
	unsigned long touch_interval;
#endif// CONFIG_LGE_DVFS

	//pr_info("LGE: synaptics_ts_irq_handler\n");
	disable_irq_nosync(ts->client->irq);

	queue_work(synaptics_wq, &ts->work);

#ifdef CONFIG_LGE_DVFS
	if(ds_control.flag_run_dvs == 1)
	{
		if(ds_cpu == 0){
			if(per_cpu(ds_sys_status, 0).flag_consecutive_touches == 0){
				if(per_cpu(ds_counter, ds_cpu).elapsed_sec < 
				   per_cpu(ds_sys_status, 0).new_touch_sec + 2)
				{
					touch_interval = (per_cpu(ds_counter, ds_cpu).elapsed_sec -
							  per_cpu(ds_sys_status, 0).new_touch_sec) * 1000000 +
						(per_cpu(ds_counter, ds_cpu).elapsed_usec -
						 per_cpu(ds_sys_status, 0).new_touch_usec);
					if(touch_interval <= DS_CONT_TOUCH_THRESHOLD_USEC)
					{
						if(per_cpu(ds_sys_status, 0).flag_consecutive_touches == 0){
							per_cpu(ds_sys_status, 0).first_consecutive_touch_sec =
								per_cpu(ds_counter, ds_cpu).elapsed_sec;
						}
						per_cpu(ds_sys_status, 0).flag_consecutive_touches = 1;
					}
				}
			}
			per_cpu(ds_sys_status, 0).new_touch_sec = 
				per_cpu(ds_counter, ds_cpu).elapsed_sec;
			per_cpu(ds_sys_status, 0).new_touch_usec = 
				per_cpu(ds_counter, ds_cpu).elapsed_usec;
			
			if(per_cpu(ds_sys_status, 0).flag_consecutive_touches == 0){
				per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 
					DS_TOUCH_CPU_OP_UP_CNT_MAX;
				if((per_cpu(ds_counter, ds_cpu).elapsed_usec + 
				    DS_TOUCH_CPU_OP_UP_INTERVAL) < 1000000)
				{
					per_cpu(ds_sys_status, 0).touch_timeout_sec = 
						per_cpu(ds_counter, ds_cpu).elapsed_sec;
					per_cpu(ds_sys_status, 0).touch_timeout_usec = 
						per_cpu(ds_counter, ds_cpu).elapsed_usec + 
						DS_TOUCH_CPU_OP_UP_INTERVAL;
				}
				else{
					per_cpu(ds_sys_status, 0).touch_timeout_sec = 
						per_cpu(ds_counter, ds_cpu).elapsed_sec + 1;
					per_cpu(ds_sys_status, 0).touch_timeout_usec = 
						(per_cpu(ds_counter, ds_cpu).elapsed_usec + 
						 DS_TOUCH_CPU_OP_UP_INTERVAL) - 1000000;
				}
			}
			
			if(per_cpu(ds_sys_status, 0).flag_consecutive_touches == 1){
				if(per_cpu(ds_sys_status, 0).flag_long_consecutive_touches == 0){
					if(per_cpu(ds_counter, ds_cpu).elapsed_sec > 
					   (per_cpu(ds_sys_status, 0).first_consecutive_touch_sec + 
					    DS_CONT_TOUCH_CARE_WAIT_SEC))
					{
						per_cpu(ds_sys_status, 0).flag_long_consecutive_touches = 1;
					}
				}
			}
		}
	}
#endif// CONFIG_LGE_DVFS

	return IRQ_HANDLED;
}


static unsigned char synaptics_ts_check_fwver(struct i2c_client *client)
{
	unsigned char RMI_Query_BaseAddr;
	unsigned char FWVersion_Addr;

	unsigned char SynapticsFirmVersion;

	RMI_Query_BaseAddr = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_RMI_QUERY_BASE_REG);
	FWVersion_Addr = RMI_Query_BaseAddr+3;
	
	SynapticsFirmVersion = synaptics_ts_i2c_read_byte_data(client, FWVersion_Addr);
	pr_debug("[TOUCH] Touch controller Firmware Version = %x\n", SynapticsFirmVersion);

	return SynapticsFirmVersion;
}


static ssize_t hub_ts_FW_show(struct device *dev,  struct device_attribute *attr,  char *buf)
{
	int r;

	r = snprintf(buf, PAGE_SIZE,
		"%d\n", touch_fw_version);

	return r;

}

static DEVICE_ATTR(fw, 0666, hub_ts_FW_show, NULL);




#ifdef SYNAPTICS_SUPPORT_FW_UPGRADE
static unsigned long ExtractLongFromHeader(const unsigned char *SynaImage)  // Endian agnostic 
{
  return((unsigned long)SynaImage[0] +
         (unsigned long)SynaImage[1]*0x100 +
         (unsigned long)SynaImage[2]*0x10000 +
         (unsigned long)SynaImage[3]*0x1000000);
}

static void CalculateChecksum(uint16_t *data, uint16_t len, uint32_t *dataBlock)
{
  unsigned long temp = *data++;
  unsigned long sum1;
  unsigned long sum2;

  *dataBlock = 0xffffffff;

  sum1 = *dataBlock & 0xFFFF;
  sum2 = *dataBlock >> 16;

  while (len--)
  {
    sum1 += temp;    
    sum2 += sum1;    
    sum1 = (sum1 & 0xffff) + (sum1 >> 16);    
    sum2 = (sum2 & 0xffff) + (sum2 >> 16);
  }

  *dataBlock = sum2 << 16 | sum1;
}

static void SpecialCopyEndianAgnostic(uint8_t *dest, uint16_t src) 
{
  dest[0] = src%0x100;  //Endian agnostic method
  dest[1] = src/0x100;  
}


static bool synaptics_ts_fw_upgrade(struct i2c_client *client)
{
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);
	int i;
	int j;

	uint8_t FlashQueryBaseAddr, FlashDataBaseAddr;
	uint8_t RMICommandBaseAddr;
	
	uint8_t BootloaderIDAddr;
	uint8_t BlockSizeAddr;
	uint8_t FirmwareBlockCountAddr;
	uint8_t ConfigBlockCountAddr;

	uint8_t BlockNumAddr;
	uint8_t BlockDataStartAddr;

	uint8_t current_fw_ver;
	
	uint8_t bootloader_id[2];

	uint8_t temp_array[2], temp_data, flashValue, m_firmwareImgVersion;
	uint8_t checkSumCode;

	uint16_t ts_block_size, ts_config_block_count, ts_fw_block_count;
	uint16_t m_bootloadImgID;
	
	uint32_t ts_config_img_size;
	uint32_t ts_fw_img_size;
	uint32_t pinValue, m_fileSize, m_firmwareImgSize, m_configImgSize, m_FirmwareImgFile_checkSum;

	////////////////////////////

	pr_debug("[Touch Driver] Synaptics_UpgradeFirmware [START]\n");
/*
	if(!(synaptics_ts_check_fwver(client) < SynapticsFirmware[0x1F]))
	{
		// Firmware Upgrade does not necessary!!!!
		pr_debug("[Touch Driver] Synaptics_UpgradeFirmware does not necessary!!!!\n");
		return true;
	}
*/
	if (ts->product_value==1)
	{
		memcpy(SynapticsFirmware, SynapticsFirmware_misung, sizeof(SynapticsFirmware_misung));
		current_fw_ver = synaptics_ts_check_fwver(client);
		if((current_fw_ver >= 0x64 && SynapticsFirmware[0x1F] >= 0x64) || (current_fw_ver < 0x64 && SynapticsFirmware[0x1F] < 0x64))
		{
			if(!(current_fw_ver < SynapticsFirmware[0x1F]))
			{
				// Firmware Upgrade does not necessary!!!!
				pr_debug("[Touch Driver] Synaptics_UpgradeFirmware does not necessary!!!!\n");
				return true;
			}
		}
	}
	else if (ts->product_value==2)
	{
			memcpy(SynapticsFirmware, SynapticsFirmware_lgit, sizeof(SynapticsFirmware_lgit));
			current_fw_ver = synaptics_ts_check_fwver(client);
			if((current_fw_ver >= 0x01 && SynapticsFirmware[0x1F] >= 0x01) || (current_fw_ver < 0x01 && SynapticsFirmware[0x1F] < 0x01))
			{
				if(!(current_fw_ver < SynapticsFirmware[0x1F]))
				{
					// Firmware Upgrade does not necessary!!!!
					pr_debug("[Touch Driver] Synaptics_UpgradeFirmware does not necessary!!!!\n");
					return true;
				}
			}
	}
	else
		return true;

	// Address Configuration
	FlashQueryBaseAddr = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_FLASH_QUERY_BASE_REG);

	BootloaderIDAddr = FlashQueryBaseAddr;
	BlockSizeAddr = FlashQueryBaseAddr + 3;
	FirmwareBlockCountAddr = FlashQueryBaseAddr + 5;
	ConfigBlockCountAddr = FlashQueryBaseAddr + 7;
	

	FlashDataBaseAddr = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_FLASH_DATA_BASE_REG);

	BlockNumAddr = FlashDataBaseAddr;
	BlockDataStartAddr = FlashDataBaseAddr + 2;

	// Get New Firmware Information from Header
	m_fileSize = sizeof(SynapticsFirmware) -1;

	checkSumCode         = ExtractLongFromHeader(&(SynapticsFirmware[0]));
	m_bootloadImgID      = (unsigned int)SynapticsFirmware[4] + (unsigned int)SynapticsFirmware[5]*0x100;
	m_firmwareImgVersion = SynapticsFirmware[7]; 
	m_firmwareImgSize    = ExtractLongFromHeader(&(SynapticsFirmware[8]));
	m_configImgSize      = ExtractLongFromHeader(&(SynapticsFirmware[12]));    

	CalculateChecksum((uint16_t*)&(SynapticsFirmware[4]), (uint16_t)(m_fileSize-4)>>1, &m_FirmwareImgFile_checkSum);

	// Get Current Firmware Information
	synaptics_ts_i2c_read_block_data(client, BlockSizeAddr, sizeof(temp_array), (u8 *)&temp_array[0]);
	ts_block_size = temp_array[0] + (temp_array[1] << 8);
	
	synaptics_ts_i2c_read_block_data(client, FirmwareBlockCountAddr, sizeof(temp_array), (u8 *)&temp_array[0]);
	ts_fw_block_count = temp_array[0] + (temp_array[1] << 8);
	ts_fw_img_size = ts_block_size * ts_fw_block_count;
	
	synaptics_ts_i2c_read_block_data(client, ConfigBlockCountAddr, sizeof(temp_array), (u8 *)&temp_array[0]);
	ts_config_block_count = temp_array[0] + (temp_array[1] << 8);
	ts_config_img_size = ts_block_size * ts_config_block_count;

	synaptics_ts_i2c_read_block_data(client, BootloaderIDAddr, sizeof(bootloader_id), (u8 *)&bootloader_id[0]);
	pr_debug("[TOUCH] Synaptics_UpgradeFirmware :: BootloaderID %02x %02x\n", bootloader_id[0], bootloader_id[1]);

	// Compare
	if (m_fileSize != (0x100+m_firmwareImgSize+m_configImgSize))
	{
		pr_debug("[TOUCH] Synaptics_UpgradeFirmware :: Error : Invalid FileSize\n");
		return true;
	}

	if (m_firmwareImgSize != ts_fw_img_size)
	{
		pr_debug("[TOUCH] Synaptics_UpgradeFirmware :: Error : Invalid Firmware Image Size\n");
		return true;
	}

	if (m_configImgSize != ts_config_img_size)
	{
		pr_debug("[TOUCH] Synaptics_UpgradeFirmware :: Error : Invalid Config Image Size\n");
		return true;
	}

	// Flash Write Ready - Flash Command Enable & Erase
	//synaptics_ts_i2c_write_block_data(client, BlockDataStartAddr, sizeof(bootloader_id), &bootloader_id[0]);
	// How can i use 'synaptics_ts_i2c_write_block_data'
	for(i = 0; i < sizeof(bootloader_id); i++)
	{
		if(synaptics_ts_i2c_write_byte_data(client, BlockDataStartAddr+i, bootloader_id[i]))
			pr_debug("[TOUCH] Synaptics_UpgradeFirmware :: Address %02x, Value %02x\n", BlockDataStartAddr+i, bootloader_id[i]);
	}

	do
	{
		flashValue = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_FLASH_CONTROL_REG);
		temp_data = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_INT_STATUS_REG);
	} while((flashValue & 0x0f) != 0x00);

	synaptics_ts_i2c_write_byte_data(client, SYNAPTICS_FLASH_CONTROL_REG, SYNAPTICS_FLASH_CMD_ENABLE);

	do
	{
		pinValue = gpio_get_value(TOUCH_INT_N_GPIO);
		mdelay(1);
	} while(pinValue);
	do
	{
		flashValue = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_FLASH_CONTROL_REG);
		temp_data = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_INT_STATUS_REG);
	} while(flashValue != 0x80);
	flashValue = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_FLASH_CONTROL_REG);

	pr_debug("[TOUCH] Synaptics_UpgradeFirmware :: Flash Program Enable Setup Complete\n");

	//synaptics_ts_i2c_write_block_data(client, BlockDataStartAddr, sizeof(bootloader_id), &bootloader_id[0]);
	// How can i use 'synaptics_ts_i2c_write_block_data'
	for(i = 0; i < sizeof(bootloader_id); i++)
	{
		if(synaptics_ts_i2c_write_byte_data(client, BlockDataStartAddr+i, bootloader_id[i]))
			pr_debug("[TOUCH] Synaptics_UpgradeFirmware :: Address %02x, Value %02x\n", BlockDataStartAddr+i, bootloader_id[i]);
	}

	if(m_firmwareImgVersion == 0 && ((unsigned int)bootloader_id[0] + (unsigned int)bootloader_id[1]*0x100) != m_bootloadImgID)
	{
		pr_debug("[TOUCH] Synaptics_UpgradeFirmware :: Error : Invalid Bootload Image\n");
		return true;
	}

	synaptics_ts_i2c_write_byte_data(client, SYNAPTICS_FLASH_CONTROL_REG, SYNAPTICS_FLASH_CMD_ERASEALL);

	pr_debug("[TOUCH] Synaptics_UpgradeFirmware :: SYNAPTICS_FLASH_CMD_ERASEALL\n");

	do
	{
		pinValue = gpio_get_value(TOUCH_INT_N_GPIO);
		mdelay(1);
	} while(pinValue);
	do
	{
		flashValue = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_FLASH_CONTROL_REG);
		temp_data = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_INT_STATUS_REG);
	} while(flashValue != 0x80);
	
	pr_debug("[TOUCH] Synaptics_UpgradeFirmware :: Flash Erase Complete\n");

	// Flash Firmware Data Write
	for(i = 0; i < ts_fw_block_count; ++i)
	{
		temp_array[0] = i & 0xff;
		temp_array[1] = (i & 0xff00) >> 8;

		// Write Block Number
		//synaptics_ts_i2c_write_block_data(client, BlockNumAddr, sizeof(temp_array), &temp_array[0]);
		// How can i use 'synaptics_ts_i2c_write_block_data'
		for(j = 0; j < sizeof(temp_array); j++)
		{
			synaptics_ts_i2c_write_byte_data(client, BlockNumAddr+j, temp_array[j]);
		}

		// Write Data Block&SynapticsFirmware[0]
		//synaptics_ts_i2c_write_block_data(client, BlockDataStartAddr, ts_block_size, &SynapticsFirmware[0x100+i*ts_block_size]);
		// How can i use 'synaptics_ts_i2c_write_block_data'
		for(j = 0; j < ts_block_size; j++)
		{
			synaptics_ts_i2c_write_byte_data(client, BlockDataStartAddr+j, SynapticsFirmware[0x100+i*ts_block_size+j]);
		}

		// Issue Write Firmware Block command
		synaptics_ts_i2c_write_byte_data(client, SYNAPTICS_FLASH_CONTROL_REG, SYNAPTICS_FLASH_CMD_FW_WRITE);
		do
		{
			pinValue = gpio_get_value(TOUCH_INT_N_GPIO);
			mdelay(1);
		} while(pinValue);
		do
		{
			flashValue = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_FLASH_CONTROL_REG);
			temp_data = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_INT_STATUS_REG);
		} while(flashValue != 0x80);
	} //for
	
	pr_debug("[TOUCH] Synaptics_UpgradeFirmware :: Flash Firmware Write Complete\n");

	// Flash Firmware Config Write
	for(i = 0; i < ts_config_block_count; i++)
	{
		SpecialCopyEndianAgnostic(&temp_array[0], i);

		// Write Configuration Block Number
		synaptics_ts_i2c_write_block_data(client, BlockNumAddr, sizeof(temp_array), &temp_array[0]);
		// How can i use 'synaptics_ts_i2c_write_block_data'
		for(j = 0; j < sizeof(temp_array); j++)
		{
			synaptics_ts_i2c_write_byte_data(client, BlockNumAddr+j, temp_array[j]);
		}

		// Write Data Block
		//synaptics_ts_i2c_write_block_data(client, BlockDataStartAddr, ts_block_size, &SynapticsFirmware[0x100+m_firmwareImgSize+i*ts_block_size]);
		// How can i use 'synaptics_ts_i2c_write_block_data'
		for(j = 0; j < ts_block_size; j++)
		{
			synaptics_ts_i2c_write_byte_data(client, BlockDataStartAddr+j, SynapticsFirmware[0x100+m_firmwareImgSize+i*ts_block_size+j]);
		}

		// Issue Write Configuration Block command to flash command register
		synaptics_ts_i2c_write_byte_data(client, SYNAPTICS_FLASH_CONTROL_REG, SYNAPTICS_FLASH_CMD_CONFIG_WRITE);
		do
		{
			pinValue = gpio_get_value(TOUCH_INT_N_GPIO);
			mdelay(1);
		} while(pinValue);
		do
		{
			flashValue = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_FLASH_CONTROL_REG);
			temp_data = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_INT_STATUS_REG);
		} while(flashValue != 0x80);
	}
	
	pr_debug("[TOUCH] Synaptics_UpgradeFirmware :: Flash Config Write Complete\n");


	RMICommandBaseAddr = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_RMI_CMD_BASE_REG);
	synaptics_ts_i2c_write_byte_data(client, RMICommandBaseAddr, 0x01);
	mdelay(100);

	do
	{
		pinValue = gpio_get_value(TOUCH_INT_N_GPIO);
		mdelay(1);
	} while(pinValue);
	do
	{
		flashValue = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_FLASH_CONTROL_REG);
		temp_data = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_INT_STATUS_REG);
	} while((flashValue & 0x0f) != 0x00);

	// Clear the attention assertion by reading the interrupt status register
	temp_data = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_INT_STATUS_REG);

	// Read F01 Status flash prog, ensure the 6th bit is '0'
	do
	{
		temp_data = synaptics_ts_i2c_read_byte_data(client, SYNAPTICS_DATA_BASE_REG);
	} while((temp_data & 0x40) != 0);

	return true;
}
#endif

#ifdef FEATURE_LGE_TOUCH_ESD_DETECT //20101221 seven.kim@lge.com to detect Register change by ESD
static bool  Synatics_ts_touch_Recovery()
{
	//touch power down reset for ESD recovery
	aat2870_touch_ldo_enable(aat2870_i2c_client, 0);
	mdelay(50);

	//touch power on reset for ESD recovery
	aat2870_touch_ldo_enable(aat2870_i2c_client, 1);
	mdelay(50);
	
	return true;
}
#endif /*FEATURE_LGE_TOUCH_ESD_DETECT*/


/*************************************************************************************************
 * 1. Set interrupt configuration
 * 2. Disable interrupt
 * 3. Power up
 * 4. Read RMI Version
 * 5. Read Firmware version & Upgrade firmware automatically
 * 6. Read Data To Initialization Touch IC
 * 7. Set some register
 * 8. Enable interrupt
*************************************************************************************************/
static int synaptics_ts_probe(
	struct i2c_client *client, const struct i2c_device_id *id)
{
	struct synaptics_ts_data *ts;
	int ret = 0;
	uint16_t max_x;
	uint16_t max_y;
	uint8_t max_pressure;
	uint8_t max_width;

	char product_id[6];
	uint8_t product_id_addr;

	if (lcd_off_boot) return -ENODEV;
	pr_warning("%s() -- start\n\n\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("synaptics_ts_probe: need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	ts->client = client;
	hub_ts_client = client;
	i2c_set_clientdata(client, ts);

	INIT_WORK(&ts->work, synaptics_ts_work_func);
	INIT_DELAYED_WORK(&ts->init_delayed_work, synaptics_ts_init_delayed_work);

#ifdef FEATURE_LGE_TOUCH_ESD_DETECT //20101221 seven.kim@lge.com to detect Register change by ESD
	wake_lock_init(&ts_wake_lock, WAKE_LOCK_SUSPEND, "ts_upgrade");
#endif /*FEATURE_LGE_TOUCH_ESD_DETECT*/	

// 20101215 seven.kim@lge.com grip suppression [START]
#ifdef FEATURE_LGE_TOUCH_GRIP_SUPPRESSION
	ret = device_create_file(&client->dev, &dev_attr_gripsuppression);
	if (ret) {
		pr_err("synaptics_ts_probe: grip suppression device_create_file failed\n");
		goto err_check_functionality_failed;
	}
#endif /* FEATURE_LGE_TOUCH_GRIP_SUPPRESSION */
// 20101215 seven.kim@lge.com grip suppression [END]

  	memset(&ts_reg_data, 0x0, sizeof(ts_sensor_data));
	memset(&prev_ts_data, 0x0, sizeof(ts_finger_data));
  	memset(&curr_ts_data, 0x0, sizeof(ts_finger_data));

	/*************************************************************************************************
	 * 3. Power up
	 *************************************************************************************************/
	/* It's controlled by LCD BL CHARGER PUMP */


	/*************************************************************************************************
	 * 4. Read RMI Version
	 * To distinguish T1021 and T1007. Select RMI Version
	 * TODO: Power를 이전에 하는 것으로 변경하면 위치 변경해야 한다.
	 *************************************************************************************************/

	// device check
	product_id_addr = (synaptics_ts_i2c_read_byte_data(hub_ts_client, SYNAPTICS_RMI_QUERY_BASE_REG)) + 11;
	synaptics_ts_i2c_read_block_data(hub_ts_client, product_id_addr, sizeof(product_id), (u8 *)&product_id[0]);

	if(strncmp(product_id, SYNAPTICS_TM1576_PRODUCT_ID, 6) == 0)
	{
		pr_err("synaptics_ts_probe: product ID : TM1576\n");
		SYNAPTICS_PANEL_MAX_X = SYNAPTICS_TM1576_RESOLUTION_X;
		SYNAPTICS_PANEL_MAX_Y = SYNAPTICS_TM1576_RESOLUTION_Y;
		SYNAPTICS_PANEL_LCD_MAX_Y = SYNAPTICS_TM1576_LCD_ACTIVE_AREA;
		SYNAPTICS_PANEL_BUTTON_MIN_Y = SYNAPTICS_TM1576_BUTTON_ACTIVE_AREA;
		ts->product_value=3; //other
	}
	else if(strncmp(product_id, SYNAPTICS_TM1702_PRODUCT_ID, 6) == 0)
	{
		pr_err("synaptics_ts_probe: product ID : TM1702\n");
		SYNAPTICS_PANEL_MAX_X = SYNAPTICS_TM1702_RESOLUTION_X;
		SYNAPTICS_PANEL_MAX_Y = SYNAPTICS_TM1702_RESOLUTION_Y;
		SYNAPTICS_PANEL_LCD_MAX_Y = SYNAPTICS_TM1702_LCD_ACTIVE_AREA;
		SYNAPTICS_PANEL_BUTTON_MIN_Y = SYNAPTICS_TM1702_BUTTON_ACTIVE_AREA;
		ts->product_value=1; //misung panel

		synaptics_ts_fw_upgrade(hub_ts_client);
	}
	else if(strncmp(product_id, SYNAPTICS_TM1738_PRODUCT_ID, 6) == 0)
	{
		pr_err("synaptics_ts_probe: product ID : TM1738\n");
		SYNAPTICS_PANEL_MAX_X = SYNAPTICS_TM1738_RESOLUTION_X;
		SYNAPTICS_PANEL_MAX_Y = SYNAPTICS_TM1738_RESOLUTION_Y;
		SYNAPTICS_PANEL_LCD_MAX_Y = SYNAPTICS_TM1738_LCD_ACTIVE_AREA;
		SYNAPTICS_PANEL_BUTTON_MIN_Y = SYNAPTICS_TM1738_BUTTON_ACTIVE_AREA;
		ts->product_value=2; //lgit panel

		synaptics_ts_fw_upgrade(hub_ts_client);
	}
	else
	{
		pr_err("synaptics_ts_probe: product ID : error\n");
		SYNAPTICS_PANEL_MAX_X = SYNAPTICS_TM1702_RESOLUTION_X;
		SYNAPTICS_PANEL_MAX_Y = SYNAPTICS_TM1702_RESOLUTION_Y;
		SYNAPTICS_PANEL_LCD_MAX_Y = SYNAPTICS_TM1702_LCD_ACTIVE_AREA;
		SYNAPTICS_PANEL_BUTTON_MIN_Y = SYNAPTICS_TM1702_BUTTON_ACTIVE_AREA;
		ts->product_value=0; //nothing
	}

	touch_fw_version=synaptics_ts_check_fwver(hub_ts_client);
	

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		pr_err("synaptics_ts_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	ts->input_dev->name = "hub_synaptics_touch";

	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(EV_ABS, ts->input_dev->evbit);

	// button
	set_bit(KEY_MENU, ts->input_dev->keybit);
	set_bit(KEY_HOME, ts->input_dev->keybit);
	set_bit(KEY_BACK, ts->input_dev->keybit);
	set_bit(KEY_SEARCH, ts->input_dev->keybit);
	set_bit(KEY_REJECT, ts->input_dev->keybit);

	max_x = SYNAPTICS_PANEL_MAX_X;
	max_y = SYNAPTICS_PANEL_LCD_MAX_Y;
	max_pressure = 0xFF;
	max_width = 0x0F;
 
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, max_x, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, max_y, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, max_pressure, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, max_width, 0, 0);

	pr_info("synaptics_ts_probe: max_x %d, max_y %d\n", max_x, max_y);


	/* ts->input_dev->name = ts->keypad_info->name; */
	ret = input_register_device(ts->input_dev);
	if (ret) {
		pr_err("synaptics_ts_probe: Unable to register %s input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	}

	pr_debug("########## irq [%d], irqflags[0x%x]\n", client->irq, IRQF_TRIGGER_FALLING);
	
	//[START] 20101227 kim.youngmin@lge.com to prevent interrupt in booting time 
	synaptics_ts_i2c_write_byte_data(hub_ts_client, SYNAPTICS_RIM_CONTROL_INTERRUPT_ENABLE, 0x00); //interrupt disable
	//[START] 20101227 kim.youngmin@lge.com to prevent interrupt in booting time 
	
	if (client->irq) {
		ret = request_irq(client->irq, synaptics_ts_irq_handler, IRQF_TRIGGER_FALLING, client->name, ts);

		if (ret == 0) {
			ts->use_irq = 1;
			pr_warning("request_irq\n");
			}
		else
			dev_err(&client->dev, "request_irq failed\n");
	}
	if (!ts->use_irq) {
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = synaptics_ts_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}

// 20100827 jh.koo@lge.com, for check ts i2c status [START_LGE]
	ret = device_create_file(&client->dev, &dev_attr_fw);
	if (ret) {
		pr_debug( "Hub-touch screen : touch screen_probe: Fail\n");
		device_remove_file(&client->dev, &dev_attr_fw);
		return ret;
	}
// 20100827 jh.koo@lge.com, for check ts i2c status [END_LGE]

// 20100826 jh.koo@lge.com, for stable initialization [START_LGE]
	schedule_delayed_work(&ts->init_delayed_work, msecs_to_jiffies(200/*500*/));	
// 20100826 jh.koo@lge.com, for stable initialization [END_LGE]
	
	/* 20110216 seven.kim@lge.com to follow android sleep/resume flow [START] */
//	atomic_set( &g_synaptics_ts_resume_flag, 0 );
//	atomic_set( &g_synaptics_ts_suspend_flag, 0 );
	/* 20110216 seven.kim@lge.com to follow android sleep/resume flow [END] */	
	
#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = synaptics_ts_early_suspend;
	ts->early_suspend.resume = synaptics_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	pr_warning("synaptics_ts_probe: Start touchscreen %s in %s mode\n", ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");
	return 0;

err_input_register_device_failed:
	input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
err_power_failed:
	kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
	return ret;
}

/*20110310 seven.kim@lge.com to prevent IRQ during soft reset [START] */
void synaptics_ts_disable_irq()
{
	if (lcd_off_boot == 0) 
	      disable_irq(hub_ts_client->irq);
}
EXPORT_SYMBOL(synaptics_ts_disable_irq);
/*20110310 seven.kim@lge.com to prevent IRQ during soft reset [START] */

static int synaptics_ts_remove(struct i2c_client *client)
{
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);

#ifdef FEATURE_LGE_TOUCH_GRIP_SUPPRESSION	//20101216 seven.kim@lge.com [start]
	device_remove_file(&client->dev, &dev_attr_gripsuppression);
#endif //20101216 seven.kim@lge.com [end]

	unregister_early_suspend(&ts->early_suspend);
	if (ts->use_irq)
		free_irq(client->irq, ts);
	else
		hrtimer_cancel(&ts->timer);
	input_unregister_device(ts->input_dev);
	kfree(ts);
	return 0;
}

static int synaptics_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);

	    init_stabled = 0;
	
	    if (ts->use_irq)
		disable_irq(client->irq);
	    else
		hrtimer_cancel(&ts->timer);
	
		ret = cancel_work_sync(&ts->init_delayed_work); //seven.kim@lge.com sunggyun.ryu recommand
	    ret = cancel_work_sync(&ts->work);
	    if (ret && ts->use_irq) /* if work was pending disable-count is now 2 */
		enable_irq(client->irq);

#if 0 //20110222 seven.kim@lge.com blocked to prevent i2c error 
	    ret = synaptics_ts_i2c_write_byte_data(ts->client, SYNAPTICS_CONTROL_REG, SYNAPTICS_CONTROL_SLEEP); /* sleep */
	    if (ret < 0)
		pr_err("synaptics_ts_suspend: synaptics_ts_i2c_write_byte_data failed\n");
#endif //end of seven		

#ifdef FEATURE_LGE_TOUCH_GHOST_FINGER_IMPROVE
	    melt_mode = 0;
	    ghost_finger_1 = 0;
	    ghost_finger_2 = 0;
	    pressed = 0;    
	    ghost_count=0; //20110227 seven.kim@lge.com to prevent ghost finger for HW drop test
#endif

	return 0;
}


static int synaptics_ts_resume(struct i2c_client *client)
{
	int ret;
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);
	
	    init_stabled = 1;

   	    if (ts->use_irq)
		enable_irq(client->irq);

	    if (!ts->use_irq)
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

// 20100826 jh.koo@lge.com, for stable initialization [START_LGE]
	    schedule_delayed_work(&ts->init_delayed_work, msecs_to_jiffies(400));
// 20100826 jh.koo@lge.com, for stable initialization [END_LGE]
  	    memset(&ts_reg_data, 0x0, sizeof(ts_sensor_data));
	    memset(&prev_ts_data, 0x0, sizeof(ts_finger_data));
  	    memset(&curr_ts_data, 0x0, sizeof(ts_finger_data));	
	
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_ts_early_suspend(struct early_suspend *h)
{
	struct synaptics_ts_data *ts;
	ts = container_of(h, struct synaptics_ts_data, early_suspend);
	
		synaptics_ts_suspend(ts->client, PMSG_SUSPEND);
		
	return 0;
}

static void synaptics_ts_late_resume(struct early_suspend *h)
{
	struct synaptics_ts_data *ts;
	ts = container_of(h, struct synaptics_ts_data, early_suspend);
	
	    synaptics_ts_resume(ts->client);		
	
	return 0;
}
#endif

static const struct i2c_device_id synaptics_ts_id[] = {
	{ "hub_synaptics_ts", 0 },
	{ },
};

static struct i2c_driver synaptics_ts_driver = {
	.probe		= synaptics_ts_probe,
	.remove		= synaptics_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= synaptics_ts_suspend,
	.resume		= synaptics_ts_resume,
#endif
	.id_table	= synaptics_ts_id,
	.driver = {
		.name	= "hub_synaptics_ts",
		.owner = THIS_MODULE,
	},
};

static int __devinit synaptics_ts_init(void)
{
#ifdef FEATURE_LGE_TOUCH_REAL_TIME_WORK_QUEUE	//20101221 seven.kim@lge.com to use real time work queue
	synaptics_wq = create_rt_workqueue("synaptics_wq");
#else
	synaptics_wq = create_singlethread_workqueue("synaptics_wq");
#endif /*FEATURE_LGE_TOUCH_REAL_TIME_WORK_QUEUE*/

   	pr_warning("LGE: Synaptics ts_init\n");
	if (!synaptics_wq)
		return -ENOMEM;
	return i2c_add_driver(&synaptics_ts_driver);
}

static void __exit synaptics_ts_exit(void)
{
	i2c_del_driver(&synaptics_ts_driver);
    
	if (synaptics_wq)
		destroy_workqueue(synaptics_wq);
}

module_init(synaptics_ts_init);
module_exit(synaptics_ts_exit);

MODULE_DESCRIPTION("Synaptics Touchscreen Driver");
MODULE_LICENSE("GPL");

