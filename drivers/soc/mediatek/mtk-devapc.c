// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#define VIO_MOD_TO_REG_IND(m)	((m) / 32)
#define VIO_MOD_TO_REG_OFF(m)	((m) % 32)

struct mtk_devapc_vio_dbgs {
	union {
		u32 vio_dbg0;
		struct {
			u32 mstid:16;
			u32 dmnid:6;
			u32 vio_w:1;
			u32 vio_r:1;
			u32 addr_h:4;
			u32 resv:4;
		} dbg0_bits;

		struct {
			u32 dmnid:6;
			u32 vio_w:1;
			u32 vio_r:1;
			u32 addr_h:4;
			u32 resv:20;
		} dbg0_bits_ver2;
	};

	u32 vio_dbg1;
	u32 vio_dbg2;
	u32 vio_dbg3;
};

struct mtk_devapc_data {
	/* architecture version */
	u32 version;

	/* default numbers of violation index */
	u32 default_vio_idx_num;

	/* reg offset */
	u32 vio_mask_offset;
	u32 vio_sta_offset;
	u32 vio_dbg0_offset;
	u32 vio_dbg1_offset;
	u32 vio_dbg2_offset;
	u32 vio_dbg3_offset;
	u32 apc_con_offset;
	u32 vio_shift_sta_offset;
	u32 vio_shift_sel_offset;
	u32 vio_shift_con_offset;
};

struct mtk_devapc_context {
	struct device *dev;
	void __iomem *base;
	struct clk *infra_clk;
	const struct mtk_devapc_data *data;

	/* numbers of violation index */
	u32 vio_idx_num;
};

static void clear_vio_status(struct mtk_devapc_context *ctx)
{
	void __iomem *reg;
	int i;

	reg = ctx->base + ctx->data->vio_sta_offset;

	for (i = 0; i < VIO_MOD_TO_REG_IND(ctx->vio_idx_num - 1); i++)
		writel(GENMASK(31, 0), reg + 4 * i);

	writel(GENMASK(VIO_MOD_TO_REG_OFF(ctx->vio_idx_num - 1), 0),
	       reg + 4 * i);
}

static void mask_module_irq(struct mtk_devapc_context *ctx, bool mask)
{
	void __iomem *reg;
	u32 val;
	int i;

	reg = ctx->base + ctx->data->vio_mask_offset;

	if (mask)
		val = GENMASK(31, 0);
	else
		val = 0;

	for (i = 0; i < VIO_MOD_TO_REG_IND(ctx->vio_idx_num - 1); i++)
		writel(val, reg + 4 * i);

	val = readl(reg + 4 * i);
	if (mask)
		val |= GENMASK(VIO_MOD_TO_REG_OFF(ctx->vio_idx_num - 1),
			       0);
	else
		val &= ~GENMASK(VIO_MOD_TO_REG_OFF(ctx->vio_idx_num - 1),
				0);

	writel(val, reg + 4 * i);
}

#define PHY_DEVAPC_TIMEOUT	0x10000

/*
 * devapc_sync_vio_dbg - do "shift" mechansim" to get full violation information.
 *                       shift mechanism is depends on devapc hardware design.
 *                       Mediatek devapc set multiple slaves as a group.
 *                       When violation is triggered, violation info is kept
 *                       inside devapc hardware.
 *                       Driver should do shift mechansim to sync full violation
 *                       info to VIO_DBGs registers.
 *
 */
static int devapc_sync_vio_dbg(struct mtk_devapc_context *ctx)
{
	void __iomem *pd_vio_shift_sta_reg;
	void __iomem *pd_vio_shift_sel_reg;
	void __iomem *pd_vio_shift_con_reg;
	int min_shift_group;
	int ret;
	u32 val;

	pd_vio_shift_sta_reg = ctx->base +
			       ctx->data->vio_shift_sta_offset;
	pd_vio_shift_sel_reg = ctx->base +
			       ctx->data->vio_shift_sel_offset;
	pd_vio_shift_con_reg = ctx->base +
			       ctx->data->vio_shift_con_offset;

	/* Find the minimum shift group which has violation */
	val = readl(pd_vio_shift_sta_reg);
	if (!val)
		return false;

	min_shift_group = __ffs(val);

	/* Assign the group to sync */
	writel(0x1 << min_shift_group, pd_vio_shift_sel_reg);

	/* Start syncing */
	writel(0x1, pd_vio_shift_con_reg);

	ret = readl_poll_timeout(pd_vio_shift_con_reg, val, val == 0x3, 0,
				 PHY_DEVAPC_TIMEOUT);
	if (ret) {
		dev_notice(ctx->dev, "%s: Shift violation info failed\n", __func__);
		return false;
	}

	/* Stop syncing */
	writel(0x0, pd_vio_shift_con_reg);

	/* Write clear */
	writel(0x1 << min_shift_group, pd_vio_shift_sta_reg);

	return true;
}

