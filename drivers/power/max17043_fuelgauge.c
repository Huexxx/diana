/*
 *  MAX17043_fuelgauge.c
 *  fuel-gauge systems for lithium-ion (Li+) batteries
 *
 *  Copyright (C) 2009 LG Electronics
 *  Dajin Kim <dajin.kim@lge.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/max17043_fuelgauge.h>
#include <linux/slab.h>
#include <mach/gpio.h>

// LGP970 (B-project) AP - Fuel gauge Version
#if defined(CONFIG_MACH_LGE_HUB) || defined(CONFIG_MACH_LGE_SNIPER)
#define GAUGE_INT	175
#define RCOMP_BL44JN	(0xB8)	/* Default Value for LGP970 Battery */
#if defined(CONFIG_HUB_CHARGING_IC)
#include "../hub/hub_charging_ic.h"
#endif
#if defined(CONFIG_HUB_MUIC)
#include "../hub/hub_muic.h"
#endif
extern void charger_state_update_by_other(void);	// from twl4030_bci_battery.c
#endif /* CONFIG_MACH_LGE_HUB || CONFIG_MACH_LGE_SNIPER */

#ifdef CONFIG_LGE_OMAP3_EXT_PWR
extern int get_external_power_status(void);
#endif 

#define MAX17043_VCELL_REG	0x02
#define MAX17043_SOC_REG	0x04
#define MAX17043_MODE_REG	0x06
#define MAX17043_VER_REG	0x08
#define MAX17043_CONFIG_REG	0x0C
#define MAX17043_CMD_REG	0xFE

#define MAX17043_VCELL_MSB	0x02
#define MAX17043_VCELL_LSB	0x03
#define MAX17043_SOC_MSB	0x04
#define MAX17043_SOC_LSB	0x05
#define MAX17043_MODE_MSB	0x06
#define MAX17043_MODE_LSB	0x07
#define MAX17043_VER_MSB	0x08
#define MAX17043_VER_LSB	0x09
#define MAX17043_CONFIG_MSB	0x0C
#define MAX17043_CONFIG_LSB	0x0D
#define MAX17043_CMD_MSB	0xFE
#define MAX17043_CMD_LSB	0xFF

#define MAX17043_WORK_DELAY	(10 * HZ)	// 10s
#define MAX17043_BATTERY_FULL	97		// Tuning Value
#define MAX17043_TOLERANCE	20		// Tuning Value

struct max17043_chip {
	struct i2c_client		*client;
	struct delayed_work		work;
	struct work_struct		alert_work;
	struct power_supply		battery;

	/* Max17043 Registers.(Raw Data) */
	int vcell;				// VCELL Register vaule
	int soc;				// SOC Register value
	int version;			// Max17043 Chip version
	int config;				// RCOMP, Sleep, ALRT, ATHD

	/* Interface with Android */
	int voltage;			// Battery Voltage   (Calculated from vcell)
	int capacity;			// Battery Capacity  (Calculated from soc)
	max17043_status status;	// State Of max17043
};

/*
 * Voltage Calibration Data
 *   voltage = (capacity * gradient) + intercept
 *   voltage must be in +-15%
 */
struct max17043_calibration_data {
	int voltage;	/* voltage in mA */
	int capacity;	/* capacity in % */
	int gradient;	/* gradient * 1000 */
	int intercept;	/* intercept * 1000 */
};
// 180mA Load for Battery
static struct max17043_calibration_data without_charger[] = {
	{3953,		81,		9,		3242},
	{3800,		58,		7,		3403},
	{3740,		40,		3,		3611},
	{3695,		20,		2,		3650},
	{3601,		4,		6,		3574},
	{3300,		0,		55,		3548},
	{ -1, -1, -1, -1},	// End of Data
};
// 770mA Charging Battery
static struct max17043_calibration_data with_charger[] = {
	{3865,		2,		66,		3709},
	{3956,		19,		5,		3851},
	{4021,		46,		2,		3912},
	{4088,		61,		5,		3813},
	{4158,		71,		7,		3689},
	{4200,		100,		2,		4042},
	{ -1, -1, -1, -1},	// End of Data
};

