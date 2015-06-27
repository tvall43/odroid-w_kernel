/*
 * rc5t619.h - Driver for the Maxim 77686
 *
 * Copyright (C) 2014 Hardkernel Co.,Ltd.
 * Hakjoo Kim <ruppi.kim@hardkernel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * RC5T619 has PMIC, Charger, RTC, ADC, GPIO devices.
 * The devices share the same I2C bus and included in
 * this mfd driver.
 */
#ifndef __LINUX_MFD_RC5T619_H
#define __LINUX_MFD_RC5T619_H

#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/regmap.h>

#define RC5T619_REG_DEVICE_ID 0x02
/* Maximum number of main interrupts */
#define RC5T619_MAX_INTERRUPT_MASKS	13
#define RC5T619_MAX_INTERRUPT_MASK_REGS    13
#define RC5T619_MAX_INTERRUPT_EN_REGS    12
#define RC5T619_MAX_MAIN_INTERRUPT	7
#define RC5T619_MAX_GPEDGE_REG		2

/* Power control register */
#define RC5T619_PWR_WD			0x0B
#define RC5T619_PWR_WD_COUNT		0x0C
#define RC5T619_PWR_FUNC		0x0D
#define RC5T619_PWR_SLP_CNT		0x0E
#define RC5T619_PWR_REP_CNT		0x0F
#define RC5T619_PWR_ON_TIMSET		0x10
#define RC5T619_PWR_NOE_TIMSET		0x11
#define RC5T619_PWR_IRSEL		0x15

/* Interrupt enable register */
#define RC5T619_INT_EN_SYS     0x12
#define RC5T619_INT_EN_DCDC        0x40
#define RC5T619_INT_EN_RTC     0xAE
#define RC5T619_INT_EN_ADC1        0x88
#define RC5T619_INT_EN_ADC2        0x89
#define RC5T619_INT_EN_ADC3        0x8A
#define RC5T619_INT_EN_GPIO        0x94
#define RC5T619_INT_EN_GPIO2       0x98 // dummy
#define RC5T619_INT_MSK_CHGCTR     0xBE
#define RC5T619_INT_MSK_CHGSTS1    0xBF
#define RC5T619_INT_MSK_CHGSTS2    0xC0
#define RC5T619_INT_MSK_CHGERR     0xC1
#define RC5T619_INT_MSK_CHGEXTIF   0xD1

/* Interrupt select register */
#define RC5T619_PWR_IRSEL			0x15
#define RC5T619_CHG_CTRL_DETMOD1	0xCA
#define RC5T619_CHG_CTRL_DETMOD2	0xCB
#define RC5T619_CHG_STAT_DETMOD1	0xCC
#define RC5T619_CHG_STAT_DETMOD2	0xCD
#define RC5T619_CHG_STAT_DETMOD3	0xCE


/* interrupt status registers (monitor regs)*/
#define RC5T619_INTC_INTPOL		0x9C
#define RC5T619_INTC_INTEN		0x9D
#define RC5T619_INTC_INTMON		0x9E

#define RC5T619_INT_MON_SYS		0x14
#define RC5T619_INT_MON_DCDC		0x42
#define RC5T619_INT_MON_RTC		0xAF

#define RC5T619_INT_MON_CHGCTR		0xC6
#define RC5T619_INT_MON_CHGSTS1	0xC7
#define RC5T619_INT_MON_CHGSTS2	0xC8
#define RC5T619_INT_MON_CHGERR		0xC9
#define RC5T619_INT_MON_CHGEXTIF	0xD3

/* interrupt clearing registers */
#define RC5T619_INT_IR_SYS		0x13
#define RC5T619_INT_IR_DCDC		0x41
#define RC5T619_INT_IR_RTC		0xAF
#define RC5T619_INT_IR_ADC1		0x8C
#define RC5T619_INT_IR_ADC2		0x8D
#define RC5T619_INT_IR_ADC3		0x8E
#define RC5T619_INT_IR_GPR		0x95
#define RC5T619_INT_IR_GPF		0x96
#define RC5T619_INT_IR_CHGCTR	0xC2
#define RC5T619_INT_IR_CHGSTS1	0xC3
#define RC5T619_INT_IR_CHGSTS2	0xC4
#define RC5T619_INT_IR_CHGERR	0xC5
#define RC5T619_INT_IR_CHGEXTIF	0xD2

