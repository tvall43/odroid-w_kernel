/*
 * rtc-rc5t619.c -- RICOH RC5T619 Real Time Clock
 *
 * Copyright (C) 2014 Hardkernel Co.,Ltd.
 * Hakjoo Kim <ruppi.kim@hardkernel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 *
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/mfd/rc5t619.h>

enum {
	RTC_SEC = 0,
	RTC_MIN,
	RTC_HOUR,
	RTC_WEEKDAY,
	RTC_MONTH,
	RTC_YEAR,
	RTC_DATE,
	RTC_NR_TIME
};

struct rc5t619_rtc_info {
	struct device		*dev;
	struct rc5t619_dev	*rc5t619;
	struct i2c_client	*rtc;
	struct rtc_device	*rtc_dev;
	struct mutex		lock;
	/* To store the list of enabled interrupts, during system suspend */
	u32 irqen;

	struct regmap		*regmap;

	int virq;
	int rtc_24hr_mode;
};

/* Total number of RTC registers needed to set time*/
#define NUM_TIME_REGS	(RC5T619_RTC_YEAR - RC5T619_RTC_SEC + 1)

/* Total number of RTC registers needed to set Y-Alarm*/
#define NUM_YAL_REGS	(RC5T619_RTC_AY_YEAR - RC5T619_RTC_AY_MIN + 1)

/* Set Y-Alarm interrupt */
#define SET_YAL BIT(5)

/* Get Y-Alarm interrupt status*/
#define GET_YAL_STATUS BIT(0)

#define RTC_YEAR_OFFSET 100
#define OS_REF_YEAR 1900

static int rc5t619_rtc_alarm_irq_enable(struct device *dev, unsigned enabled)
{
	struct rc5t619_dev *rc5t619 = dev_get_drvdata(dev->parent);
	u8 val;
	dev_info(rc5t619->dev, "%s\n", __func__);

	/* Set Y-Alarm, based on 'enabled' */
	val = enabled ? SET_YAL : 0;

	return regmap_update_bits(rc5t619->regmap, RC5T619_RTC_CTL1, SET_YAL,
		val);
}

static void print_time(struct device *dev, struct rtc_time *tm)
{
	dev_info(dev, "rtc-time : %d/%d/%d %d:%d:%d\n",
	(tm->tm_mon + 1), tm->tm_mday, (tm->tm_year + 1970),
		tm->tm_hour, tm->tm_min, tm->tm_sec);
}

static int rc5t619_rtc_valid_tm(struct device *dev, struct rtc_time *tm)
{
	if (tm->tm_year >= (RTC_YEAR_OFFSET +99) 
		|| tm->tm_mon > 12
		|| tm->tm_mday < 1
		|| tm->tm_mday > rtc_month_days(tm->tm_mon,
			tm->tm_year + OS_REF_YEAR)
		|| tm->tm_hour >= 24
		|| tm->tm_min >= 60) {

		return -EINVAL;
	}
	return 0;
}

/*
 * Gets current rc5t619 RTC time and date parameters.
 *
 * The RTC's time/alarm representation is not what gmtime(3) requires
 * Linux to use:
 *
 *  - Months are 1..12 vs Linux 0-11
 *  - Years are 0..99 vs Linux 1900..N (we assume 21st century)
 */
static int rc5t619_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct rc5t619_dev *rc5t619 = dev_get_drvdata(dev->parent);
	u8 rtc_data[NUM_TIME_REGS];
	int ret;
	//dev_info(rc5t619->dev, "%s\n", __func__);

	ret = regmap_bulk_read(rc5t619->regmap, RC5T619_RTC_SEC, rtc_data,
		sizeof(rtc_data));
	if (ret < 0) {
		dev_err(dev, "RTC read time failed with err:%d\n", ret);
		return ret;
	}

	tm->tm_sec = bcd2bin(rtc_data[0]);
	tm->tm_min = bcd2bin(rtc_data[1]);
	tm->tm_hour = bcd2bin(rtc_data[2]);
	tm->tm_wday = bcd2bin(rtc_data[3]);
	tm->tm_mday = bcd2bin(rtc_data[4]);
	tm->tm_mon = bcd2bin(rtc_data[5]) - 1;
	tm->tm_year = bcd2bin(rtc_data[6]) + 100;

	//print_time(dev, tm);
	return ret;
}

