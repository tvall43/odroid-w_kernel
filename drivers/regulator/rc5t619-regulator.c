/*
 * Regulator driver for RICOH RC5T619 power management chip.
 *
 * Copyright (c) 2011-2012, NVIDIA CORPORATION.  All rights reserved.
 * Author: Laxman dewangan <ldewangan@nvidia.com>
 *
 * based on code
 *      Copyright (C) 2011 RICOH COMPANY,LTD
 *
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/gpio.h>
#include <linux/mfd/rc5t619.h>

struct rc5t619_regulator_info {
	int			deepsleep_id;

	/* Regulator register address.*/
	uint8_t			reg_disc_reg;
	uint8_t			disc_bit;
	uint8_t			deepsleep_reg;

	/* Regulator specific turn-on delay  and voltage settling time*/
	int			enable_uv_per_us;

	/* Used by regulator core */
	struct regulator_desc	desc;
};

struct rc5t619_regulator {
	struct rc5t619_regulator_info *reg_info;
	struct regulator_dev	*rdev;
};

static int rc5t619_regulator_enable_time(struct regulator_dev *rdev)
{
	struct rc5t619_regulator *reg = rdev_get_drvdata(rdev);
	int vsel = regulator_get_voltage_sel_regmap(rdev);
	int curr_uV = regulator_list_voltage_linear(rdev, vsel);

	return DIV_ROUND_UP(curr_uV, reg->reg_info->enable_uv_per_us);
}

static struct regulator_ops rc5t619_ops = {
	.is_enabled		= regulator_is_enabled_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.enable_time		= rc5t619_regulator_enable_time,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.set_voltage_time_sel	= regulator_set_voltage_time_sel,
};

#define RC5T619_REG(_id, _en_reg, _en_bit, _disc_reg, _disc_bit, \
		_vout_mask, _min_mv, _max_mv, _step_uV, _enable_mv) \
{								\
	.reg_disc_reg	= RC5T619_##_disc_reg,		\
	.disc_bit	= _disc_bit,				\
	.deepsleep_reg	= RC5T619_##_id##DAC_SLP,		\
	.enable_uv_per_us = _enable_mv * 1000,			\
	.deepsleep_id	= RC5T619_DS_##_id,			\
	.desc = {						\
		.name = "rc5t619-regulator-"#_id,		\
		.id = RC5T619_REGULATOR_##_id,			\
		.n_voltages = (_max_mv - _min_mv) * 1000 / _step_uV + 1, \
		.ops = &rc5t619_ops,				\
		.type = REGULATOR_VOLTAGE,			\
		.owner = THIS_MODULE,				\
		.vsel_reg = RC5T619_##_id##DAC,		\
		.vsel_mask = _vout_mask,			\
		.enable_reg = RC5T619_##_en_reg,		\
		.enable_mask = BIT(_en_bit),			\
		.min_uV	= _min_mv * 1000,			\
		.uV_step = _step_uV,				\
		.ramp_delay = 40 * 1000,			\
	},							\
}

static struct rc5t619_regulator_info rc5t619_reg_info[RC5T619_REGULATOR_MAX] = {
	RC5T619_REG(DC1, DC1CTL, 0, DC1CTL, 1, 0xFF, 600, 3500, 12500, 14),
	RC5T619_REG(DC2, DC2CTL, 0, DC2CTL, 1, 0xFF, 600, 3500, 12500, 14),
	RC5T619_REG(DC3, DC3CTL, 0, DC3CTL, 1, 0xFF, 600, 3500, 12500, 14),
	RC5T619_REG(DC4, DC4CTL, 0, DC4CTL, 1, 0xFF, 600, 3500, 12500, 14),
	RC5T619_REG(DC5, DC5CTL, 0, DC5CTL, 1, 0xFF, 600, 3500, 12500, 14),