/* GPIO register base address */
#define RC5T619_GPIO_IOSEL		0x90
#define RC5T619_GPIO_IOOUT		0x91
#define RC5T619_GPIO_GPEDGE1		0x92
#define RC5T619_GPIO_GPEDGE2		0x93
#define RC5T619_GPIO_EN_GPIR		0x94
#define RC5T619_GPIO_IR_GPR         0x95
#define RC5T619_GPIO_IR_GPF         0x96
#define RC5T619_GPIO_MON_IOIN		0x97
#define RC5T619_GPIO_LED_FUNC		0x98

#define RC5T619_REG_BANKSEL		0xFF

/* Charger Control register */
#define RC5T619_CHG_CTL1		0xB3
#define	TIMSET_REG			0xB9

/* ADC Control register */
#define RC5T619_ADC_CNT1		0x64
#define RC5T619_ADC_CNT2		0x65
#define RC5T619_ADC_CNT3		0x66
#define RC5T619_ADC_VADP_THL		0x7C
#define RC5T619_ADC_VSYS_THL		0x80

#define	RC5T619_FG_CTRL		0xE0
#define	RC5T619_PSWR			0x07

#define RC5T619_DC1_SLOT 0x16
#define RC5T619_DC2_SLOT 0x17
#define RC5T619_DC3_SLOT 0x18
#define RC5T619_DC4_SLOT 0x19
#define RC5T619_DC5_SLOT 0x1a

#define RC5T619_LDO1_SLOT 0x1b
#define RC5T619_LDO2_SLOT 0x1c
#define RC5T619_LDO3_SLOT 0x1d
#define RC5T619_LDO4_SLOT 0x1e
#define RC5T619_LDO5_SLOT 0x1f
#define RC5T619_LDO6_SLOT 0x20
#define RC5T619_LDO7_SLOT 0x21
#define RC5T619_LDO8_SLOT 0x22
#define RC5T619_LDO9_SLOT 0x23
#define RC5T619_LDO10_SLOT 0x24

#define RC5T619_DC1CTL  0x2c
#define RC5T619_DC1CTL2 0x2d
#define RC5T619_DC2CTL  0x2e
#define RC5T619_DC2CTL2 0x2f
#define RC5T619_DC3CTL  0x30
#define RC5T619_DC3CTL2 0x31
#define RC5T619_DC4CTL  0x33
#define RC5T619_DC4CTL2 0x33
#define RC5T619_DC5CTL  0x34
#define RC5T619_DC5CTL2 0x35

#define RC5T619_DC1DAC 0x36
#define RC5T619_DC2DAC 0x37
#define RC5T619_DC3DAC 0x38
#define RC5T619_DC4DAC 0x39
#define RC5T619_DC5DAC 0x3a

#define RC5T619_DC1DAC_SLP 0x3b
#define RC5T619_DC2DAC_SLP 0x3c
#define RC5T619_DC3DAC_SLP 0x3d
#define RC5T619_DC4DAC_SLP 0x3e
#define RC5T619_DC5DAC_SLP 0x3f

#define RC5T619_DCIREN  0x40
#define RC5T619_DCIRQ   0x41
#define RC5T619_DCIRMON 0x42

#define RC5T619_LDOEN1 0x44
#define RC5T619_LDOEN2 0x45
#define RC5T619_LOODIS1 0x46
#define RC5T619_LDODIS2 0x47
#define RC5T619_LDOECO 0x48

#define RC5T619_LDOECO_SLP 0x4a
#define RC5T619_LDO1DAC 0x4c
#define RC5T619_LDO2DAC 0x4d
#define RC5T619_LDO3DAC 0x4e
#define RC5T619_LDO4DAC 0x4f
#define RC5T619_LDO5DAC 0x50
#define RC5T619_LDO6DAC 0x51
#define RC5T619_LDO7DAC 0x52
#define RC5T619_LDO8DAC 0x53
#define RC5T619_LDO9DAC 0x54
#define RC5T619_LDO10DAC 0x55
#define RC5T619_LDORTC1DAC 0x56
#define RC5T619_LDORTC2DAC 0x57
#define RC5T619_LDO1DAC_SLP 0x58
#define RC5T619_LDO2DAC_SLP 0x59
#define RC5T619_LDO3DAC_SLP 0x5a
#define RC5T619_LDO4DAC_SLP 0x5b
#define RC5T619_LDO5DAC_SLP 0x5c
#define RC5T619_LDO6DAC_SLP 0x5d
#define RC5T619_LDO7DAC_SLP 0x5e
#define RC5T619_LDO8DAC_SLP 0x6f
#define RC5T619_LDO9DAC_SLP 0x60
#define RC5T619_LDO10DAC_SLP 0x61