static int rc5t619_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct rc5t619_dev *rc5t619 = dev_get_drvdata(dev->parent);
	unsigned char rtc_data[NUM_TIME_REGS];
	int ret;
	
	//dev_info(rc5t619->dev, "%s\n", __func__);
	rtc_data[0] = bin2bcd(tm->tm_sec);
	rtc_data[1] = bin2bcd(tm->tm_min);
	rtc_data[2] = bin2bcd(tm->tm_hour);
	rtc_data[3] = bin2bcd(tm->tm_wday);
	rtc_data[4] = bin2bcd(tm->tm_mday);
	rtc_data[5] = bin2bcd(tm->tm_mon + 1);
	rtc_data[6] = bin2bcd(tm->tm_year - 100);

	ret = regmap_bulk_write(rc5t619->regmap, RC5T619_RTC_SEC, rtc_data,
		sizeof(rtc_data));
	if (ret < 0) {
		dev_err(dev, "RTC set time failed with error %d\n", ret);
		return ret;
	}

	//print_time(dev, tm);
	return ret;
}

static int rc5t619_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	struct rc5t619_dev *rc5t619 = dev_get_drvdata(dev->parent);
	unsigned char alarm_data[NUM_YAL_REGS];
	u32 interrupt_enable;
	int ret;
	dev_info(rc5t619->dev, "%s\n", __func__);

	ret = regmap_bulk_read(rc5t619->regmap, RC5T619_RTC_AY_MIN, alarm_data,
		NUM_YAL_REGS);
	if (ret < 0) {
		dev_err(dev, "rtc_read_alarm error %d\n", ret);
		return ret;
	}

	alm->time.tm_min = bcd2bin(alarm_data[0]);
	alm->time.tm_hour = bcd2bin(alarm_data[1]);
	alm->time.tm_mday = bcd2bin(alarm_data[2]);
	alm->time.tm_mon = bcd2bin(alarm_data[3]) - 1;
	alm->time.tm_year = bcd2bin(alarm_data[4]) + 100;

	ret = regmap_read(rc5t619->regmap, RC5T619_RTC_CTL1, &interrupt_enable);
	if (ret < 0)
		return ret;

	/* check if YALE is set */
	if (interrupt_enable & SET_YAL)
		alm->enabled = 1;

	return ret;
}

static int rc5t619_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	struct rc5t619_dev *rc5t619 = dev_get_drvdata(dev->parent);
	unsigned char alarm_data[NUM_YAL_REGS];
	int ret;

	dev_info(rc5t619->dev, "%s\n", __func__);
	ret = rc5t619_rtc_alarm_irq_enable(dev, 0);
	if (ret)
		return ret;

	alarm_data[0] = bin2bcd(alm->time.tm_min);
	alarm_data[1] = bin2bcd(alm->time.tm_hour);
	alarm_data[2] = bin2bcd(alm->time.tm_mday);
	alarm_data[3] = bin2bcd(alm->time.tm_mon + 1);
	alarm_data[4] = bin2bcd(alm->time.tm_year - 100);

	ret = regmap_bulk_write(rc5t619->regmap, RC5T619_RTC_AY_MIN, alarm_data,
		NUM_YAL_REGS);
	if (ret) {
		dev_err(dev, "rtc_set_alarm error %d\n", ret);
		return ret;
	}

	if (alm->enabled)
		ret = rc5t619_rtc_alarm_irq_enable(dev, 1);

	return ret;
}

