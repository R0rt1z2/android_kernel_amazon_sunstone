// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Samuel Hsieh <samuel.hsieh@mediatek.com>
 */

#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/mfd/mt6359/registers.h>
#include <linux/mfd/mt6397/core.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#include "mtk_battery_oc_throttling.h"

#define MT6375_FGADC_CUR_CON1	0x2E9
#define MT6375_FGADC_CUR_CON2	0x2EB
#define MT6375_FGADC_ANA_ELR4	0x263
#define FG_GAINERR_SEL_MASK	GENMASK(1, 0)

/* Customize the setting in dts node */
#define DEF_BAT_OC_THD_H	5800
#define DEF_BAT_OC_THD_L	6300

#define UNIT_TRANS_10		10
#define CURRENT_CONVERT_RATIO	95
#define OCCB_MAX_NUM		16

/* Get R_FG_VALUE/CAR_TUNE_VALUE from gauge dts node */
#define	MT6357_DEFAULT_RFG		(100)
#define	MT6357_UNIT_FGCURRENT		(314331)

#define	MT6358_DEFAULT_RFG		(100)
#define	MT6358_UNIT_FGCURRENT		(381470)

#define	MT6359_DEFAULT_RFG		(50)
#define	MT6359_UNIT_FGCURRENT		(610352)

#define	MT6375_UNIT_FGCURRENT		(610352)

struct reg_t {
	unsigned int addr;
	unsigned int mask;
	size_t size;
};

struct battery_oc_data_t {
	const char *regmap_source;
	const char *gauge_node_name;
	struct reg_t fg_cur_hth;
	struct reg_t fg_cur_lth;
	bool cust_rfg;
	struct reg_t reg_default_rfg;
};

struct battery_oc_data_t mt6359_battery_oc_data = {
	.regmap_source = "parent_drvdata",
	.gauge_node_name = "mtk_gauge",
	.fg_cur_hth = {MT6359_FGADC_CUR_CON2, 0xFFFF, 1},
	.fg_cur_lth = {MT6359_FGADC_CUR_CON1, 0xFFFF, 1},
	.cust_rfg = false,
};

struct battery_oc_data_t mt6375_battery_oc_data = {
	.regmap_source = "dev_get_regmap",
	.gauge_node_name = "mtk_gauge",
	.fg_cur_hth = {MT6375_FGADC_CUR_CON2, 0xFFFF, 2},
	.fg_cur_lth = {MT6375_FGADC_CUR_CON1, 0xFFFF, 2},
	.cust_rfg = true,
	.reg_default_rfg = {MT6375_FGADC_ANA_ELR4, FG_GAINERR_SEL_MASK, 1},
};

struct battery_oc_priv {
	struct regmap *regmap;
	int oc_level;
	unsigned int oc_thd_h;
	unsigned int oc_thd_l;
	int fg_cur_h_irq;
	int fg_cur_l_irq;
	int r_fg_value;
	int default_rfg;
	int car_tune_value;
	int unit_fg_cur;
	const struct battery_oc_data_t *ocdata;
};

static int g_battery_oc_stop;

struct battery_oc_callback_table {
	void (*occb)(enum BATTERY_OC_LEVEL_TAG);
};

static struct battery_oc_callback_table occb_tb[OCCB_MAX_NUM] = { {0} };

static int __regmap_update_bits(struct regmap *regmap, const struct reg_t *reg,
				unsigned int val)
{
	if (reg->size == 1)
		return regmap_update_bits(regmap, reg->addr, reg->mask, val);
	/*
	 * here we assume those register addresses are continuous and
	 * there is one and only one function in them.
	 * please take care of the endian if it is necessary.
	 * this is not a good assumption but we do this here for compatiblity.
	 * please abstract the register control if there is a chance to refactor
	 * this file.
	 */
	val &= reg->mask;
	return regmap_bulk_write(regmap, reg->addr, &val, reg->size);
}

static int __regmap_read(struct regmap *regmap, const struct reg_t *reg,
			 unsigned int *val)
{
	if (reg->size == 1)
		return regmap_read(regmap, reg->addr, val);
	return regmap_bulk_read(regmap, reg->addr, val, reg->size);
}

void register_battery_oc_notify(battery_oc_callback oc_cb,
				enum BATTERY_OC_PRIO_TAG prio_val)
{
	if (prio_val >= OCCB_MAX_NUM || prio_val < 0) {
		pr_info("[%s] prio_val=%d, out of boundary\n",
			__func__, prio_val);
		return;
	}
	occb_tb[prio_val].occb = oc_cb;
	pr_info("[%s] prio_val=%d\n", __func__, prio_val);
}
EXPORT_SYMBOL(register_battery_oc_notify);

