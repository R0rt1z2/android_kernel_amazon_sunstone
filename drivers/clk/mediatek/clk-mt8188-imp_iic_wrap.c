// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2021 MediaTek Inc.
// Author: Chun-Jie Chen <chun-jie.chen@mediatek.com>

#include "clk-gate.h"
#include "clk-mtk.h"
#include <dt-bindings/clock/mt8188-clk.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/platform_device.h>

static const struct mtk_gate_regs imp_iic_wrap_cg_regs = {
	.set_ofs = 0xe08,
	.clr_ofs = 0xe04,
	.sta_ofs = 0xe00,
};

#define GATE_IMP_IIC_WRAP(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &imp_iic_wrap_cg_regs, _shift, &mtk_clk_gate_ops_setclr)

static const struct mtk_gate imp_iic_wrap_c_clks[] = {
	GATE_IMP_IIC_WRAP(CLK_IMP_IIC_WRAP_C_AP_CLOCK_I2C0,
			  "imp_iic_wrap_c_ap_clock_i2c0", "i2c_sel", 0),
	GATE_IMP_IIC_WRAP(CLK_IMP_IIC_WRAP_C_AP_CLOCK_I2C2,
			  "imp_iic_wrap_c_ap_clock_i2c2", "i2c_sel", 1),
	GATE_IMP_IIC_WRAP(CLK_IMP_IIC_WRAP_C_AP_CLOCK_I2C3,
			  "imp_iic_wrap_c_ap_clock_i2c3", "i2c_sel", 2),
};

static const struct mtk_gate imp_iic_wrap_w_clks[] = {
	GATE_IMP_IIC_WRAP(CLK_IMP_IIC_WRAP_W_AP_CLOCK_I2C1,
			  "imp_iic_wrap_w_ap_clock_i2c1", "i2c_sel", 0),
	GATE_IMP_IIC_WRAP(CLK_IMP_IIC_WRAP_W_AP_CLOCK_I2C4,
			  "imp_iic_wrap_w_ap_clock_i2c4", "i2c_sel", 1),
};

static const struct mtk_gate imp_iic_wrap_en_clks[] = {
	GATE_IMP_IIC_WRAP(CLK_IMP_IIC_WRAP_EN_AP_CLOCK_I2C5,
			  "imp_iic_wrap_en_ap_clock_i2c5", "i2c_sel", 0),
	GATE_IMP_IIC_WRAP(CLK_IMP_IIC_WRAP_EN_AP_CLOCK_I2C6,
			  "imp_iic_wrap_en_ap_clock_i2c6", "i2c_sel", 1),
};

static const struct mtk_clk_desc imp_iic_wrap_c_desc = {
	.clks = imp_iic_wrap_c_clks,
	.num_clks = ARRAY_SIZE(imp_iic_wrap_c_clks),
};

static const struct mtk_clk_desc imp_iic_wrap_w_desc = {
	.clks = imp_iic_wrap_w_clks,
	.num_clks = ARRAY_SIZE(imp_iic_wrap_w_clks),
};

static const struct mtk_clk_desc imp_iic_wrap_en_desc = {
	.clks = imp_iic_wrap_en_clks,
	.num_clks = ARRAY_SIZE(imp_iic_wrap_en_clks),
};

static const struct of_device_id of_match_clk_mt8188_imp_iic_wrap[] = {
	{
		.compatible = "mediatek,mt8188-imp_iic_wrap_c",
		.data = &imp_iic_wrap_c_desc,
	}, {
		.compatible = "mediatek,mt8188-imp_iic_wrap_w",
		.data = &imp_iic_wrap_w_desc,
	}, {
		.compatible = "mediatek,mt8188-imp_iic_wrap_en",
		.data = &imp_iic_wrap_en_desc,
	}, {
		/* sentinel */
	}
};

static struct platform_driver clk_mt8188_imp_iic_wrap_drv = {
	.probe = mtk_clk_simple_probe,
	.driver = {
		.name = "clk-mt8188-imp_iic_wrap",
		.of_match_table = of_match_clk_mt8188_imp_iic_wrap,
	},
};

module_platform_driver(clk_mt8188_imp_iic_wrap_drv);
MODULE_LICENSE("GPL");