/* RTC registers */
#define RC5T619_RTC_SEC     0xA0
#define RC5T619_RTC_MIN     0xA1
#define RC5T619_RTC_HOUR    0xA2
#define RC5T619_RTC_WDAY    0xA3
#define RC5T619_RTC_DAY     0xA4
#define RC5T619_RTC_MONTH   0xA5
#define RC5T619_RTC_YEAR    0xA6
#define RC5T619_RTC_ADJ     0xA7
#define RC5T619_RTC_AW_SEC  0xA8
#define RC5T619_RTC_AW_MIN  0xA9
#define RC5T619_RTC_AW_HOUR 0xAA
#define RC5T619_RTC_AW_DAY  0xAB
#define RC5T619_RTC_AW_MONTH    0xAC
#define RC5T619_RTC_AD_YEAR  0xAD
#define RC5T619_RTC_CTL1    0xAE
#define RC5T619_RTC_CTL2    0xAF

#define RC5T619_RTC_AY_MIN  0xA0
#define RC5T619_RTC_AY_HOUR 0xF1
#define RC5T619_RTC_AY_DAY  0xF2
#define RC5T619_RTC_AY_MONTH 0xF3
#define RC5T619_RTC_AY_YEAR 0xF4

/* RC5T619 IRQ definitions */
enum {
	RC5T619_IRQ_POWER_ON,
	RC5T619_IRQ_EXTIN,
	RC5T619_IRQ_PRE_VINDT,
	RC5T619_IRQ_PREOT,
	RC5T619_IRQ_POWER_OFF,
	RC5T619_IRQ_NOE_OFF,
	RC5T619_IRQ_WD,
	RC5T619_IRQ_CLK_STP,

	RC5T619_IRQ_DC1LIM,
	RC5T619_IRQ_DC2LIM,
	RC5T619_IRQ_DC3LIM,
	RC5T619_IRQ_DC4LIM,
	RC5T619_IRQ_DC5LIM,

	RC5T619_IRQ_ILIMLIR,
	RC5T619_IRQ_VBATLIR,
	RC5T619_IRQ_VADPLIR,
	RC5T619_IRQ_VUSBLIR,
	RC5T619_IRQ_VSYSLIR,
	RC5T619_IRQ_VTHMLIR,
	RC5T619_IRQ_AIN1LIR,
	RC5T619_IRQ_AIN0LIR,
	
	RC5T619_IRQ_ILIMHIR,
	RC5T619_IRQ_VBATHIR,
	RC5T619_IRQ_VADPHIR,
	RC5T619_IRQ_VUSBHIR,
	RC5T619_IRQ_VSYSHIR,
	RC5T619_IRQ_VTHMHIR,
	RC5T619_IRQ_AIN1HIR,
	RC5T619_IRQ_AIN0HIR,

	RC5T619_IRQ_ADC_ENDIR,

	RC5T619_IRQ_GPIO0,
	RC5T619_IRQ_GPIO1,
	RC5T619_IRQ_GPIO2,
	RC5T619_IRQ_GPIO3,
	RC5T619_IRQ_GPIO4,

	RC5T619_IRQ_CTC,
	RC5T619_IRQ_DALE,

	RC5T619_IRQ_FVADPDETSINT,
	RC5T619_IRQ_FVUSBDETSINT,
	RC5T619_IRQ_FVADPLVSINT,
	RC5T619_IRQ_FVUSBLVSINT,
	RC5T619_IRQ_FWVADPSINT,
	RC5T619_IRQ_FWVUSBSINT,

	RC5T619_IRQ_FONCHGINT,
	RC5T619_IRQ_FCHGCMPINT,
	RC5T619_IRQ_FBATOPENINT,
	RC5T619_IRQ_FSLPMODEINT,
	RC5T619_IRQ_FBTEMPJTA1INT,
	RC5T619_IRQ_FBTEMPJTA2INT,
	RC5T619_IRQ_FBTEMPJTA3INT,
	RC5T619_IRQ_FBTEMPJTA4INT,

	RC5T619_IRQ_FCURTERMINT,
	RC5T619_IRQ_FVOLTERMINT,
	RC5T619_IRQ_FICRVSINT,
	RC5T619_IRQ_FPOOR_CHGCURINT,
	RC5T619_IRQ_FOSCFDETINT1,
	RC5T619_IRQ_FOSCFDETINT2,
	RC5T619_IRQ_FOSCFDETINT3,
	RC5T619_IRQ_FOSCMDETINT,