/*
 * devapc_extract_vio_dbg - extract full violation information after doing
 *                          shift mechanism.
 */
static void devapc_extract_vio_dbg(struct mtk_devapc_context *ctx)
{
	struct mtk_devapc_vio_dbgs vio_dbgs = { 0 };
	void __iomem *vio_dbg0_reg;
	void __iomem *vio_dbg1_reg;
	void __iomem *vio_dbg2_reg;
	void __iomem *vio_dbg3_reg;
	u32 vio_addr_l, vio_addr_h, bus_id, domain_id;
	u32 vio_w, vio_r;
	u64 vio_addr;

	vio_dbg0_reg = ctx->base + ctx->data->vio_dbg0_offset;
	vio_dbg1_reg = ctx->base + ctx->data->vio_dbg1_offset;
	vio_dbg2_reg = ctx->base + ctx->data->vio_dbg2_offset;
	vio_dbg3_reg = ctx->base + ctx->data->vio_dbg3_offset;

	vio_dbgs.vio_dbg0 = readl(vio_dbg0_reg);
	vio_dbgs.vio_dbg1 = readl(vio_dbg1_reg);
	if (ctx->data->version >= 2U)
		vio_dbgs.vio_dbg2 = readl(vio_dbg2_reg);
	if (ctx->data->version == 3U)
		vio_dbgs.vio_dbg3 = readl(vio_dbg3_reg);

	if (ctx->data->version == 1U) {
		/* arch version 1 */
		bus_id = vio_dbgs.dbg0_bits.mstid;
		vio_addr = vio_dbgs.vio_dbg1;
		domain_id = vio_dbgs.dbg0_bits.dmnid;
		vio_w = vio_dbgs.dbg0_bits.vio_w;
		vio_r = vio_dbgs.dbg0_bits.vio_r;
	} else {
		/* arch version 2 & 3 */
		bus_id = vio_dbgs.vio_dbg1;

		vio_addr_l = vio_dbgs.vio_dbg2;
		vio_addr_h = ctx->data->version == 2U ? vio_dbgs.dbg0_bits_ver2.addr_h :
							vio_dbgs.vio_dbg3;
		vio_addr = ((u64)vio_addr_h << 32) + vio_addr_l;
		domain_id = vio_dbgs.dbg0_bits_ver2.dmnid;
		vio_w = vio_dbgs.dbg0_bits_ver2.vio_w;
		vio_r = vio_dbgs.dbg0_bits_ver2.vio_r;
	}

	/* Print violation information */
	if (vio_w)
		dev_info(ctx->dev, "Write Violation\n");
	else if (vio_r)
		dev_info(ctx->dev, "Read Violation\n");

	dev_info(ctx->dev, "Bus ID:0x%x, Dom ID:0x%x, Vio Addr:0x%llx\n",
		 bus_id, domain_id, vio_addr);
}

/*
 * devapc_violation_irq - the devapc Interrupt Service Routine (ISR) will dump
 *                        violation information including which master violates
 *                        access slave.
 */
static irqreturn_t devapc_violation_irq(int irq_number, void *data)
{
	struct mtk_devapc_context *ctx = data;

	mask_module_irq(ctx, true);

	while (devapc_sync_vio_dbg(ctx))
		devapc_extract_vio_dbg(ctx);

	clear_vio_status(ctx);

	mask_module_irq(ctx, false);

	return IRQ_HANDLED;
}

/*
 * start_devapc - unmask slave's irq to start receiving devapc violation.
 */