static struct max17043_chip* reference = NULL;
static int need_to_quickstart = 0;

static int max17043_write_reg(struct i2c_client *client, int reg, u16 value)
{
	int ret = 0;
	int retry = 3;

	value = ((value & 0xFF00) >> 8) | ((value & 0xFF) << 8);

	while(retry--) {
		ret = i2c_smbus_write_word_data(client, reg, value);

		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);			
		else
			break;
	}

	return ret;
}
static int max17043_read_reg(struct i2c_client *client, int reg)
{
	int ret = 0;
	int retry = 3;

	while(retry--) {
		ret = i2c_smbus_read_word_data(client, reg);

		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
		else
			break;
	}
	if(ret < 0)
		return ret;
	
	return ((ret & 0xFF00) >> 8) | ((ret & 0xFF) << 8);
}
static int max17043_reset(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);

	max17043_write_reg(client, MAX17043_CMD_REG, 0x5400);
	
	chip->status = MAX17043_RESET;
	dev_info(&client->dev, "MAX17043 Fuel-Gauge Reset\n");
	return 0;
}

static int max17043_quickstart(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);

	max17043_write_reg(client, MAX17043_MODE_REG, 0x4000);

	chip->status = MAX17043_QUICKSTART;
	dev_info(&client->dev, "MAX17043 Fuel-Gauge Quick-Start\n");
	return 0;
}
static int max17043_read_vcell(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);
	u16 value;
	
	value = max17043_read_reg(client, MAX17043_VCELL_REG);

	if(value < 0)
		return value;

	chip->vcell = value >> 4;
	
	return 0;
}
static int max17043_read_soc(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);
	u16 value;

	value = max17043_read_reg(client, MAX17043_SOC_REG);

	if(value < 0)
		return value;

	chip->soc = value;

	return 0;
}
static int max17043_read_version(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);
	u16 value;

	value = max17043_read_reg(client, MAX17043_VER_REG);
	
	chip->version = value;

	dev_info(&client->dev, "MAX17043 Fuel-Gauge Ver %d\n", value);

	return 0;
}
static int max17043_read_config(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);
	u16 value;

	value = max17043_read_reg(client, MAX17043_CONFIG_REG);

	if(value < 0)
		return value;

	chip->config = value;

	return 0;
}
static int max17043_write_config(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);

	max17043_write_reg(client, MAX17043_CONFIG_REG, chip->config);

	return 0;
}