	RC5T619_IRQ_FDIEOFFINT,
	RC5T619_IRQ_FDIEERRINT,
	RC5T619_IRQ_FBTEMPERRINT,
	RC5T619_IRQ_FVBATOVINT,
	RC5T619_IRQ_FTTIMOVINT,
	RC5T619_IRQ_FRTIMOVINT,
	RC5T619_IRQ_FVADPOVSINT,
	RC5T619_IRQ_FVUSBOVSINT,

	RC5T619_IRQ_FGCDET,
	RC5T619_IRQ_FPCDET,
	RC5T619_IRQ_FWARN_ADP,

	/* Should be last entry */
	RC5T619_NR_IRQS,
};

/* rc5t619 gpio definitions */
enum {
	RC5T619_GPIO0,
	RC5T619_GPIO1,
	RC5T619_GPIO2,
	RC5T619_GPIO3,
	RC5T619_GPIO4,

	RC5T619_NR_GPIO,
};

enum rc5t619_sleep_control_id {
	RC5T619_DS_DC1,
	RC5T619_DS_DC2,
	RC5T619_DS_DC3,
	RC5T619_DS_DC4,
	RC5T619_DS_DC5,
	RC5T619_DS_LDO1,
	RC5T619_DS_LDO2,
	RC5T619_DS_LDO3,
	RC5T619_DS_LDO4,
	RC5T619_DS_LDO5,
	RC5T619_DS_LDO6,
	RC5T619_DS_LDO7,
	RC5T619_DS_LDO8,
	RC5T619_DS_LDO9,
	RC5T619_DS_LDO10,
	RC5T619_DS_LDORTC1,
	RC5T619_DS_LDORTC2,
	RC5T619_DS_PSO0,
	RC5T619_DS_PSO1,
	RC5T619_DS_PSO2,
	RC5T619_DS_PSO3,
	RC5T619_DS_PSO4,
};


#define RC5T619_MAX_REGS 0xF8
struct rc5t619_dev {
    struct device *dev;
	struct i2c_client *i2c;
    struct regmap *regmap;

    struct regmap_irq_chip_data *irq_data_pwr;
    struct regmap_irq_chip_data *irq_data_rtc;
    int irq;
    int chip_irq;
    bool wakeup;
    int irq_base;
    int irq_gpio;
    int gpio_base;
    struct mutex irq_lock;
    unsigned long group_irq_en[RC5T619_MAX_MAIN_INTERRUPT];

    /* For main interrupt bits in INC */
    uint8_t     intc_inten_reg;

    /* For group interrupt bits and address */
    uint8_t     irq_en_reg[RC5T619_MAX_INTERRUPT_MASKS];

    /* For gpio edge */
    uint8_t     gpedge_cache[RC5T619_MAX_GPEDGE_REG];
    uint8_t     gpedge_reg[RC5T619_MAX_GPEDGE_REG];

    int         bank_num;
};

struct rc5t619_platform_data {
    int irq_base;
    int wakeup;
    int gpio_base;
    int irq;
    int irq_gpio;
    int enable_shutdown;
};

static inline int rc5t619_write(struct device *dev, uint8_t reg, uint8_t val)
{
    struct rc5t619_dev *rc5t619 = dev_get_drvdata(dev);
    return regmap_write(rc5t619->regmap, reg, val);
}

static inline int rc5t619_read(struct device *dev, uint8_t reg, uint8_t *val)
{
    struct rc5t619_dev *rc5t619 = dev_get_drvdata(dev);
    unsigned int ival;
    int ret;

    ret = regmap_read(rc5t619->regmap, reg, &ival);
    if (!ret)
        *val = (uint8_t)ival;
    return ret;
}

static inline int rc5t619_set_bits(struct device *dev, unsigned int reg,
    unsigned int bit_mask)
{
    struct rc5t619_dev *rc5t619 = dev_get_drvdata(dev);
    return regmap_update_bits(rc5t619->regmap, reg, bit_mask, bit_mask);
}

static inline int rc5t619_clear_bits(struct device *dev, unsigned int reg,
    unsigned int bit_mask)
{
    struct rc5t619_dev *rc5t619 = dev_get_drvdata(dev);
    return regmap_update_bits(rc5t619->regmap, reg, bit_mask, 0); 
}

static inline int rc5t619_update(struct device *dev, unsigned int reg,
    unsigned int val, unsigned int mask)
{
    struct rc5t619_dev *rc5t619 = dev_get_drvdata(dev);
    return regmap_update_bits(rc5t619->regmap, reg, mask, val);
}

#endif /* __LINUX_MFD_RC5T619_H */
