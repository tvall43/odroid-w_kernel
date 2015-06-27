/*
 * Core driver access RC5T619 power management chip.
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
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/pm_runtime.h>
#include <linux/module.h>
#include <linux/mfd/core.h>
#include <linux/mfd/rc5t619.h>
#include <linux/regmap.h>
#include <mach/irqs.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#define RICOH_ONOFFSEL_REG	0x09

struct deepsleep_control_data {
	u8 reg_add;
	u8 ds_pos_bit;
};

#define DEEPSLEEP_INIT(_id, _reg, _pos)		\
	{					\
		.reg_add = RC5T619_##_reg,	\
		.ds_pos_bit = _pos,		\
	}

static struct deepsleep_control_data deepsleep_data[] = {
	DEEPSLEEP_INIT(DC1, DC1_SLOT, 0),
	DEEPSLEEP_INIT(DC2, DC2_SLOT, 0),
	DEEPSLEEP_INIT(DC3, DC3_SLOT, 0),
	DEEPSLEEP_INIT(DC4, DC4_SLOT, 0),
	DEEPSLEEP_INIT(DC5, DC5_SLOT, 0),
};

#if 0
#define EXT_PWR_REQ		\
	(RC5T619_EXT_PWRREQ1_CONTROL | RC5T619_EXT_PWRREQ2_CONTROL)
#endif

static struct mfd_cell rc5t619_subdevs[] = {
	{.name = "rc5t619-gpio",},
	{.name = "rc5t619-regulator",},
	{.name = "rc5t619-rtc",      },
//	{.name = "rc5t619-key",      }
};

static int __rc5t619_set_ext_pwrreq1_control(struct device *dev,
	int id, int ext_pwr, int slots)
{
	int ret;
	uint8_t sleepseq_val = 0;
	unsigned int en_bit;
	unsigned int slot_bit;

	if (id == RC5T619_DS_DC1) {
		dev_err(dev, "PWRREQ1 is invalid control for rail %d\n", id);
		return -EINVAL;
	}

	en_bit = deepsleep_data[id].ds_pos_bit;
	slot_bit = en_bit + 1;
	ret = rc5t619_read(dev, deepsleep_data[id].reg_add, &sleepseq_val);
	if (ret < 0) {
		dev_err(dev, "Error in reading reg 0x%x\n",
				deepsleep_data[id].reg_add);
		return ret;
	}

	sleepseq_val &= ~(0xF << en_bit);
	sleepseq_val |= BIT(en_bit);
	sleepseq_val |= ((slots & 0x7) << slot_bit);
	ret = rc5t619_set_bits(dev, RICOH_ONOFFSEL_REG, BIT(1));
	if (ret < 0) {
		dev_err(dev, "Error in updating the 0x%02x register\n",
				RICOH_ONOFFSEL_REG);
		return ret;
	}

	ret = rc5t619_write(dev, deepsleep_data[id].reg_add, sleepseq_val);
	if (ret < 0) {
		dev_err(dev, "Error in writing reg 0x%x\n",
				deepsleep_data[id].reg_add);
		return ret;
	}

	return ret;
}

static int __rc5t619_set_ext_pwrreq2_control(struct device *dev,
	int id, int ext_pwr)
{
	int ret;

	if (id != RC5T619_DC1_SLOT) {
		dev_err(dev, "PWRREQ2 is invalid control for rail %d\n", id);
		return -EINVAL;
	}

	ret = rc5t619_set_bits(dev, RICOH_ONOFFSEL_REG, BIT(2));
	if (ret < 0)
		dev_err(dev, "Error in updating the ONOFFSEL 0x10 register\n");
	return ret;
}

int rc5t619_ext_power_req_config(struct device *dev, int ds_id,
	int ext_pwr_req, int deepsleep_slot_nr)
{
#if 0
	if ((ext_pwr_req & EXT_PWR_REQ) == EXT_PWR_REQ)
		return -EINVAL;

	if (ext_pwr_req & RC5T619_EXT_PWRREQ1_CONTROL)
		return __rc5t619_set_ext_pwrreq1_control(dev, ds_id,
				ext_pwr_req, deepsleep_slot_nr);

	if (ext_pwr_req & RC5T619_EXT_PWRREQ2_CONTROL)
		return __rc5t619_set_ext_pwrreq2_control(dev,
			ds_id, ext_pwr_req);
#endif 
	return 0;
}
EXPORT_SYMBOL(rc5t619_ext_power_req_config);

static int rc5t619_clear_ext_power_req(struct rc5t619_dev *rc5t619,
	struct rc5t619_platform_data *pdata)
{
	int ret;
	int i;
	uint8_t on_off_val = 0;

	/*  Clear ONOFFSEL register */
	if (pdata->enable_shutdown)
		on_off_val = 0x1;

	ret = rc5t619_write(rc5t619->dev, RICOH_ONOFFSEL_REG, on_off_val);
	if (ret < 0)
		dev_warn(rc5t619->dev, "Error in writing reg %d error: %d\n",
					RICOH_ONOFFSEL_REG, ret);


	/* Clear sleep sequence register */
	for (i = RC5T619_DC1_SLOT; i <= RC5T619_DC4_SLOT; ++i) {
		ret = rc5t619_write(rc5t619->dev, i, 0x0);
		if (ret < 0)
			dev_warn(rc5t619->dev,
				"Error in writing reg 0x%02x error: %d\n",
				i, ret);
	}
	return 0;
}