static int max17043_need_quickstart(int charging)
{
	struct max17043_calibration_data* data;
	int i = 0;
	int expected;
	int diff;
	int vol;
	int level;

	if(reference == NULL)
		return 0;

	// Get Current Data
	vol = reference->voltage;
	level = reference->soc >> 8;
	if(level > 100)
		level = 100;
	else if(level < 0)
		level = 0;
	
	// choose data to use
	if(charging) {
		data = with_charger;
		while(data[i].voltage != -1) {
			if(vol <= data[i].voltage)
				break;
			i++;
		}
	} else {
		data = without_charger;
		while(data[i].voltage != -1) {
			if(vol >= data[i].voltage)
				break;
			i++;
		}
	}

	// absense of data
	if(data[i].voltage == -1) {
		if(charging) {
			if(level == 100)
				return 0;
			else /* Votlage > 4200 but Capacity is not 100% */
				return 1;
		}
		else {
			if(level == 0)
				return 0;
			else /* Voltage < 3300 but Capacity is not 0% */
				return 1;
		}
	}
	
	// calculate diff
	expected = (vol - data[i].intercept) / data[i].gradient;
	if(expected > 100)
		expected = 100;
	else if(expected < 0)
		expected = 0;
	diff = expected - level;

	// judge
	if(diff < -MAX17043_TOLERANCE || diff > MAX17043_TOLERANCE) {
		//printk(KERN_DEBUG "[BATTERY] real : %d%% , expected : %d%%\n", level, expected);
		need_to_quickstart += 1;
	} else {
		need_to_quickstart = 0;
	}

	// Maximum continuous reset time is 2. If reset over 2 times, discard it.
	if(need_to_quickstart > 2)
		need_to_quickstart = 0;
		
	return need_to_quickstart;
}
static int max17043_next_alert_level(int level)
{
	if(level >= 16)
		return 16;
	else if(level <= 5)
		return level;
	else
		return 5;
}
static int max17043_set_rcomp(int rcomp)
{
	if(reference == NULL)
		return -1;

	rcomp &= 0xff;
	reference->config = ((reference->config & 0x00ff) | (rcomp << 8));

	max17043_write_config(reference->client);

	return 0;
}
static int max17043_set_athd(int level)
{
	if(reference == NULL)
		return -1;

	if(level > 32)
		level = 32;
	else if(level < 1)
		level = 1;

	level = 32 - level;
	if(level == (reference->config & 0x1F))
		return level;

	reference->config = ((reference->config & 0xffe0) | level);
	max17043_write_config(reference->client);

	return level;
}
static int max17043_clear_interrupt(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);

	if(chip->config & 0x20) {
		chip->config &= 0xffdf;
		max17043_write_config(chip->client);
	}
	
	return 0;
}
static int max17043_update(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);
	int ret;
	
	ret = max17043_read_vcell(client);
	if(ret < 0)
		return ret;
	ret = max17043_read_soc(client);
	if(ret < 0)
		return ret;
	
	/* convert raw data to usable data */
	chip->voltage = (chip->vcell * 5) >> 2;	// vcell * 1.25 mV
	chip->capacity = chip->soc >> 8;
	// adjust full condition...just hack...
	if(chip->capacity >= 72)
		chip->capacity++;
	if(chip->capacity >= 64)
		chip->capacity++;
	if(chip->capacity >= 35)
		chip->capacity++;
	if(chip->soc & 0x80)	// half up
		chip->capacity++;
	
	if(chip->capacity > 100)
		chip->capacity = 100;
	else if(chip->capacity < 0)
		chip->capacity = 0;
	
	chip->status = MAX17043_WORKING;

	return 0;
}
static void max17043_work(struct work_struct *work)
{
	struct max17043_chip *chip;
	int alert_level;
	int charging = 0;

	chip = container_of(work, struct max17043_chip, work.work);
#if defined(CONFIG_HUB_CHARGING_IC)
	if(get_charging_ic_status() != CHARGING_IC_DEACTIVE)
		charging = 1;
#endif
	switch(chip->status) {
		case MAX17043_RESET:
			max17043_read_version(chip->client);
			max17043_read_config(chip->client);
			max17043_set_rcomp(RCOMP_BL44JN);
		case MAX17043_QUICKSTART:
			max17043_update(chip->client);
			if(max17043_need_quickstart(charging)) {
				max17043_reset(chip->client);
				schedule_delayed_work(&chip->work,  HZ);
				return;
			}
			alert_level = max17043_next_alert_level(chip->capacity);
			max17043_set_athd(alert_level);
			max17043_clear_interrupt(chip->client);
			need_to_quickstart = 0;
			break;
		case MAX17043_WORKING:
		default:
			max17043_update(chip->client);
			break;
	}
	alert_level = max17043_next_alert_level(chip->capacity);
	max17043_set_athd(alert_level);
	schedule_delayed_work(&chip->work, MAX17043_WORK_DELAY);
	charger_state_update_by_other();
}