static irqreturn_t rc5t619_rtc_interrupt(int irq, void *rtc)

{
	struct device *dev = rtc;
	struct rc5t619_dev *rc5t619 = dev_get_drvdata(dev->parent);
	struct rc5t619_rtc_info *info = dev_get_drvdata(dev);
	unsigned long events = 0;
	int ret;
	u32 rtc_reg;

	dev_info(rc5t619->dev, "%s\n", __func__);
	ret = regmap_read(rc5t619->regmap, RC5T619_RTC_CTL2, &rtc_reg);

	dev_info(rc5t619->dev, "rtc_ctl2 : 0x%x\n",rtc_reg);
	if (ret < 0)
		return IRQ_NONE;

	if (rtc_reg & GET_YAL_STATUS) {
		events = RTC_IRQF | RTC_AF;
		/* clear pending Y-alarm interrupt bit */
		rtc_reg &= ~GET_YAL_STATUS | ~0x80;
	}

	ret = regmap_write(rc5t619->regmap, RC5T619_RTC_CTL2, rtc_reg);
	if (ret)
		return IRQ_NONE;

	events = RTC_IRQF | RTC_AF;
	/* Notify RTC core on event */
	rtc_update_irq(info->rtc_dev, 1, events);

	return IRQ_HANDLED;
}

#define MODEL24_SHIFT		5
#define MODEL24_MASK		(1 << MODEL24_SHIFT)
#define ALARMD_SHIFT		6
#define ALARMD_MASK		(1 << ALARMD_SHIFT)

static int rc5t619_rtc_init_reg(struct rc5t619_rtc_info *info)
{
	int ret;
	struct rtc_time tm;
	u32 rtc_reg;

	/* Set RTC control register : 24hour mdoe and alarm d */
	rtc_reg =  (1 << MODEL24_SHIFT) | (1 << ALARMD_SHIFT);

	/* to enable alarm_d and 24-hour format */
	ret = regmap_write(info->rc5t619->regmap, RC5T619_RTC_CTL1, rtc_reg);
	if (ret < 0) {
		dev_err(info->dev, "%s: fail to write rtc control 1 reg(%d)\n",
				__func__, ret);
		return ret;
	}
	info->rtc_24hr_mode = 1;

	/* clear RTC Adjust register */
	rtc_reg =0;
	ret = regmap_write(info->rc5t619->regmap, RC5T619_RTC_ADJ, 0);

	ret = regmap_read(info->rc5t619->regmap, RC5T619_RTC_CTL2, &rtc_reg);
	if (ret < 0) {
		dev_err(info->rc5t619->dev, "%s: fail to read rtc control 2 reg(%d)\n",
				__func__, ret);
		return ret;
	}

	/* Set default time-2014.7.17-00h:0m:0s if PON is on */
	if(rtc_reg&0x10) {
		dev_err(info->rc5t619->dev, "%s: PON=1 -- CRTL2=%x\n", __func__, rtc_reg);
		tm.tm_sec = 0;
		tm.tm_min = 0;
		tm.tm_hour = 12;
		tm.tm_wday = 3;
		tm.tm_mday = 17;
		tm.tm_mon = 6;
		tm.tm_year = 18;
		/* VDET & PON =0, others are not changed */
		rtc_reg &=~ 0x50;
		ret = regmap_write(info->rc5t619->regmap, RC5T619_RTC_CTL2, rtc_reg);
		rc5t619_rtc_set_time(info->dev, &tm);
	} else {
		dev_err(info->rc5t619->dev, "%s: rtc_reg(0x%x)\n", __func__, rtc_reg);
		rc5t619_rtc_read_time(info->dev, &tm);
	}

	return ret;
}