static bool volatile_reg(struct device *dev, unsigned int reg)
{
	/* Enable caching in interrupt registers */
	switch (reg) {
	case RC5T619_INT_EN_SYS:
	case RC5T619_INT_EN_DCDC:
	case RC5T619_INT_EN_RTC:
	case RC5T619_INT_EN_ADC1:
	case RC5T619_INT_EN_ADC2:
	case RC5T619_INT_EN_ADC3:
	case RC5T619_INT_EN_GPIO: 
	case RC5T619_INT_EN_GPIO2: 
		return false;

	case RC5T619_GPIO_MON_IOIN:
		/* This is gpio input register */
		return true;

	default:
		/* Enable caching in gpio registers */
		if ((reg >= RC5T619_GPIO_IOSEL) &&
				(reg <= RC5T619_GPIO_LED_FUNC))
			return false;

		/* Enable caching in sleep seq registers */
		if ((reg >= RC5T619_DC1_SLOT) && (reg <= RC5T619_LDO10_SLOT))
			return false;

		/* Enable caching of regulator registers */
		if ((reg >= RC5T619_DC1CTL) && (reg <= RC5T619_LDO5DAC))
			return false;
		if ((reg >= RC5T619_LDOEN1) &&
					(reg <= RC5T619_LDO10DAC))
			return false;

		break;
	}

	return true;
}

static const struct regmap_config rc5t619_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_reg = volatile_reg,
	.max_register = RC5T619_MAX_REGS,
	.num_reg_defaults_raw = RC5T619_MAX_REGS,
	.cache_type = REGCACHE_RBTREE,
};

#define RC5T619_HOST_IRQ_GPIO 32
static const struct rc5t619_platform_data rc5t619_data_init = {
	.gpio_base = BCM2708_NR_GPIOS,
	.irq_gpio = RC5T619_HOST_IRQ_GPIO,
	.irq_base = GPIO_IRQ_START,
};

static const struct regmap_irq rc5t619_pwr_irqs[] = {
	{ .mask = RC5T619_IRQ_POWER_ON, },
	{ .mask = RC5T619_IRQ_EXTIN, },

};

static const struct regmap_irq_chip rc5t619_pwr_irq_chip = {
	.name		= "rc5t619-pwr",
	.status_base	= RC5T619_INT_IR_SYS,
	.mask_base	= RC5T619_INT_EN_SYS,
	.mask_invert	= true,
	.num_regs	= 1,
	.irqs		= rc5t619_pwr_irqs,
	.num_irqs	= ARRAY_SIZE(rc5t619_pwr_irqs),
};

static const struct regmap_irq rc5t619_rtc_irqs[] = {
	{ .mask = BIT(0), },
};

static const struct regmap_irq_chip rc5t619_rtc_irq_chip = {
	.name		= "rc5t619-rtc",
	.status_base	= RC5T619_INT_IR_RTC,
	.mask_base	= RC5T619_INT_EN_RTC,
	.mask_invert	= true,
	.num_regs	= 1,
	.irqs		= rc5t619_rtc_irqs,
	.num_irqs	= ARRAY_SIZE(rc5t619_rtc_irqs),
};

static inline void gpio_wr(void __iomem *base, unsigned reg,
		u32 val)
{
	writel(val, base + reg);
}

#define GPIO_REG_OFFSET(p)	((p) / 32)
#define GPIO_REG_SHIFT(p)	((p) % 32)
#define GPPUD		0x94	/* Pin Pull-up/down Enable */
#define GPPUDCLK0	0x98	/* Pin Pull-up/down Enable Clock */

static inline void gpio_enable_pullup(unsigned pin)
{
	u32 off, bit;
	void __iomem *base = __io_address(GPIO_BASE);

	off = GPIO_REG_OFFSET(pin);
	bit = GPIO_REG_SHIFT(pin);
	gpio_wr(base, GPPUD, 0x2);
	udelay(150);
	gpio_wr(base, GPPUDCLK0+ (off*4), BIT(bit));
	udelay(150);
	gpio_wr(base, GPPUDCLK0+ (off*4), 0);
}