static void max17043_alert_work(struct work_struct *work)
{
	int alert_level;

	if(reference == NULL)
		return;

	if(reference->status == MAX17043_WORKING) {
		cancel_delayed_work_sync(&reference->work);	
		schedule_delayed_work(&reference->work, MAX17043_WORK_DELAY);
	}
	
	max17043_read_config(reference->client);
	
	max17043_update(reference->client);

	alert_level = max17043_next_alert_level(reference->capacity);
	max17043_set_athd(alert_level);
	max17043_clear_interrupt(reference->client);

#if defined(CONFIG_HUB_MUIC)
	if(get_muic_mode() == MUIC_NONE)
#elif defined(CONFIG_LGE_OMAP3_EXT_PWR)
	if ( 0 == get_external_power_status() )
#else
#error
#endif
	{
		printk(KERN_INFO "[Battery] Low Level Alert (%d)\n",reference->capacity);
		charger_state_update_by_other();
	}
}

static irqreturn_t max17043_interrupt_handler(int irq, void *data)
{
	if(reference == NULL) {
		return IRQ_HANDLED;
	}
	schedule_work(&reference->alert_work);
	return IRQ_HANDLED;
}
#if 0	// B-Project Does not use fuel gauge as a battery driver
/* sysfs(power_supply) interface : for Android Battery Service [START] */
static enum power_supply_property max17043_battery_props[] = {
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
};
static int max17043_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct max17043_chip *chip = container_of(psy,
				struct max17043_chip, battery);

	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = chip->voltage;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = chip->capacity;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
/* sysfs interface : for Android Battery Service [END] */
#endif
/* sysfs interface : for AT Commands [START] */
ssize_t max17043_show_soc(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	int level;
	
	if(reference == NULL)
		return snprintf(buf, PAGE_SIZE, "ERROR\n");

	level = ((reference->soc) >> 8);
	if(level > 100)
		level = 100;
	else if(level < 0)
		level = 0;
		
	return snprintf(buf, PAGE_SIZE, "%d\n", level);
}
DEVICE_ATTR(soc, 0444, max17043_show_soc, NULL);
ssize_t max17043_show_status(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	if(reference == NULL)
		return snprintf(buf, PAGE_SIZE, "ERROR\n");

	switch(reference->status) {
		case MAX17043_RESET:
			return snprintf(buf, PAGE_SIZE, "reset\n");
		case MAX17043_QUICKSTART:
			return snprintf(buf, PAGE_SIZE, "quickstart\n");
		case MAX17043_WORKING:
			return snprintf(buf, PAGE_SIZE, "working\n");
		default:
			return snprintf(buf, PAGE_SIZE, "ERROR\n");
	}
}
ssize_t max17043_store_status(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf,
			  size_t count)
{
	if(reference == NULL)
		return -1;

	if(strncmp(buf,"reset",5) == 0) {
		cancel_delayed_work(&reference->work);
		max17043_reset(reference->client);
		schedule_delayed_work(&reference->work, HZ);
	} else if(strncmp(buf,"quickstart",10) == 0) {
		cancel_delayed_work(&reference->work);
		max17043_quickstart(reference->client);
		schedule_delayed_work(&reference->work, HZ);
	} else if(strncmp(buf,"working",7) == 0) {
		// do nothing
	} else {
		return -1;
	}
	return count;
}
DEVICE_ATTR(state, 0664, max17043_show_status, max17043_store_status);
/* sysfs interface : for AT Commands [END] */