static void start_devapc(struct mtk_devapc_context *ctx)
{
	devapc_violation_irq(0, ctx);
	dev_info(ctx->dev, "%s: Clear pending violation done...\n", __func__);

	mask_module_irq(ctx, false);
}

/*
 * stop_devapc - mask slave's irq to stop service.
 */
static void stop_devapc(struct mtk_devapc_context *ctx)
{
	mask_module_irq(ctx, true);

	writel(BIT(2), ctx->base + ctx->data->apc_con_offset);
}

static const struct mtk_devapc_data devapc_mt6779 = {
	.version = 1,
	.default_vio_idx_num = 511,
	.vio_mask_offset = 0x0,
	.vio_sta_offset = 0x400,
	.vio_dbg0_offset = 0x900,
	.vio_dbg1_offset = 0x904,
	.apc_con_offset = 0xF00,
	.vio_shift_sta_offset = 0xF10,
	.vio_shift_sel_offset = 0xF14,
	.vio_shift_con_offset = 0xF20,
};

static const struct mtk_devapc_data devapc_mt8188 = {
	.version = 3,
	.vio_mask_offset = 0x0,
	.vio_sta_offset = 0x400,
	.vio_dbg0_offset = 0x900,
	.vio_dbg1_offset = 0x904,
	.vio_dbg2_offset = 0x908,
	.vio_dbg3_offset = 0x90C,
	.apc_con_offset = 0xF00,
	.vio_shift_sta_offset = 0xF20,
	.vio_shift_sel_offset = 0xF30,
	.vio_shift_con_offset = 0xF10,
};

static const struct of_device_id mtk_devapc_dt_match[] = {
	{
		.compatible = "mediatek,mt6779-devapc",
		.data = &devapc_mt6779,
	}, {
		.compatible = "mediatek,mt8188-devapc",
		.data = &devapc_mt8188,
	}, {
	},
};
MODULE_DEVICE_TABLE(of, mtk_devapc_dt_match);

static int mtk_devapc_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct mtk_devapc_context *ctx;
	u32 devapc_irq;
	int ret;

	if (IS_ERR(node))
		return -ENODEV;

	ctx = devm_kzalloc(&pdev->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->data = of_device_get_match_data(&pdev->dev);
	if (ctx->data->version == 0 || ctx->data->version >= 4)
		return -EINVAL;

	ctx->dev = &pdev->dev;

	ctx->base = of_iomap(node, 0);
	if (!ctx->base)
		return -EINVAL;

	/* Set vio_idx_num to default value.
	 * If the value is 0, get the info from DT.
	 */
	ctx->vio_idx_num = ctx->data->default_vio_idx_num;
	if (!ctx->vio_idx_num)
		if (of_property_read_u32(node,
					 "vio-idx-num",
					 &ctx->vio_idx_num))
			return -EINVAL;

	devapc_irq = irq_of_parse_and_map(node, 0);
	if (!devapc_irq)
		return -EINVAL;

	ctx->infra_clk = devm_clk_get(&pdev->dev, "devapc-infra-clock");
	if (IS_ERR(ctx->infra_clk))
		return -EINVAL;

	if (clk_prepare_enable(ctx->infra_clk))
		return -EINVAL;

	ret = devm_request_irq(&pdev->dev, devapc_irq, devapc_violation_irq,
			       IRQF_TRIGGER_NONE | IRQF_SHARED, "devapc", ctx);
	if (ret) {
		clk_disable_unprepare(ctx->infra_clk);
		return ret;
	}

	platform_set_drvdata(pdev, ctx);

	start_devapc(ctx);

	return 0;
}

static int mtk_devapc_remove(struct platform_device *pdev)
{
	struct mtk_devapc_context *ctx = platform_get_drvdata(pdev);

	stop_devapc(ctx);

	clk_disable_unprepare(ctx->infra_clk);

	return 0;
}

static struct platform_driver mtk_devapc_driver = {
	.probe = mtk_devapc_probe,
	.remove = mtk_devapc_remove,
	.driver = {
		.name = "mtk-devapc",
		.of_match_table = mtk_devapc_dt_match,
	},
};

module_platform_driver(mtk_devapc_driver);

MODULE_DESCRIPTION("Mediatek Device APC Driver");
MODULE_AUTHOR("Neal Liu <neal.liu@mediatek.com>");
MODULE_LICENSE("GPL");
