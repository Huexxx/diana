/* 
 * Copyright (C) 2009 LGE Inc.
 * 
 * Heaven Motion header file.
 *
 * 2009/09/05 : created by <chpark96@lge.com>
 * 
 */
 
 #ifndef __HEAVEN_MOTION_H__
#define __HEAVEN_MOTION_H__


/*==================================================================================================
				LGE Definitions
==================================================================================================*/

#define  HEAVEN_MOTION_SENSOR_NAME   	       "motion_sensor"        /* platform device*/

#define  HEAVEN_MOTION_IOCTL_NAME      		"motion_daemon"     /* for heaven_motion daemon process */

#define  HEAVEN_MOTION_INPUT_NAME      		"lge_gesture"

#define  HEAVEN_I2C_ACCEL_NAME            			 "heaven_accel"

#define  HEAVEN_I2C_GYRO_NAME              			"heaven_gyro"


typedef enum
{
	HEAVEN_SENSOR_NONE 	    = 0x00,
	HEAVEN_ACCELEROMETER   = 0x01,
	HEAVEN_ORIENTATION 	    = 0x02,
	HEAVEN_TILT  			    = 0x04,
	HEAVEN_SHAKE 			    = 0x08,
	HEAVEN_SNAP 			    = 0x10,
	HEAVEN_FLIP 			    = 0x20,
	HEAVEN_TAP 			    = 0x40,
	HEAVEN_YAWIMAGE		= 0x80,
	HEAVEN_GYRO			= 0x100,
	HEAVEN_COMPASS		= 0x200,
	HEAVEN_CALIBRATION      = 0x400,
	HEAVEN_COMPOSITE		= 0x800,
}heaven_sensor_enum_type;


/*-------------------------------------------------------------------*/
/*		   			                   IOCTL					                        */
/*-------------------------------------------------------------------*/

/*----------------- motion daemon process -------------------------*/
#define MOTION_MAGIC					     		0xA2

#define MOTION_IOCTL_ENABLE_DISABLE	    		_IOWR(MOTION_MAGIC, 0x01, short)
#define MOTION_IOCTL_SENSOR_DELAY	    		_IOWR(MOTION_MAGIC, 0x02, short)
#define MOTION_IOCTL_READ_ACCEL_DATA   		_IOWR(MOTION_MAGIC, 0x03, char[10])   // ACCEL RAW DATA
#define MOTION_IOCTL_ACCEL_RAW      	    		_IOWR(MOTION_MAGIC, 0x04, int[5])
#define MOTION_IOCTL_TILT            	    	    		_IOWR(MOTION_MAGIC, 0x05, int[5])
#define MOTION_IOCTL_SHAKE        	    	    		_IOWR(MOTION_MAGIC, 0x06, int[5])
#define MOTION_IOCTL_SNAP        	    	    		_IOWR(MOTION_MAGIC, 0x07, int[5])
#define MOTION_IOCTL_FLIP        	    	    	    		_IOWR(MOTION_MAGIC, 0x08, int[5])
#define MOTION_IOCTL_TAP             	    	    		_IOWR(MOTION_MAGIC, 0x09, int[5])
#define MOTION_IOCTL_COMPOSITE             	    	    		_IOWR(MOTION_MAGIC, 0x0A, int[5])
#define MOTION_IOCTL_YAWIMAGE         	    	_IOWR(MOTION_MAGIC, 0x0B, int[5])
#define MOTION_IOCTL_MAGNETIC_RAW      	    		_IOWR(MOTION_MAGIC, 0x10, int[5])
#define MOTION_IOCTL_GYRO_RAW      	    		_IOWR(MOTION_MAGIC, 0x11, int[5])

// for  MPL library - MPU3050 Gyroscpoe
#define MOTION_IOCTL_ACCEL_COMPASS_SLEEP_WAKE_UP 		_IOWR(MOTION_MAGIC, 0x0C, short)
#define MOTION_IOCTL_ACCEL_COMPASS_SLEEP_MODE   		_IOWR(MOTION_MAGIC, 0x0D, short)
#define MOTION_IOCTL_MPU3050_I2C_READ     	   	_IOWR(MOTION_MAGIC, 0x0E, char[200])
#define MOTION_IOCTL_MPU3050_I2C_WRITE   	   	_IOWR(MOTION_MAGIC, 0x0F, char[200])
#define MOTION_IOCTL_SENSOR_SUSPEND_RESUME	    		_IOWR(MOTION_MAGIC, 0x12, short)
#define MOTION_IOCTL_ACCEL_INT_ENABLE_DISABLE 		_IOWR(MOTION_MAGIC, 0x13, short)
#define MOTION_IOCTL_ACCEL_INIT 		_IOWR(MOTION_MAGIC, 0x14, short)
#define MOTION_IOCTL_CALIBRATION_FINISHED 		_IOWR(MOTION_MAGIC, 0x15, int[5])

void motion_send_tap_detection(int type,int direction);
void motion_send_flip_detection(int value);

#endif //__MPU3050_H__