/* SYMBOLS to use outside of this module */
int max17043_get_capacity(void)
{
	if(reference == NULL)	// if fuel gauge is not initialized,
		return 100;			// return Dummy Value
	return reference->capacity;
}
EXPORT_SYMBOL(max17043_get_capacity);
int max17043_get_voltage(void)
{
	if(reference == NULL)	// if fuel gauge is not initialized,
		return 4200;		// return Dummy Value
	return reference->voltage;
}
EXPORT_SYMBOL(max17043_get_voltage);
int max17043_do_calibrate(void)
{
	int need_quickstart = 0;
	int charging = 0;

	if(reference == NULL)
		return -1;

#if defined(CONFIG_HUB_MUIC)
	if(get_muic_mode() != MUIC_NONE)
#elif defined(CONFIG_LGE_OMAP3_EXT_PWR)
	if ( 1 == get_external_power_status() )
#else
#error
#endif
	{
		charging = 1;
	}

	
	need_quickstart = max17043_need_quickstart(charging);

	if(need_quickstart) {
		max17043_quickstart(reference->client);
	}
	cancel_delayed_work(&reference->work);
	schedule_delayed_work(&reference->work, HZ);

	return 0;
}
EXPORT_SYMBOL(max17043_do_calibrate);
int max17043_set_rcomp_by_temperature(int temp)
{
	int rcomp;
	if(reference == NULL)
		return -1;	// MAX17043 not initialized

	rcomp = RCOMP_BL44JN;
	
	// RCOMP compensation refer to temperature
	if(temp < 200)
		rcomp = rcomp + ((200 - temp) / 2);
	else if(temp > 200)
		rcomp = rcomp - ((temp - 200) * 11 / 50);

	max17043_set_rcomp(rcomp);
	return 0;
}
EXPORT_SYMBOL(max17043_set_rcomp_by_temperature);
int max17043_set_alert_level(int alert_level)
{
	return max17043_set_athd(alert_level);
}
EXPORT_SYMBOL(max17043_set_alert_level);
/* End SYMBOLS */

static int __devinit max17043_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct max17043_chip *chip;
	int ret = 0;
	int alert_level;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WORD_DATA))
		return -EIO;

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	ret = gpio_request(GAUGE_INT, "max17043_alert");
	if (ret < 0) {
		printk(KERN_DEBUG " [MAX17043] GPIO Request Failed\n");
		goto err_gpio_request_failed;
	}
	gpio_direction_input(GAUGE_INT);

	ret = request_irq(gpio_to_irq(GAUGE_INT),
			max17043_interrupt_handler,
			IRQF_TRIGGER_FALLING,
			"MAX17043_Alert", NULL);
	if (ret < 0) {
		printk(KERN_DEBUG " [MAX17043] IRQ Request Failed\n");
		goto err_request_irq_failed;
	}

	ret = enable_irq_wake(gpio_to_irq(GAUGE_INT));
	if (ret < 0) {
		printk(KERN_DEBUG "[MAX17043] set GAUGE_INT to wakeup source failed.\n");
		goto err_request_wakeup_irq_failed;
	}
	//enable_irq(gpio_to_irq(GAUGE_INT));
	chip->client = client;

	i2c_set_clientdata(client, chip);

#if 0	// B-Project Does not use fuel gauge as a battery driver
	chip->battery.name		= "battery";
	chip->battery.type		= POWER_SUPPLY_TYPE_BATTERY;
	chip->battery.get_property	= max17043_get_property;
	chip->battery.properties	= max17043_battery_props;
	chip->battery.num_properties	= ARRAY_SIZE(max17043_battery_props);

	ret = power_supply_register(&client->dev, &chip->battery);
	if (ret) {
		dev_err(&client->dev, "failed: power supply register\n");
		goto err_power_supply_register_failed;
	}