static const struct rtc_class_ops rc5t619_rtc_ops = {
	.read_time	= rc5t619_rtc_read_time,
	.set_time	= rc5t619_rtc_set_time,
//	.read_alarm	= rc5t619_rtc_read_alarm,
//	.set_alarm	= rc5t619_rtc_set_alarm,
//	.alarm_irq_enable = rc5t619_rtc_alarm_irq_enable,
};

static int rc5t619_rtc_probe(struct platform_device *pdev)
{
	struct rc5t619_dev *rc5t619 = dev_get_drvdata(pdev->dev.parent);
	struct rc5t619_rtc_info *info;
	int ret;

	dev_info(&pdev->dev, "%s\n", __func__);

	info = devm_kzalloc(&pdev->dev, sizeof(struct rc5t619_rtc_info),
			GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->dev = &pdev->dev;
	info->rc5t619 = rc5t619;

	platform_set_drvdata(pdev, info);

	ret = rc5t619_rtc_init_reg(info);

	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to initialize RTC reg:%d\n", ret);
		goto err_rtc;
	}

	device_init_wakeup(&pdev->dev, 1);
	info->rtc_dev = devm_rtc_device_register(&pdev->dev, pdev->name,
		&rc5t619_rtc_ops, THIS_MODULE);
	if (IS_ERR(info->rtc_dev)) {
		ret = PTR_ERR(info->rtc_dev);
		dev_err(&pdev->dev, "RTC device register: err %d\n", ret);
		return ret;
	}
#if 0
	info->virq = regmap_irq_get_virq(rc5t619->irq_data_rtc, 0);
	if (info->virq < 0)
		return info->virq;
	ret = devm_request_threaded_irq(&pdev->dev, info->virq, NULL,
		rc5t619_rtc_interrupt, IRQF_ONESHOT,
		"rc5t619-alarm", &pdev->dev);

	if (ret < 0)
		dev_err(&pdev->dev, "Failed to request IRQ%d: %d\n",
			info->virq, ret);
#endif

err_rtc:
	return 0;
}

/*
 * Disable rc5t619 RTC interrupts.
 * Sets status flag to free.
 */
static int rc5t619_rtc_remove(struct platform_device *pdev)
{
	struct rc5t619_rtc_info *info = platform_get_drvdata(pdev); 

	rc5t619_rtc_alarm_irq_enable(&info->rtc_dev->dev, 0);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int rc5t619_rtc_suspend(struct device *dev)
{
	struct rc5t619_dev *rc5t619 = dev_get_drvdata(dev->parent);
	struct rc5t619_rtc_info *info = dev_get_drvdata(dev);
	int ret;

	/* Store current list of enabled interrupts*/
	ret = regmap_read(rc5t619->regmap, RC5T619_RTC_CTL1,
		&info->irqen);
	return ret;
}

static int rc5t619_rtc_resume(struct device *dev)
{
	struct rc5t619_dev *rc5t619 = dev_get_drvdata(dev->parent);
	struct rc5t619_rtc_info *info = dev_get_drvdata(dev);

	/* Restore list of enabled interrupts before suspend */
	return regmap_write(rc5t619->regmap, RC5T619_RTC_CTL1,
		info->irqen);
}
#endif

static SIMPLE_DEV_PM_OPS(rc5t619_rtc_pm_ops, rc5t619_rtc_suspend,
			rc5t619_rtc_resume);

static const struct platform_device_id rtc_id[] = {
	{ "rc5t619-rtc", 0 },
	{},
};

static struct platform_driver rc5t619_rtc_driver = {
	.probe		= rc5t619_rtc_probe,
	.remove		= rc5t619_rtc_remove,
	.id_table	= rtc_id,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "rtc-rc5t619",
		.pm	= &rc5t619_rtc_pm_ops,
	},
};

module_platform_driver(rc5t619_rtc_driver);
MODULE_ALIAS("platform:rtc-rc5t619");
MODULE_AUTHOR("Hakjo Kim <ruppi.kim@hardkernel.com>");
MODULE_LICENSE("GPL v2");