static int rc5t619_i2c_probe(struct i2c_client *i2c,
			      const struct i2c_device_id *id)
{
	struct rc5t619_dev *rc5t619;
	const struct rc5t619_platform_data *pdata = dev_get_platdata(&i2c->dev);
	int ret;
	uint8_t reg = 0;

	if (!pdata) {
		dev_err(&i2c->dev, "NO Platform data, using defauts\n");
		pdata = &rc5t619_data_init;
	}

	rc5t619 = devm_kzalloc(&i2c->dev, sizeof(struct rc5t619_dev), GFP_KERNEL);
	if (!rc5t619) {
		dev_err(&i2c->dev, "Memory allocation failed\n");
		return -ENOMEM;
	}

	rc5t619->dev = &i2c->dev;
	i2c_set_clientdata(i2c, rc5t619);
	rc5t619->i2c = i2c;
	gpio_enable_pullup(pdata->irq_gpio);
	rc5t619->irq = i2c->irq = gpio_to_irq(pdata->irq_gpio);

	rc5t619->regmap = devm_regmap_init_i2c(i2c, &rc5t619_regmap_config);
	if (IS_ERR(rc5t619->regmap)) {
		ret = PTR_ERR(rc5t619->regmap);
		dev_err(&i2c->dev, "regmap initialization failed: %d\n", ret);
		return ret;
	}

	ret = rc5t619_clear_ext_power_req(rc5t619, pdata);
	if (ret < 0)
		return ret;

	ret = regmap_add_irq_chip(rc5t619->regmap, rc5t619->irq,
				IRQF_ONESHOT | IRQF_SHARED | IRQF_TRIGGER_LOW, 0,
				&rc5t619_pwr_irq_chip,
				&rc5t619->irq_data_pwr);

	if (ret != 0) {
		dev_err(&i2c->dev, "failed to add power irq chip: %d\n", ret);
		goto err_irq_pwr;
	}

	ret = regmap_add_irq_chip(rc5t619->regmap, rc5t619->irq,
				IRQF_ONESHOT | IRQF_SHARED | IRQF_TRIGGER_LOW, 0,
				&rc5t619_rtc_irq_chip,
				&rc5t619->irq_data_rtc);

	if (ret != 0) {
		dev_err(&i2c->dev, "failed to add rtc irq chip: %d\n", ret);
		goto err_irq_rtc;
	}

	rc5t619_read(rc5t619->dev, 0xb7, &reg);
	rc5t619_write(rc5t619->dev, 0xb7, 0x14);

	ret = mfd_add_devices(rc5t619->dev, -1, rc5t619_subdevs,
			      ARRAY_SIZE(rc5t619_subdevs), NULL, 0, NULL);
	if (ret) {
		dev_err(&i2c->dev, "add mfd devices failed: %d\n", ret);
		goto err_add_devs;
	}

	return 0;

err_add_devs:
	mfd_remove_devices(rc5t619->dev);
	regmap_del_irq_chip(rc5t619->irq, rc5t619->irq_data_rtc);
err_irq_rtc:
	regmap_del_irq_chip(rc5t619->irq, rc5t619->irq_data_pwr);
err_irq_pwr:
	return ret;
}

static int  rc5t619_i2c_remove(struct i2c_client *i2c)
{
	struct rc5t619_dev *rc5t619 = i2c_get_clientdata(i2c);

	mfd_remove_devices(rc5t619->dev);
	regmap_del_irq_chip(rc5t619->irq, rc5t619->irq_data_rtc);
	regmap_del_irq_chip(rc5t619->irq, rc5t619->irq_data_pwr);
	return 0;
}

static const struct i2c_device_id rc5t619_i2c_id[] = {
	{.name = "rc5t619", .driver_data = 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, rc5t619_i2c_id);

static struct i2c_driver rc5t619_i2c_driver = {
	.driver = {
		   .name = "rc5t619",
		   .owner = THIS_MODULE,
		   },
	.probe = rc5t619_i2c_probe,
	.remove = rc5t619_i2c_remove,
	.id_table = rc5t619_i2c_id,
};

static int __init rc5t619_i2c_init(void)
{
	return i2c_add_driver(&rc5t619_i2c_driver);
}
subsys_initcall(rc5t619_i2c_init);

static void __exit rc5t619_i2c_exit(void)
{
	i2c_del_driver(&rc5t619_i2c_driver);
}

module_exit(rc5t619_i2c_exit);

MODULE_DESCRIPTION("RICOH rc5t619 multi-function core driver");
MODULE_AUTHOR("Hakjoo Kim <ruppi.kim@hardkernel.com>");
MODULE_LICENSE("GPL v2");