#endif

	// sysfs path : /sys/devices/platform/i2c_omap.2/i2c-2/2-0036/soc
	ret = device_create_file(&client->dev, &dev_attr_soc);
	if (ret < 0) {
		pr_err("%s:File device creation failed: %d\n", __func__, ret);
		ret = -ENODEV;
		goto err_create_file_soc_failed;
	}
	// sysfs path : /sys/devices/platform/i2c_omap.2/i2c-2/2-0036/state
	ret = device_create_file(&client->dev, &dev_attr_state);
	if (ret < 0) {
		pr_err("%s:File device creation failed: %d\n", __func__, ret);
		ret = -ENODEV;
		goto err_create_file_state_failed;
	}
	
	chip->vcell = 3360;
	chip->soc = 100 << 8;
	chip->voltage = 4200;
	chip->capacity = 100;
	chip->config = 0x971C;

	INIT_DELAYED_WORK_DEFERRABLE(&chip->work, max17043_work);
	INIT_WORK(&chip->alert_work, max17043_alert_work);

	reference = chip;	
	max17043_read_version(client);
	max17043_read_config(client);

	if(need_to_quickstart) {
		max17043_set_rcomp(RCOMP_BL44JN);
		max17043_quickstart(client);
		schedule_delayed_work(&chip->work, HZ);
		return 0;
	}
	
	max17043_update(client);
	alert_level = max17043_next_alert_level(chip->capacity);
	max17043_set_athd(alert_level);
	max17043_clear_interrupt(client);
	schedule_delayed_work(&chip->work, MAX17043_WORK_DELAY);
	
	return 0;

err_create_file_state_failed:
	device_remove_file(&client->dev, &dev_attr_soc);
err_create_file_soc_failed:
#if 0	// B-Project. Does not use fuel gauge as a battery driver
err_power_supply_register_failed:
	i2c_set_clientdata(client, NULL);
#endif
	kfree(chip);
	disable_irq_wake(gpio_to_irq(GAUGE_INT));
err_request_wakeup_irq_failed:
	free_irq(gpio_to_irq(GAUGE_INT), NULL);
err_request_irq_failed:
	gpio_free(GAUGE_INT);
err_gpio_request_failed:

	return ret;
}

static int __devexit max17043_remove(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);

	power_supply_unregister(&chip->battery);
	cancel_delayed_work(&chip->work);
	flush_work(&chip->alert_work);
	i2c_set_clientdata(client, NULL);
	kfree(chip);
	return 0;
}

#ifdef CONFIG_PM
static int max17043_suspend(struct i2c_client *client,
		pm_message_t state)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);

	cancel_delayed_work(&chip->work);
	flush_work(&chip->alert_work);
	client->dev.power.power_state = state;

	return 0;
}

static int max17043_resume(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);

	if(max17043_update(client) >= 0)
		charger_state_update_by_other();
	schedule_delayed_work(&chip->work, MAX17043_WORK_DELAY);
	client->dev.power.power_state = PMSG_ON;

	return 0;
}
#else
#define max17043_suspend NULL
#define max17043_resume NULL
#endif /* CONFIG_PM */

static const struct i2c_device_id max17043_id[] = {
	{ "max17043", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max17043_id);

static struct i2c_driver max17043_i2c_driver = {
	.driver	= {
		.name	= "max17043",
		.owner	= THIS_MODULE,
	},
	.probe		= max17043_probe,
	.remove		= __devexit_p(max17043_remove),
	.suspend	= max17043_suspend,
	.resume		= max17043_resume,
	.id_table	= max17043_id,
};

/* boot argument from boot loader */
static s32 __init max17043_state(char *str)
{
	switch(str[0]) {
		case 'b':	// battery not connected when booting
		case 'g':	// fuel gauge value is good
		case 'q':	// did quikcstart.
			need_to_quickstart = 0;
			break;
		case 'e':	// quickstart needed. but error occured.
			need_to_quickstart = 1;
			break;
		default:
			// can not enter here
			break;
	}
	return 0;
}
__setup("fuelgauge=", max17043_state);

#if 1 // B-Project Rev.D
void __init max17043_init(void)
{
	i2c_add_driver(&max17043_i2c_driver);
}
#else
static int __init max17043_init(void)
{
	return i2c_add_driver(&max17043_i2c_driver);
}
module_init(max17043_init);

static void __exit max17043_exit(void)
{
	i2c_del_driver(&max17043_i2c_driver);
}
module_exit(max17043_exit);
#endif

MODULE_AUTHOR("LG Electronics");
MODULE_DESCRIPTION("MAX17043 Fuel Gauge");
MODULE_LICENSE("GPL");