void exec_battery_oc_callback(enum BATTERY_OC_LEVEL_TAG battery_oc_level)
{
	int i;

	if (g_battery_oc_stop == 1) {
		pr_info("[%s] g_battery_oc_stop=%d\n"
			, __func__, g_battery_oc_stop);
	} else {
		for (i = 0; i < OCCB_MAX_NUM; i++) {
			if (occb_tb[i].occb)
				occb_tb[i].occb(battery_oc_level);
		}
		pr_info("[%s] battery_oc_level=%d\n", __func__, battery_oc_level);
	}
}

static ssize_t battery_oc_protect_ut_show(
		struct device *pdev, struct device_attribute *attr,
		char *buf)
{
	struct battery_oc_priv *priv = dev_get_drvdata(pdev);

	pr_debug("[%s] g_battery_oc_level=%d\n",
		__func__, priv->oc_level);
	return sprintf(buf, "%u\n", priv->oc_level);
}

static ssize_t battery_oc_protect_ut_store(
		struct device *pdev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	unsigned int val = 0;
	char cmd[21];

	dev_info(pdev, "[%s]\n", __func__);

	if (sscanf(buf, "%20s %u\n", cmd, &val) != 2) {
		dev_info(pdev, "parameter number not correct\n");
		return -EINVAL;
	}

	if (strncmp(cmd, "Utest", 5))
		return -EINVAL;

	if (val < BATTERY_OC_LEVEL_NUM) {
		dev_info(pdev, "[%s] your input is %d\n", __func__, val);
		exec_battery_oc_callback(val);
	} else {
		dev_info(pdev, "[%s] wrong number (%d)\n", __func__, val);
	}

	return size;
}

static DEVICE_ATTR_RW(battery_oc_protect_ut);

static ssize_t battery_oc_protect_stop_show(
		struct device *pdev, struct device_attribute *attr,
		char *buf)
{
	pr_debug("[%s] g_battery_oc_stop=%d\n",
		__func__, g_battery_oc_stop);
	return sprintf(buf, "%u\n", g_battery_oc_stop);
}

static ssize_t battery_oc_protect_stop_store(
		struct device *pdev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	unsigned int val = 0;
	char cmd[21];

	dev_info(pdev, "[%s]\n", __func__);

	if (sscanf(buf, "%20s %u\n", cmd, &val) != 2) {
		dev_info(pdev, "parameter number not correct\n");
	}

	if (strncmp(cmd, "stop", 4))
		return -EINVAL;

	if ((val != 0) && (val != 1))
		val = 0;

	g_battery_oc_stop = val;
	dev_info(pdev, "g_battery_oc_stop=%d\n",
		 g_battery_oc_stop);

	return size;
}

static DEVICE_ATTR_RW(battery_oc_protect_stop);

static ssize_t battery_oc_protect_level_show(
		struct device *pdev, struct device_attribute *attr,
		char *buf)
{
	struct battery_oc_priv *priv = dev_get_drvdata(pdev);

	pr_info("[%s] g_battery_oc_level=%d\n",
		__func__, priv->oc_level);
	return sprintf(buf, "%u\n", priv->oc_level);
}

static ssize_t battery_oc_protect_level_store(
		struct device *pdev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct battery_oc_priv *priv = dev_get_drvdata(pdev);

	pr_info("[%s] g_battery_oc_level = %d\n",
		__func__, priv->oc_level);

	return size;
}

static DEVICE_ATTR_RW(battery_oc_protect_level);

/*
 * 65535 - (I_mA * 1000 * r_fg_value / DEFAULT_RFG * 1000000 / car_tune_value
 * / UNIT_FGCURRENT * CURRENT_CONVERT_RATIO / 100)
 */
static unsigned int to_fg_code(struct battery_oc_priv *priv, u64 cur_mA)
{
	cur_mA = div_u64(cur_mA * 1000 * priv->r_fg_value, priv->default_rfg);
	cur_mA = div_u64(cur_mA * 1000000, priv->car_tune_value);
	cur_mA = div_u64(cur_mA, priv->unit_fg_cur);
	cur_mA = div_u64(cur_mA * CURRENT_CONVERT_RATIO, 100);

	/* 2's complement */
	return (0xFFFF - cur_mA);
}

