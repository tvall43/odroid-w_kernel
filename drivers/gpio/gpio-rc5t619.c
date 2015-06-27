/*
 * GPIO driver for RICOH619 power management chip.
 *
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 * Author: Laxman dewangan <ldewangan@nvidia.com>
 *
 * Based on code
 *	Copyright (C) 2011 RICOH COMPANY,LTD
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
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/mfd/rc5t619.h>

struct rc5t619_gpio {
	struct gpio_chip gpio_chip;
	struct rc5t619_dev *rc5t619;
};

static inline struct rc5t619_gpio *to_rc5t619_gpio(struct gpio_chip *chip)
{
	return container_of(chip, struct rc5t619_gpio, gpio_chip);
}

static int rc5t619_gpio_get(struct gpio_chip *gc, unsigned int offset)
{
	struct rc5t619_gpio *rc5t619_gpio = to_rc5t619_gpio(gc);
	struct device *parent = rc5t619_gpio->rc5t619->dev;
	uint8_t val = 0;
	int ret;

	ret = rc5t619_read(parent, RC5T619_GPIO_MON_IOIN, &val);
	if (ret < 0)
		return ret;

	return !!(val & BIT(offset));
}

static void rc5t619_gpio_set(struct gpio_chip *gc, unsigned int offset, int val)
{
	struct rc5t619_gpio *rc5t619_gpio = to_rc5t619_gpio(gc);
	struct device *parent = rc5t619_gpio->rc5t619->dev;
	if (val)
		rc5t619_set_bits(parent, RC5T619_GPIO_IOOUT, BIT(offset));
	else
		rc5t619_clear_bits(parent, RC5T619_GPIO_IOOUT, BIT(offset));
}

static int rc5t619_gpio_dir_input(struct gpio_chip *gc, unsigned int offset)
{
	struct rc5t619_gpio *rc5t619_gpio = to_rc5t619_gpio(gc);
	struct device *parent = rc5t619_gpio->rc5t619->dev;
	int ret;

	ret = rc5t619_clear_bits(parent, RC5T619_GPIO_IOSEL, BIT(offset));
	if (ret < 0)
		return ret;
}

static int rc5t619_gpio_dir_output(struct gpio_chip *gc, unsigned offset,
			int value)
{
	struct rc5t619_gpio *rc5t619_gpio = to_rc5t619_gpio(gc);
	struct device *parent = rc5t619_gpio->rc5t619->dev;
	int ret;

	rc5t619_gpio_set(gc, offset, value);
	ret = rc5t619_set_bits(parent, RC5T619_GPIO_IOSEL, BIT(offset));
	if (ret < 0)
		return ret;
}

static int rc5t619_gpio_to_irq(struct gpio_chip *gc, unsigned offset)
{
	struct rc5t619_gpio *rc5t619_gpio = to_rc5t619_gpio(gc);

	if ((offset >= 0) && (offset < 8))
		return rc5t619_gpio->rc5t619->irq_base +
				RC5T619_IRQ_GPIO0 + offset;
	return -EINVAL;
}

static void rc5t619_gpio_free(struct gpio_chip *gc, unsigned offset)
{
}

static int rc5t619_gpio_probe(struct platform_device *pdev)
{
	struct rc5t619_dev *rc5t619 = dev_get_drvdata(pdev->dev.parent);
	struct rc5t619_platform_data *pdata = dev_get_platdata(rc5t619->dev);
	struct rc5t619_gpio *rc5t619_gpio;

	rc5t619_gpio = devm_kzalloc(&pdev->dev, sizeof(*rc5t619_gpio),
					GFP_KERNEL);
	if (!rc5t619_gpio) {
		dev_warn(&pdev->dev, "Mem allocation for rc5t619_gpio failed");
		return -ENOMEM;
	}

	rc5t619_gpio->gpio_chip.label = "gpio-rc5t619",
	rc5t619_gpio->gpio_chip.owner = THIS_MODULE,
	rc5t619_gpio->gpio_chip.free = rc5t619_gpio_free,
	rc5t619_gpio->gpio_chip.direction_input = rc5t619_gpio_dir_input,
	rc5t619_gpio->gpio_chip.direction_output = rc5t619_gpio_dir_output,
	rc5t619_gpio->gpio_chip.set = rc5t619_gpio_set,
	rc5t619_gpio->gpio_chip.get = rc5t619_gpio_get,
	rc5t619_gpio->gpio_chip.to_irq = rc5t619_gpio_to_irq,
	rc5t619_gpio->gpio_chip.ngpio = RC5T619_MAX_GPIO,
	rc5t619_gpio->gpio_chip.can_sleep = 1,
	rc5t619_gpio->gpio_chip.dev = &pdev->dev;
	rc5t619_gpio->gpio_chip.base = -1;
	rc5t619_gpio->rc5t619 = rc5t619;

	if (pdata && pdata->gpio_base)
		rc5t619_gpio->gpio_chip.base = pdata->gpio_base;

	platform_set_drvdata(pdev, rc5t619_gpio);

	return gpiochip_add(&rc5t619_gpio->gpio_chip);
}

static int rc5t619_gpio_remove(struct platform_device *pdev)
{
	struct rc5t619_gpio *rc5t619_gpio = platform_get_drvdata(pdev);

	return gpiochip_remove(&rc5t619_gpio->gpio_chip);
}

static struct platform_driver rc5t619_gpio_driver = {
	.driver = {
		.name    = "rc5t619-gpio",
		.owner   = THIS_MODULE,
	},
	.probe		= rc5t619_gpio_probe,
	.remove		= rc5t619_gpio_remove,
};

static int __init rc5t619_gpio_init(void)
{
	return platform_driver_register(&rc5t619_gpio_driver);
}
subsys_initcall(rc5t619_gpio_init);

static void __exit rc5t619_gpio_exit(void)
{
	platform_driver_unregister(&rc5t619_gpio_driver);
}
module_exit(rc5t619_gpio_exit);

MODULE_AUTHOR("Laxman Dewangan <ldewangan@nvidia.com>");
MODULE_DESCRIPTION("GPIO interface for RC5T619");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:rc5t619-gpio");