	RC5T619_REG(LDO1,  LDOEN1, 0, LDODIS1, 0, 0x7F, 900, 3500, 25000, 160),
	RC5T619_REG(LDO2,  LDOEN1, 1, LDODIS1, 1, 0x7F, 900, 3500, 25000, 160),
	RC5T619_REG(LDO3,  LDOEN1, 2, LDODIS1, 2, 0x7F, 900, 3500, 25000, 160),
	RC5T619_REG(LDO4,  LDOEN1, 3, LDODIS1, 3, 0x7F, 900, 3500, 25000, 160),
	RC5T619_REG(LDO5,  LDOEN1, 4, LDODIS1, 4, 0x7F, 600, 3500, 25000, 160),
	RC5T619_REG(LDO6,  LDOEN1, 5, LDODIS1, 5, 0x7F, 600, 3500, 25000, 160),
	RC5T619_REG(LDO7,  LDOEN1, 6, LDODIS1, 6, 0x7F, 900, 3500, 25000, 160),
	RC5T619_REG(LDO8,  LDOEN1, 7, LDODIS1, 7, 0x7F, 900, 3500, 25000, 160),
	RC5T619_REG(LDO9,  LDOEN2, 0, LDODIS2, 0, 0x7F, 900, 3500, 25000, 160),
	RC5T619_REG(LDO10, LDOEN2, 1, LDODIS2, 1, 0x7F, 900, 3500, 25000, 160),

//	RC5T619_REG(LDORTC1, LDOEN2, 4, LDODIS2, 7, 0x7F, 1700, 3500, 25000, 160),
//	RC5T619_REG(LDORTC2, LDOEN2, 5, LDODIS2, 8, 0x7F,  900, 3500, 25000, 160),
};

static int rc5t619_regulator_probe(struct platform_device *pdev)
{
	struct rc5t619_dev *rc5t619 = dev_get_drvdata(pdev->dev.parent);
	struct rc5t619_platform_data *pdata = dev_get_platdata(rc5t619->dev);
	struct regulator_init_data *reg_data;
	struct regulator_config config = { };
	struct rc5t619_regulator *reg = NULL;
	struct rc5t619_regulator *regs;
	struct regulator_dev *rdev;
	struct rc5t619_regulator_info *ri;
	int ret;
	int id, i;

	if (!pdata) {
		dev_err(&pdev->dev, "No platform data, exiting...\n");
		return -ENODEV;
	}

	regs = devm_kzalloc(&pdev->dev, RC5T619_REGULATOR_MAX *
			sizeof(struct rc5t619_regulator), GFP_KERNEL);
	if (!regs) {
		dev_err(&pdev->dev, "Memory allocation failed exiting..\n");
		return -ENOMEM;
	}


	for (i = 0; i < pdata->num_regulators; i++) {
		id = pdata->regulators[i].id;
		reg_data = pdata->regulators[i].initdata;

		/* No need to register if there is no regulator data */
		if (!reg_data)
			continue;

		reg = &regs[id];
		ri = &rc5t619_reg_info[id];
		reg->reg_info = ri;

//		if (ri->deepsleep_id == RC5T619_DS_NONE)
//			goto skip_ext_pwr_config;
//
//		ret = rc5t619_ext_power_req_config(rc5t619->dev,
//				ri->deepsleep_id,
//				pdata->regulator_ext_pwr_control[id],
//				pdata->regulator_deepsleep_slot[id]);
		/*
		 * Configuring external control is not a major issue,
		 * just give warning.
		 */
//		if (ret < 0)
//			dev_warn(&pdev->dev,
//				"Failed to configure ext control %d\n", id);

//skip_ext_pwr_config:

		config.dev = &pdev->dev;
		config.regmap = rc5t619->regmap;
		config.init_data = pdata->regulators[i].initdata;
		config.driver_data = reg;

		rdev = regulator_register(&ri->desc, &config);
		if (IS_ERR(rdev)) {
			dev_err(&pdev->dev, "Failed to register regulator %s\n",
						ri->desc.name);
			ret = PTR_ERR(rdev);
			goto clean_exit;
		}
		reg->rdev = rdev;
	}
	platform_set_drvdata(pdev, regs);
	return 0;

clean_exit:
	while (--id >= 0)
		regulator_unregister(regs[id].rdev);

	return ret;
}

static int rc5t619_regulator_remove(struct platform_device *pdev)
{
	struct rc5t619_regulator *regs = platform_get_drvdata(pdev);
	int id;

	for (id = 0; id < RC5T619_REGULATOR_MAX; ++id)
		regulator_unregister(regs[id].rdev);
	return 0;
}

static struct platform_driver rc5t619_regulator_driver = {
	.driver	= {
		.name	= "rc5t619-regulator",
		.owner	= THIS_MODULE,
	},
	.probe		= rc5t619_regulator_probe,
	.remove		= rc5t619_regulator_remove,
};

static int __init rc5t619_regulator_init(void)
{
	return platform_driver_register(&rc5t619_regulator_driver);
}
subsys_initcall(rc5t619_regulator_init);

static void __exit rc5t619_regulator_exit(void)
{
	platform_driver_unregister(&rc5t619_regulator_driver);
}
module_exit(rc5t619_regulator_exit);

MODULE_AUTHOR("Laxman Dewangan <ldewangan@nvidia.com>");
MODULE_DESCRIPTION("RC5T619 regulator driver");
MODULE_ALIAS("platform:rc5t619-regulator");
MODULE_LICENSE("GPL v2");