static irqreturn_t fg_cur_h_int_handler(int irq, void *data)
{
	struct battery_oc_priv *priv = data;

	priv->oc_level = BATTERY_OC_LEVEL_0;
	exec_battery_oc_callback(priv->oc_level);
	disable_irq_nosync(priv->fg_cur_h_irq);
	enable_irq(priv->fg_cur_l_irq);

	return IRQ_HANDLED;
}

static irqreturn_t fg_cur_l_int_handler(int irq, void *data)
{
	struct battery_oc_priv *priv = data;

	priv->oc_level = BATTERY_OC_LEVEL_1;
	exec_battery_oc_callback(priv->oc_level);
	disable_irq_nosync(priv->fg_cur_l_irq);
	enable_irq(priv->fg_cur_h_irq);

	return IRQ_HANDLED;
}

static int battery_oc_parse_dt(struct platform_device *pdev)
{
	struct mt6397_chip *pmic;
	struct battery_oc_priv *priv = dev_get_drvdata(&pdev->dev);
	struct device_node *np;
	int ret = 0;
	const int r_fg_val[] = { 50, 20, 10, 5 };
	u32 regval = 0;

	/* Get R_FG_VALUE/CAR_TUNE_VALUE from gauge dts node */
	np = of_find_node_by_name(pdev->dev.parent->of_node,
				  priv->ocdata->gauge_node_name);
	if (!np) {
		dev_notice(&pdev->dev, "get mtk_gauge node fail\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "R_FG_VALUE", &priv->r_fg_value);
	if (ret) {
		dev_notice(&pdev->dev, "get R_FG_VALUE fail\n");
		return -EINVAL;
	}
	priv->r_fg_value *= UNIT_TRANS_10;

	ret = of_property_read_u32(np, "CAR_TUNE_VALUE", &priv->car_tune_value);
	if (ret) {
		dev_notice(&pdev->dev, "get CAR_TUNE_VALUE fail\n");
		return -EINVAL;
	}
	priv->car_tune_value *= UNIT_TRANS_10;

	/* Get oc_thd_h/oc_thd_l value from dts node */
	np = of_find_node_by_name(pdev->dev.parent->of_node,
				  "mtk_battery_oc_throttling");
	if (!np) {
		dev_notice(&pdev->dev, "get mtk battery oc node fail\n");
		return -EINVAL;
	}
	ret = of_property_read_u32(np, "oc-thd-h", &priv->oc_thd_h);
	if (ret)
		priv->oc_thd_h = DEF_BAT_OC_THD_H;

	ret = of_property_read_u32(np, "oc-thd-l", &priv->oc_thd_l);
	if (ret)
		priv->oc_thd_l = DEF_BAT_OC_THD_L;

	/* Get DEFAULT_RFG/UNIT_FGCURRENT from pre-defined MACRO */
	if (priv->ocdata->cust_rfg) {
		__regmap_read(priv->regmap, &priv->ocdata->reg_default_rfg, &regval);
		regval &= priv->ocdata->reg_default_rfg.mask;
		priv->default_rfg = r_fg_val[regval];
		priv->unit_fg_cur = MT6375_UNIT_FGCURRENT;
	} else {
		pmic = dev_get_drvdata(pdev->dev.parent);
		switch (pmic->chip_id) {
		case MT6359_CHIP_ID:
			priv->default_rfg = MT6359_DEFAULT_RFG;
			priv->unit_fg_cur = MT6359_UNIT_FGCURRENT;
			break;

		default:
			dev_info(&pdev->dev, "unsupported chip: 0x%x\n", pmic->chip_id);
			return -EINVAL;
		}
	}
	dev_info(&pdev->dev, "r_fg=%d car_tune=%d DEFAULT_RFG=%d UNIT_FGCURRENT=%d\n"
		 , priv->r_fg_value, priv->car_tune_value
		 , priv->default_rfg, priv->unit_fg_cur);
	return 0;
}

static int battery_oc_throttling_probe(struct platform_device *pdev)
{
	int ret;
	struct battery_oc_priv *priv;
	struct mt6397_chip *chip;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	dev_set_drvdata(&pdev->dev, priv);
	priv->ocdata = of_device_get_match_data(&pdev->dev);
	if (!strcmp(priv->ocdata->regmap_source, "parent_drvdata")) {
		chip = dev_get_drvdata(pdev->dev.parent);
		priv->regmap = chip->regmap;
	} else
		priv->regmap = dev_get_regmap(pdev->dev.parent, NULL);

	if (!priv->regmap) {
		dev_notice(&pdev->dev, "failed to get regmap\n");
		return -ENODEV;
	}

	/* set Maximum threshold to avoid irq being triggered at init */
	__regmap_update_bits(priv->regmap, &priv->ocdata->fg_cur_hth, 0x7FFF);
	__regmap_update_bits(priv->regmap, &priv->ocdata->fg_cur_lth, 0x8000);
	priv->fg_cur_h_irq = platform_get_irq_byname(pdev, "fg_cur_h");
	if (priv->fg_cur_h_irq < 0) {
		dev_notice(&pdev->dev, "failed to get fg_cur_h irq, ret=%d\n",
			   priv->fg_cur_h_irq);
		return priv->fg_cur_h_irq;
	}
	priv->fg_cur_l_irq = platform_get_irq_byname(pdev, "fg_cur_l");
	if (priv->fg_cur_l_irq < 0) {
		dev_notice(&pdev->dev, "failed to get fg_cur_l irq, ret=%d\n",
			   priv->fg_cur_l_irq);
		return priv->fg_cur_l_irq;
	}
	ret = devm_request_threaded_irq(&pdev->dev, priv->fg_cur_h_irq, NULL,
					fg_cur_h_int_handler, IRQF_ONESHOT,
					"fg_cur_h", priv);
	if (ret < 0)
		dev_notice(&pdev->dev, "request fg_cur_h irq fail\n");
	ret = devm_request_threaded_irq(&pdev->dev, priv->fg_cur_l_irq, NULL,
					fg_cur_l_int_handler, IRQF_ONESHOT,
					"fg_cur_l", priv);
	if (ret < 0)
		dev_notice(&pdev->dev, "request fg_cur_l irq fail\n");
	disable_irq_nosync(priv->fg_cur_h_irq);

	ret = battery_oc_parse_dt(pdev);
	if (ret < 0) {
		dev_notice(&pdev->dev, "bat_oc parse dt fail, ret=%d\n", ret);
		return ret;
	}

	__regmap_update_bits(priv->regmap, &priv->ocdata->fg_cur_hth,
			     to_fg_code(priv, priv->oc_thd_h));
	__regmap_update_bits(priv->regmap, &priv->ocdata->fg_cur_lth,
			     to_fg_code(priv, priv->oc_thd_l));
	dev_info(&pdev->dev, "%dmA(0x%x), %dmA(0x%x) Done\n",
		 priv->oc_thd_h, to_fg_code(priv, priv->oc_thd_h),
		 priv->oc_thd_l, to_fg_code(priv, priv->oc_thd_l));

	ret = device_create_file(&(pdev->dev),
		&dev_attr_battery_oc_protect_ut);
	ret |= device_create_file(&(pdev->dev),
		&dev_attr_battery_oc_protect_stop);
	ret |= device_create_file(&(pdev->dev),
		&dev_attr_battery_oc_protect_level);
	if (ret)
		dev_notice(&pdev->dev, "create file error ret=%d\n", ret);

	return ret;
}

static int __maybe_unused battery_oc_throttling_suspend(struct device *d)
{
	struct battery_oc_priv *priv = dev_get_drvdata(d);

	if (priv->oc_level == BATTERY_OC_LEVEL_0)
		disable_irq_nosync(priv->fg_cur_l_irq);
	else
		disable_irq_nosync(priv->fg_cur_h_irq);
	return 0;
}

static int __maybe_unused battery_oc_throttling_resume(struct device *d)
{
	struct battery_oc_priv *priv = dev_get_drvdata(d);

	if (priv->oc_level == BATTERY_OC_LEVEL_0)
		enable_irq(priv->fg_cur_l_irq);
	else
		enable_irq(priv->fg_cur_h_irq);
	return 0;
}

static SIMPLE_DEV_PM_OPS(battery_oc_throttling_pm_ops,
			 battery_oc_throttling_suspend,
			 battery_oc_throttling_resume);

static const struct of_device_id battery_oc_throttling_of_match[] = {
	{
		.compatible = "mediatek,mt6359-battery_oc_throttling",
		.data = &mt6359_battery_oc_data,
	}, {
		.compatible = "mediatek,mt6375-battery_oc_throttling",
		.data = &mt6375_battery_oc_data,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, battery_oc_throttling_of_match);

static struct platform_driver battery_oc_throttling_driver = {
	.driver = {
		.name = "mtk_battery_oc_throttling",
		.of_match_table = battery_oc_throttling_of_match,
		.pm = &battery_oc_throttling_pm_ops,
	},
	.probe = battery_oc_throttling_probe,
};
module_platform_driver(battery_oc_throttling_driver);

MODULE_AUTHOR("Jeter Chen");
MODULE_DESCRIPTION("MTK battery over current throttling driver");
MODULE_LICENSE("GPL");
