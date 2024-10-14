// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015-2016 MediaTek Inc.
 * Author: Yong Wu <yong.wu@mediatek.com>
 */
#include <linux/clk.h>
#include <linux/component.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/pm_runtime.h>
#include <soc/mediatek/smi.h>
#include <dt-bindings/memory/mt2701-larb-port.h>
#include <dt-bindings/memory/mtk-memory-port.h>

#include <linux/kthread.h>

#ifdef CONFIG_MTK_IOMMU_MT8XXX
#include "mtk_iommu_sec.h"
#endif

/* mt8173 */
#define SMI_LARB_MMU_EN		0xf00

/* mt8167 */
#define MT8167_SMI_LARB_MMU_EN	0xfc0

/* mt2701 */
#define REG_SMI_SECUR_CON_BASE		0x5c0

/* every register control 8 port, register offset 0x4 */
#define REG_SMI_SECUR_CON_OFFSET(id)	(((id) >> 3) << 2)
#define REG_SMI_SECUR_CON_ADDR(id)	\
	(REG_SMI_SECUR_CON_BASE + REG_SMI_SECUR_CON_OFFSET(id))

/*
 * every port have 4 bit to control, bit[port + 3] control virtual or physical,
 * bit[port + 2 : port + 1] control the domain, bit[port] control the security
 * or non-security.
 */
#define SMI_SECUR_CON_VAL_MSK(id)	(~(0xf << (((id) & 0x7) << 2)))
#define SMI_SECUR_CON_VAL_VIRT(id)	BIT((((id) & 0x7) << 2) + 3)
/* mt2701 domain should be set to 3 */
#define SMI_SECUR_CON_VAL_DOMAIN(id)	(0x3 << ((((id) & 0x7) << 2) + 1))

/* mt2712 */
#define SMI_LARB_NONSEC_CON(id)	(0x380 + ((id) * 4))
#define F_MMU_EN		BIT(0)
#define BANK_SEL(id)		({			\
	u32 _id = (id) & 0x3;				\
	(_id << 8 | _id << 10 | _id << 12 | _id << 14);	\
})
#define PATH_SEL(a)		((((a) & 0x1) << 16) | (((a) & 0x1) << 17) |\
				 (((a) & 0x1) << 18) | (((a) & 0x1) << 19))

/* mt6873 */
#define SMI_LARB_OSTDL_PORT		0x200
#define SMI_LARB_OSTDL_PORTx(id)	(SMI_LARB_OSTDL_PORT + ((id) << 2))

#define SMI_LARB_SLP_CON		0x00c
#define SLP_PROT_EN			BIT(0)
#define SLP_PROT_RDY			BIT(16)
#define SMI_LARB_CMD_THRT_CON		0x24
#define SMI_LARB_SW_FLAG		0x40
#define SMI_LARB_WRR_PORT		0x100
#define SMI_LARB_WRR_PORTx(id)		(SMI_LARB_WRR_PORT + ((id) << 2))
#define SMI_LARB_FORCE_ULTRA		0x78

/* mt6893 */
#define SMI_LARB_VC_PRI_MODE		(0x20)
#define SMI_LARB_DBG_CON			(0xf0)
#define INT_SMI_LARB_DBG_CON		(0x500 + (SMI_LARB_DBG_CON))
#define INT_SMI_LARB_CMD_THRT_CON	(0x500 + (SMI_LARB_CMD_THRT_CON))

/* SMI COMMON */
#define SMI_BUS_SEL			0x220
#define SMI_BUS_LARB_SHIFT(larbid)	((larbid) << 1)
/* All are MMU0 defaultly. Only specialize mmu1 here. */
#define F_MMU1_LARB(larbid)		(0x1 << SMI_BUS_LARB_SHIFT(larbid))
#define SMI_L1LEN			0x100
#define SMI_L1ARB0			0x104
#define SMI_L1ARB(id)			(SMI_L1ARB0 + ((id) << 2))
#define SMI_M4U_TH			0x234
#define SMI_FIFO_TH1			0x238
#define SMI_FIFO_TH2			0x23c
#define SMI_PREULTRA_MASK1	0x244
#define SMI_DCM				0x300
#define SMI_DUMMY			0x444
#define SMI_LARB_PORT_NR_MAX		32
#define SMI_COMMON_LARB_NR_MAX		8
#define MTK_COMMON_NR_MAX		20
#define SMI_LARB_MISC_NR		5
#define SMI_COMMON_MISC_NR		8
struct mtk_smi_reg_pair {
	u16	offset;
	u32	value;
};


#define SMI_L1LEN			0x100
#define SMI_L1ARB0			0x104
#define SMI_L1ARB(id)			(SMI_L1ARB0 + ((id) << 2))

enum mtk_smi_gen {
	MTK_SMI_GEN1,
	MTK_SMI_GEN2,
	MTK_SMI_GEN3
};

struct mtk_smi_common_plat {
	enum mtk_smi_gen gen;
	bool             has_gals;
	u32              bus_sel; /* Balance some larbs to enter mmu0 or mmu1 */
	bool		has_bwl;
	u16		*bwl;
	struct mtk_smi_reg_pair *misc;
};

struct mtk_smi_larb_gen {
	int port_in_larb[MTK_LARB_NR_MAX + 1];
	int port_in_larb_gen2[MTK_LARB_NR_MAX + 1];
	void (*config_port)(struct device *);
	void (*sleep_ctrl)(struct device *dev, bool toslp);
	unsigned int			larb_direct_to_common_mask;
	bool				has_gals;
	bool		has_bwl;
	bool		has_grouping;
	bool		has_bw_thrt;
	u8		*bwl;
	u8		*cmd_group; /* ovl with large ostd */
	u8		*bw_thrt_en; /* non-HRT */
	unsigned long	sram_larb;
	u32		*sram_port;
	struct mtk_smi_reg_pair *misc;
};

struct mtk_smi {
	struct device			*dev;
	struct clk			*clk_apb, *clk_smi;
	struct clk			*clk_gals0, *clk_gals1, *clk_gals2;
	struct clk			*clk_async; /*only needed by mt2701*/
	struct clk			*clk_reorder;
	union {
		void __iomem		*smi_ao_base; /* only for gen1 */
		void __iomem		*base;	      /* only for gen2 */
	};
	const struct mtk_smi_common_plat *plat;
	int				smi_id;
	bool				init_power_on;
};

struct mtk_smi_larb { /* larb: local arbiter */
	struct mtk_smi			smi;
	void __iomem			*base;
	struct device			*smi_common_dev;
	const struct mtk_smi_larb_gen	*larb_gen;
	/* merge larb->smi.larbid & smi->commid to smi->smi_id */
	u32				*mmu;
	unsigned char			*bank;
	bool				resumed;	/* not 1st resume */
	struct list_head		power_on;
};

static u32 log_level;
enum smi_log_level {
	log_config_bit = 0,
	log_set_bw,
	log_pm_control,
};
#define log_level_with(bit)		(log_level & BIT(bit))

static LIST_HEAD(init_power_on_list);

void mtk_smi_common_bw_set(struct device *dev, const u32 port, const u32 val)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	struct mtk_smi *common;

	if (!larb)
		return;

	common = dev_get_drvdata(larb->smi_common_dev);
	if (!common)
		return;

	if (common->plat->bwl) {
		if (common->plat->gen == MTK_SMI_GEN3)
			common->plat->bwl[common->smi_id *
					  SMI_COMMON_LARB_NR_MAX + port] = val;
		else if (common->plat->gen == MTK_SMI_GEN2)
			common->plat->bwl[port] = val;
	}

	if (pm_runtime_active(common->dev)) {
		writel(val, common->base + SMI_L1ARB(port));
	} else if (log_level_with(log_set_bw)) {
		dev_notice(dev, "set common bw fail reg:%#x, port:%d, val:%u\n",
			common->base + SMI_L1ARB(port), port, val);
		dump_stack();
	}
}
EXPORT_SYMBOL_GPL(mtk_smi_common_bw_set);

void mtk_smi_larb_bw_set(struct device *dev, const u32 port, const u32 val)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);

	if (!val || !larb)
		return;

	if (larb->larb_gen->bwl)
		larb->larb_gen->bwl[larb->smi.smi_id * SMI_LARB_PORT_NR_MAX +
				    port] = val;

	if (pm_runtime_active(larb->smi.dev)) {
		writel(val, larb->base + SMI_LARB_OSTDL_PORTx(port));
	} else if (log_level_with(log_set_bw)) {
		dev_notice(dev, "set larb bw fail larb:%d, port:%d, val:%u\n",
			larb->smi.smi_id, port, val);
		dump_stack();
	}

}
EXPORT_SYMBOL_GPL(mtk_smi_larb_bw_set);

void mtk_smi_init_power_off(void)
{
	struct mtk_smi_larb *larb, *tmp;

	/* SMI init-power-on:
	 * Put sync pm runtime status for SMI LARB which with "init-power-on"
	 * only once time. Then the linked SMI COMMON and IOMMU will be put
	 * sync, too.
	 */
	list_for_each_entry_safe(larb, tmp, &init_power_on_list, power_on) {
		if (larb->smi.init_power_on) {
			pm_runtime_put_sync(larb->smi.dev);
			larb->smi.init_power_on = false;
		}
	}
}
EXPORT_SYMBOL_GPL(mtk_smi_init_power_off);

static int mtk_smi_clk_enable(const struct mtk_smi *smi)
{
	int ret;

	ret = clk_prepare_enable(smi->clk_apb);
	if (ret)
		return ret;

	ret = clk_prepare_enable(smi->clk_smi);
	if (ret)
		goto err_disable_apb;

	ret = clk_prepare_enable(smi->clk_gals0);
	if (ret)
		goto err_disable_smi;

	ret = clk_prepare_enable(smi->clk_gals1);
	if (ret)
		goto err_disable_gals0;

	ret = clk_prepare_enable(smi->clk_gals2);
	if (ret)
		goto err_disable_gals1;

	ret = clk_prepare_enable(smi->clk_reorder);
	if (ret)
		goto err_disable_gals2;

	return 0;

err_disable_gals2:
	clk_disable_unprepare(smi->clk_gals2);
err_disable_gals1:
	clk_disable_unprepare(smi->clk_gals1);
err_disable_gals0:
	clk_disable_unprepare(smi->clk_gals0);
err_disable_smi:
	clk_disable_unprepare(smi->clk_smi);
err_disable_apb:
	clk_disable_unprepare(smi->clk_apb);
	return ret;
}

static void mtk_smi_clk_disable(const struct mtk_smi *smi)
{
	clk_disable_unprepare(smi->clk_reorder);
	clk_disable_unprepare(smi->clk_gals2);
	clk_disable_unprepare(smi->clk_gals1);
	clk_disable_unprepare(smi->clk_gals0);
	clk_disable_unprepare(smi->clk_smi);
	clk_disable_unprepare(smi->clk_apb);
}

int mtk_smi_larb_get(struct device *larbdev)
{
	int ret = pm_runtime_resume_and_get(larbdev);

	return (ret < 0) ? ret : 0;
}
EXPORT_SYMBOL_GPL(mtk_smi_larb_get);

void mtk_smi_larb_put(struct device *larbdev)
{
	pm_runtime_put_sync(larbdev);
}
EXPORT_SYMBOL_GPL(mtk_smi_larb_put);

void mtk_smi_add_device_link(struct device *dev, struct device *larbdev)
{
	struct device_link *link;
	struct mtk_smi_larb *larb = dev_get_drvdata(larbdev);

	link = device_link_add(dev, larbdev,
			       DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);
	if (!link)
		dev_notice(dev, "Unable to link SMI LARB%d\n", larb->smi.smi_id);

}
EXPORT_SYMBOL_GPL(mtk_smi_add_device_link);

static void mtk_smi_larb_sleep_ctrl(struct device *dev, bool toslp)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	void __iomem *base = larb->base;
	u32 tmp;

	if (toslp) {
		writel_relaxed(SLP_PROT_EN, base + SMI_LARB_SLP_CON);
		if (readl_poll_timeout_atomic(base + SMI_LARB_SLP_CON,
				tmp, !!(tmp & SLP_PROT_RDY), 10, 10000))
			dev_warn(dev, "larb sleep con not ready(%d)\n", tmp);
	} else
		writel_relaxed(0, base + SMI_LARB_SLP_CON);
}

static int
mtk_smi_larb_bind(struct device *dev, struct device *master, void *data)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	struct mtk_smi_larb_iommu *larb_mmu = data;
	unsigned int         i;

	if (log_level_with(log_config_bit))
		pr_info("[SMI] start larb bind\n");
	for (i = 0; i < MTK_LARB_NR_MAX; i++) {
		if (dev == larb_mmu[i].dev) {
			larb->smi.smi_id = i;
			larb->mmu = &larb_mmu[i].mmu;
			larb->bank = larb_mmu[i].bank;
			if (log_level_with(log_config_bit))
				dev_notice(dev,
					"[SMI]larb%d bind ptr_mmu:0x%x val_mmu_32:0x%x bit32:%u\n",
					i, larb->mmu, *((unsigned int *)(larb->mmu)),
					larb->bank[i]);
			return 0;
		}
	}
	if (log_level_with(log_config_bit))
		dev_notice(dev, "failed to find any larb matched\n");
	return -ENODEV;
}

static void mtk_smi_larb_config_port_gen2_general(struct device *dev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	u32 reg;
	int i, j, idx = -1;

	if (BIT(larb->smi.smi_id) & larb->larb_gen->larb_direct_to_common_mask)
		return;

	if (log_level_with(log_config_bit))
		dev_notice(dev, "[SMI]larb%d config port\n", larb->smi.smi_id);

	if (BIT(larb->smi.smi_id) & larb->larb_gen->sram_larb) {
		for_each_set_bit(j, &larb->larb_gen->sram_larb, 32) {
			idx++;
			if (j == larb->smi.smi_id)
				break;
		}
	}

	if (larb->mmu) {
		if (log_level_with(log_config_bit))
			dev_notice(dev, "[SMI]larb%d config mmu:0x%lx mmu_32:0x%x\n",
				larb->smi.smi_id, *((unsigned long *)(larb->mmu)),
				*((unsigned int *)(larb->mmu)));
		for_each_set_bit(i, (unsigned long *)larb->mmu, 32) {
			reg = readl_relaxed(larb->base + SMI_LARB_NONSEC_CON(i));
			reg |= F_MMU_EN;
			if ((idx != -1) &&
			    ((BIT(i) & larb->larb_gen->sram_port[idx])))
				reg |= PATH_SEL(1);
			else
				reg |= BANK_SEL(larb->bank[i]);
			writel(reg, larb->base + SMI_LARB_NONSEC_CON(i));
			if (log_level_with(log_config_bit))
				dev_notice(dev,
					"[SMI]larb:%d port:%d mmu:%u bit32:%u offset:%#x reg:%#x\n",
					larb->smi.smi_id, i, larb->mmu, larb->bank[i],
					SMI_LARB_NONSEC_CON(i), reg);
		}
	}

	if (!larb->larb_gen->has_bwl)
		return;

	for (i = 0; i < larb->larb_gen->port_in_larb_gen2[larb->smi.smi_id]; i++)
		writel(larb->larb_gen->bwl[larb->smi.smi_id * SMI_LARB_PORT_NR_MAX + i],
			larb->base + SMI_LARB_OSTDL_PORTx(i));

	for (i = 0; i < SMI_LARB_MISC_NR; i++)
		writel_relaxed(larb->larb_gen->misc[
			larb->smi.smi_id * SMI_LARB_MISC_NR + i].value,
			larb->base + larb->larb_gen->misc[
			larb->smi.smi_id * SMI_LARB_MISC_NR + i].offset);

	if (!larb->larb_gen->has_grouping || !larb->larb_gen->has_bw_thrt)
		return;

	for (i = larb->larb_gen->cmd_group[larb->smi.smi_id * 2];
		i < larb->larb_gen->cmd_group[larb->smi.smi_id * 2 + 1]; i++) {
		writel_relaxed(readl_relaxed(larb->base + SMI_LARB_NONSEC_CON(i)) | 0x2,
			larb->base + SMI_LARB_NONSEC_CON(i));
	}

	for (i = larb->larb_gen->bw_thrt_en[larb->smi.smi_id * 2];
		i < larb->larb_gen->bw_thrt_en[larb->smi.smi_id * 2 + 1]; i++) {
		writel_relaxed(readl_relaxed(larb->base + SMI_LARB_NONSEC_CON(i)) | 0x8,
			larb->base + SMI_LARB_NONSEC_CON(i));
	}

	wmb(); /* make sure settings are written */

}

static void mtk_smi_larb_config_port_mt8173(struct device *dev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);

	writel(*larb->mmu, larb->base + SMI_LARB_MMU_EN);
}

static void mtk_smi_larb_config_port_mt8167(struct device *dev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);

	writel(*larb->mmu, larb->base + MT8167_SMI_LARB_MMU_EN);
}

static void mtk_smi_larb_config_port_mt8188(struct device *dev)
{
#ifdef CONFIG_MTK_IOMMU_MT8XXX
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	struct arm_smccc_res res;

	if (larb->mmu) {
		arm_smccc_smc(MTK_IOMMU_SECURE_CONTROL,
			      IOMMU_ATF_SET_LARB_COMMAND(larb->smi.smi_id,
			      IOMMU_ATF_SET_SMI_SEC_LARB), *larb->mmu,
			      0, 0, 0, 0, 0, &res);
		if (res.a0 != 0)
			dev_err(dev, "secure config fail, mmu 0x%x ret %ld\n",
				*larb->mmu, res.a0);
	}
#endif

	mtk_smi_larb_config_port_gen2_general(dev);
}

static void mtk_smi_larb_config_port_mt8195(struct device *dev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);

	/* for disp lk switch to kernel, they use cmdq to enable iommu &
	 * replace pa to iova simultaneously. So we don't enable iommu
	 * in the first resume.
	 */
	if (!larb->resumed) {
		larb->resumed = true;
		if (larb->smi.smi_id == 0 || larb->smi.smi_id == 1)
			return;
	}
	mtk_smi_larb_config_port_gen2_general(dev);
}

static void mtk_smi_larb_config_port_gen1(struct device *dev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	const struct mtk_smi_larb_gen *larb_gen = larb->larb_gen;
	struct mtk_smi *common = dev_get_drvdata(larb->smi_common_dev);
	int i, m4u_port_id, larb_port_num;
	u32 sec_con_val, reg_val;

	m4u_port_id = larb_gen->port_in_larb[larb->smi.smi_id];
	larb_port_num = larb_gen->port_in_larb[larb->smi.smi_id + 1]
			- larb_gen->port_in_larb[larb->smi.smi_id];

	for (i = 0; i < larb_port_num; i++, m4u_port_id++) {
		if (*larb->mmu & BIT(i)) {
			/* bit[port + 3] controls the virtual or physical */
			sec_con_val = SMI_SECUR_CON_VAL_VIRT(m4u_port_id);
		} else {
			/* do not need to enable m4u for this port */
			continue;
		}
		reg_val = readl(common->smi_ao_base
			+ REG_SMI_SECUR_CON_ADDR(m4u_port_id));
		reg_val &= SMI_SECUR_CON_VAL_MSK(m4u_port_id);
		reg_val |= sec_con_val;
		reg_val |= SMI_SECUR_CON_VAL_DOMAIN(m4u_port_id);
		writel(reg_val,
			common->smi_ao_base
			+ REG_SMI_SECUR_CON_ADDR(m4u_port_id));
	}
}

static void
mtk_smi_larb_unbind(struct device *dev, struct device *master, void *data)
{
	/* Do nothing as the iommu is always enabled. */
}

static const struct component_ops mtk_smi_larb_component_ops = {
	.bind = mtk_smi_larb_bind,
	.unbind = mtk_smi_larb_unbind,
};

static u8
mtk_smi_larb_mt6873_bwl[MTK_LARB_NR_MAX][SMI_LARB_PORT_NR_MAX] = {
	{0x2, 0x2, 0x28, 0xa, 0xc, 0x28,},
	{0x2, 0x2, 0x18, 0x18, 0x18, 0xa, 0xc, 0x28,},
	{0x5, 0x5, 0x5, 0x5, 0x1,},
	{},
	{0x28, 0x19, 0xb, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x4, 0x1,},
	{0x1, 0x1, 0x4, 0x1, 0x1, 0x1, 0x1, 0x16,},
	{},
	{0x1, 0x3, 0x2, 0x1, 0x1, 0x5, 0x2, 0x12, 0x13, 0x4, 0x4, 0x1,
	 0x4, 0x2, 0x1,},
	{},
	{0xa, 0x7, 0xf, 0x8, 0x1, 0x8, 0x9, 0x3, 0x3, 0x6, 0x7, 0x4,
	 0xa, 0x3, 0x4, 0xe, 0x1, 0x7, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
	 0x1, 0x1, 0x1, 0x1, 0x1,},
	{},
	{0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
	 0x1, 0x1, 0x1, 0xe, 0x1, 0x7, 0x8, 0x7, 0x7, 0x1, 0x6, 0x2,
	 0xf, 0x8, 0x1, 0x1, 0x1,},
	{},
	{0x2, 0xc, 0xc, 0xe, 0x6, 0x6, 0x6, 0x6, 0x6, 0x12, 0x6, 0x28,},
	{0x2, 0xc, 0xc, 0x28, 0x12, 0x6,},
	{},
	{0x28, 0x14, 0x2, 0xc, 0x18, 0x4, 0x28, 0x14, 0x4, 0x4, 0x4, 0x2,
	 0x4, 0x2, 0x8, 0x4, 0x4,},
	{0x28, 0x14, 0x2, 0xc, 0x18, 0x4, 0x28, 0x14, 0x4, 0x4, 0x4, 0x2,
	 0x4, 0x2, 0x8, 0x4, 0x4,},
	{0x28, 0x14, 0x2, 0xc, 0x18, 0x4, 0x28, 0x14, 0x4, 0x4, 0x4, 0x2,
	 0x4, 0x2, 0x8, 0x4, 0x4,},
	{0x2, 0x2, 0x4, 0x2,},
	{0x9, 0x9, 0x5, 0x5, 0x1, 0x1,},
};

static u8
mtk_smi_larb_mt6853_bwl[MTK_LARB_NR_MAX][SMI_LARB_PORT_NR_MAX] = {
	{0x2, 0x2, 0x28, 0x28,},
	{0x2, 0x18, 0xa, 0xc, 0x28,},
	{0x5, 0x5, 0x5, 0x5, 0x1,},
	{},
	{0x28, 0x19, 0xb, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x4, 0x1, 0x16,},
	{},
	{},
	{0x1, 0x3, 0x2, 0x1, 0x1, 0x5, 0x2, 0x12, 0x13, 0x4, 0x4, 0x1, 0x4,},
	{},
	{0xa, 0x7, 0xf, 0x8, 0x1, 0x8, 0x9, 0x3, 0x3, 0x6, 0x7, 0x4,
	 0xa, 0x3, 0x4, 0xe, 0x1, 0x7, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
	 0x1, 0x1, 0x1, 0x1, 0x1,},
	{},
	{0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
	 0x1, 0x1, 0x1, 0xe, 0x1, 0x7, 0x8, 0x7, 0x7, 0x1, 0x6, 0x2,
	 0xf, 0x8, 0x1, 0x1, 0x1,},
	{},
	{0x2, 0xc, 0xc, 0x1, 0x1, 0x1, 0x6, 0x6, 0x6, 0x12, 0x6, 0x1,},
	{0x1, 0x1, 0x1, 0x28, 0x12, 0x6,},
	{0x28, 0x1, 0x2, 0x28, 0x1,},
	{0x28, 0x14, 0x2, 0xc, 0x18, 0x4, 0x28, 0x14, 0x4, 0x4, 0x4, 0x2,
	 0x4, 0x2, 0x8, 0x4, 0x4,},
	{0x28, 0x14, 0x2, 0xc, 0x18, 0x4, 0x28, 0x14, 0x4, 0x4, 0x4, 0x2,
	 0x4, 0x2, 0x8, 0x4, 0x4,},
	{0x28, 0x14, 0x2, 0xc, 0x18, 0x4, 0x28, 0x14, 0x4, 0x4, 0x4, 0x2,
	 0x4, 0x2, 0x8, 0x4, 0x4,},
	{0x2, 0x2, 0x4, 0x2,},
	{0x9, 0x9, 0x5, 0x5, 0x1, 0x1,},
};

static u8
mtk_smi_larb_mt6893_bwl[MTK_LARB_NR_MAX][SMI_LARB_PORT_NR_MAX] = {
	{0x2, 0x6, 0x2, 0x2, 0x2, 0x28, 0x18, 0x18, 0x1, 0x1, 0x1, 0x8, 0x8, 0x1, 0x3f,},
	{0x2, 0x6, 0x2, 0x2, 0x2, 0x28, 0x18, 0x18, 0x1, 0x1, 0x1, 0x8, 0x8, 0x1, 0x3f,},
	{0x5, 0x5, 0x5, 0x5, 0x1, 0x3f,},
	{0x5, 0x5, 0x5, 0x5, 0x1, 0x3f,},
	{0x28, 0x19, 0xb, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x4, 0x1,},
	{0x1, 0x1, 0x4, 0x1, 0x1, 0x1, 0x1, 0x16,},
	{},
	{0x1, 0x4, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x4, 0x4, 0x1,
	 0x4, 0x1, 0xa, 0x6, 0x1, 0xa, 0x6, 0x1, 0x1, 0x1, 0x1, 0x5,
	 0x3, 0x3, 0x4,},
	{0x1, 0x4, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x4, 0x4, 0x1,
	 0x4, 0x1, 0xa, 0x6, 0x1, 0xa, 0x6, 0x1, 0x1, 0x1, 0x1, 0x5,
	 0x3, 0x3, 0x4,},
	{0x9, 0x7, 0xf, 0x8, 0x1, 0x8, 0x9, 0x3, 0x3, 0x6, 0x7, 0x4,
	 0x9, 0x3, 0x4, 0xe, 0x1, 0x7, 0x8, 0x7, 0x7, 0x1, 0x6, 0x2,
	 0xf, 0x8, 0x1, 0x1, 0x1,},
	{},
	{0x9, 0x7, 0xf, 0x8, 0x1, 0x8, 0x9, 0x3, 0x3, 0x6, 0x7, 0x4,
	 0x9, 0x3, 0x4, 0xe, 0x1, 0x7, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
	 0x1, 0x1, 0x1, 0x1, 0x1,},
	{},
	{0x2, 0xc, 0xc, 0xe, 0x6, 0x6, 0x6, 0x6, 0x6, 0x12, 0x6, 0x1,},
	{0x2, 0xc, 0xc, 0x28, 0x12, 0x6,},
	{0x28, 0x1, 0x2, 0x28, 0x1,},
	{0x28, 0x14, 0x2, 0xc, 0x18, 0x2, 0x14, 0x14, 0x4, 0x4, 0x4, 0x2,
	 0x4, 0x2, 0x8, 0x4, 0x4,},
	{0x28, 0x14, 0x2, 0xc, 0x18, 0x2, 0x14, 0x14, 0x4, 0x4, 0x4, 0x2,
	 0x4, 0x2, 0x8, 0x4, 0x4,},
	{0x28, 0x14, 0x2, 0xc, 0x18, 0x2, 0x14, 0x14, 0x4, 0x4, 0x4, 0x2,
	 0x4, 0x2, 0x8, 0x4, 0x4,},
	{0x2, 0x2, 0x4, 0x2,},
	{0x9, 0x9, 0x5, 0x5, 0x1, 0x1,},
};

static u8
mtk_smi_larb_mt6893_cmd_group[MTK_LARB_NR_MAX][2] = {
	{2, 3}, {1, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
};

static u8
mtk_smi_larb_mt6893_bw_thrt_en[MTK_LARB_NR_MAX][2] = {
	{14, 15}, {4, 5},
	{0, 6}, {0, 0},
	{0, 11}, {0, 0}, {0, 0},
	{0, 27}, {0, 0},
	{0, 29}, {0, 0}, {0, 29}, {0, 0},
	{11, 12}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 4}, {0, 6},
};

static u8
mtk_smi_larb_mt6983_cmd_group[MTK_LARB_NR_MAX][2] = {
	{7, 8}, {0, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {7, 8}, {0, 1}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
};

static u8
mtk_smi_larb_mt6983_bw_thrt_en[MTK_LARB_NR_MAX][2] = {
	{14, 16}, {7, 8},
	{0, 9}, {2, 5},
	{0, 11}, {0, 9}, {0, 4},
	{0, 31}, {0, 31},
	{0, 25}, {0, 20}, {0, 30}, {0, 16},
	{0, 0}, {0, 0},
	{0, 19}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {14, 16}, /*20*/
	{7, 8}, {0, 0}, {0, 30}, {0, 30},
	{0, 0}, {0, 0}, {0, 0},/*26*/
	{0, 0}, {0, 0}, {0, 0}, {0, 0},
};

static u8
mtk_smi_larb_mt6983_bwl[MTK_LARB_NR_MAX][SMI_LARB_PORT_NR_MAX] = {
	{0x2, 0x6, 0x6, 0x4, 0x4, 0x2, 0x2, 0x40, 0x26, 0x26,
	 0x20, 0x20, 0xe, 0xe, 0x7, 0x1,}, /* LARB0 */
	{0x40, 0x26, 0x26, 0x20, 0x20, 0xe, 0xe, 0x1,}, /* LARB1 */
	{0x4, 0x4, 0x4, 0x4, 0x1, 0x1, 0x1, 0x1, 0x1,}, /* LARB2 */
	{0x6, 0x6, 0x5, 0x5, 0x1, 0x1, 0x1, 0x2, 0x2,}, /* LARB3 */
	{0x40, 0x28, 0x11, 0x1, 0x1, 0x1, 0x1, 0x2, 0x2, 0x5, 0x1,}, /* LARB4 */
	{0x1, 0x1, 0x5, 0x1, 0x1, 0x2, 0x17, 0xc, 0x24,}, /* LARB5 */
	{0x1, 0x1, 0x1, 0x1,}, /* LARB6 */
	{0x1, 0x3, 0x2, 0x1, 0x1, 0x1, 0x1, 0x4, 0x4, 0x1,
	 0x1, 0x1, 0x1, 0x2, 0x1, 0x1, 0x3, 0x8, 0x5, 0x1,
	 0x1, 0x4, 0x2, 0x2, 0x3, 0x1, 0x1, 0x8, 0x5, 0x1, 0x1,}, /* LARB7 */
	{0x1, 0x3, 0x2, 0x1, 0x1, 0x1, 0x1, 0x4, 0x4, 0x1,
	 0x1, 0x1, 0x1, 0x2, 0x1, 0x1, 0x3, 0x8, 0x5, 0x1,
	 0x1, 0x4, 0x2, 0x2, 0x3, 0x1, 0x1, 0x8, 0x5, 0x1, 0x1,}, /* LARB8 */
	{0x13, 0x1, 0x10, 0x8, 0x3, 0x5, 0x1, 0x1, 0x10, 0x10,
	 0x10, 0x1f, 0x30, 0x2, 0x13, 0x10, 0x8, 0x3, 0x2, 0x7,
	 0x5, 0x1, 0x30, 0x2, 0x7,}, /* LARB9 */
	{0x2f, 0x1, 0x8, 0x8, 0x1, 0x1, 0x4, 0x18, 0x18, 0x19,
	 0xb, 0x1, 0x10, 0x9, 0x4, 0x4, 0x15, 0x1, 0x6, 0x40,}, /* LARB10 */
	{0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1f, 0x10, 0x10, 0x13,
	 0x10, 0x8, 0x3, 0x5, 0x3, 0x1f, 0x1, 0x10, 0x1, 0x1,
	 0x10, 0x1, 0x1, 0x1f, 0x18, 0x2, 0x7, 0x5, 0x3, 0x40,}, /* LARB11 */
	{0x5, 0x5, 0x3, 0x3, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
	 0x5, 0x5, 0x3, 0x3, 0x1, 0x1,}, /* LARB12 */
	{0x2, 0x2, 0xa, 0xa, 0xa, 0xa, 0x1, 0x1, 0x2, 0x2,
	 0x2, 0x2, 0xa, 0xa, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2,
	 0x2, 0x2, 0x1, 0x1,}, /* LARB13 */
	{0xa, 0xa, 0x2, 0x2, 0x2, 0x2, 0x1, 0x1, 0xa, 0xa,
	 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x1, 0x8,
	 0x8, 0x1, 0x1,}, /* LARB14 */
	{0x1f, 0x1f, 0x4, 0x1, 0x1, 0x4, 0x10, 0x10, 0x10, 0x14,
	 0x4, 0x7, 0x15, 0x1, 0x1, 0x7, 0x9, 0x14, 0x1,}, /* LARB15 */
	{0x1e, 0x2, 0x2, 0x8, 0x2, 0x8, 0x6, 0x2, 0x2, 0x4,
	 0x8, 0x4, 0x2, 0x4, 0x8, 0x2, 0x12,}, /* LARB16 */
	{0x30, 0x10, 0xe, 0x14, 0x6, 0x2, 0x6,}, /* LARB17 */
	{0x2, 0x2, 0x40, 0x1, 0x2, 0x2, 0x6, 0x1,}, /* LARB18 */
	{0x2, 0x2, 0x2, 0x2,}, /* LARB19 */
	{0x2, 0x6, 0x6, 0x4, 0x4, 0x2, 0x2, 0x40, 0x26, 0x26,
	 0x20, 0x20, 0xe, 0xe, 0x7, 0x1,}, /* LARB20 */
	{0x40, 0x26, 0x26, 0x20, 0x20, 0xe, 0xe, 0x1,}, /* LARB21 */
	{0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1f, 0x10, 0x10, 0x13,
	 0x10, 0x8, 0x3, 0x5, 0x3, 0x1f, 0x1, 0x10, 0x1, 0x1,
	 0x10, 0x1, 0x1, 0x1f, 0x18, 0x2, 0x7, 0x5, 0x3, 0x40,}, /* LARB22 */
	{0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1f, 0x10, 0x10, 0x13,
	 0x10, 0x8, 0x3, 0x5, 0x3, 0x1f, 0x1, 0x10, 0x1, 0x1,
	 0x10, 0x1, 0x1, 0x1f, 0x18, 0x2, 0x7, 0x5, 0x3, 0x40,}, /* LARB23 */
	{}, /* LARB24 */
	{0x2, 0x2, 0x2, 0x8, 0x4, 0x2, 0x2, 0x2, 0x8, 0x4,
	 0x1, 0x4, 0x4, 0x2,}, /* LARB25 */
	{0x2, 0x2, 0x2, 0x8, 0x4, 0x1, 0x1, 0x1, 0x1, 0x1,
	 0x2, 0x4, 0x4, 0x2,}, /* LARB26 */
	{0x1e, 0x2, 0x2, 0x8, 0x2, 0x8, 0x6, 0x2, 0x2, 0x4,
	 0x8, 0x4, 0x2, 0x4, 0x8, 0x2, 0x12,}, /* LARB27 */
	{0x1e, 0x2, 0x2, 0x8, 0x2, 0x8, 0x6, 0x2, 0x2, 0x4,
	 0x8, 0x4, 0x2, 0x4, 0x8, 0x2, 0x12,}, /* LARB28 */
	{0x30, 0x10, 0xe, 0x14, 0x6, 0x2, 0x6,}, /* LARB29 */
	{0x30, 0x10, 0xe, 0x14, 0x6, 0x2, 0x6,}, /* LARB30 */
};

static u8
mtk_smi_larb_mt6879_cmd_group[MTK_LARB_NR_MAX][2] = {
	{2, 3}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
};

static u8
mtk_smi_larb_mt6879_bw_thrt_en[MTK_LARB_NR_MAX][2] = {
	{4, 6}, {7, 8},
	{0, 6}, {0, 0},
	{0, 14}, {0, 0}, {0, 0},
	{0, 15}, {0, 0},
	{0, 25}, {0, 20}, {0, 30}, {0, 10},
	{0, 0}, {0, 0},
	{0, 19}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, /*20*/
	{0, 0}, {0, 30}, {0, 30}, {0, 0},
	{0, 0}, {0, 0}, {0, 0},/*27*/
	{0, 0}, {0, 0}, {0, 0}, {0, 0},
};

static u8
mtk_smi_larb_mt6879_bwl[MTK_LARB_NR_MAX][SMI_LARB_PORT_NR_MAX] = {
	{0x2, 0x4, 0x3e, 0xc, 0x6, 0x1,}, /* LARB0 */
	{0x2, 0x20, 0x2, 0x4, 0x2, 0xc, 0x4, 0x1,}, /* LARB1 */
	{0x2, 0x2, 0x4, 0x4, 0x1, 0x1,}, /* LARB2 */
	{}, /* LARB3 */
	{0x2f, 0x28, 0xc, 0x1, 0x1, 0x1, 0x1, 0x2, 0x2, 0x5, 0x1,
	 0x17, 0x2, 0x4,}, /* LARB4 */
	{}, /* LARB5 */
	{}, /* LARB6 */
	{0x1, 0x3, 0x2, 0x1, 0x1, 0x1, 0x4, 0x4, 0x1, 0x3,
	 0x4, 0x4, 0x2, 0x16, 0x17,}, /* LARB7 */
	{}, /* LARB8 */
	{0x13, 0x1, 0x10, 0x8, 0x3, 0x5, 0x1, 0x1, 0x10, 0x10, 0x10,
	0x1f, 0x30, 0x2, 0x13, 0x10, 0x8, 0x3, 0x2, 0x7, 0x5,
	0x1, 0x30, 0x2, 0x7,}, /* LARB9 */
	{0x2f, 0x1, 0x8, 0x8, 0x1, 0x1, 0x4, 0x18, 0x18, 0x19,
	 0xb, 0x1, 0x10, 0x9, 0x4, 0x4, 0x15, 0x1, 0x6, 0x40,}, /* LARB10 */
	{0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1f, 0x10, 0x10, 0x13,
	 0x10, 0x8, 0x3, 0x5, 0x3, 0x1f, 0x1, 0x10, 0x1, 0x1,
	 0x10, 0x1, 0x1, 0x1f, 0x18, 0x2, 0x7, 0x5, 0x3, 0x40,}, /* LARB11 */
	{0x5, 0x5, 0x3, 0x3, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,}, /* LARB12 */
	{0x2, 0x2, 0xa, 0xa, 0xa, 0xa, 0x4, 0x4, 0x2, 0x2,
	 0x2, 0x2, 0xa, 0xa, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2,
	 0x2, 0x2, 0x2, 0x2,}, /* LARB13 */
	{0xa, 0xa, 0x2, 0x2, 0x2, 0x2, 0x4, 0x4, 0xa, 0xa,
	 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x8,
	 0x8, 0x1, 0x1,}, /* LARB14 */
	{0x1f, 0x1f, 0x4, 0x1, 0x1, 0x4, 0x10, 0x10, 0x10, 0x14,
	 0x4, 0x7, 0x15, 0x1, 0x1, 0x7, 0x9, 0x14, 0x1,}, /* LARB15 */
	{0x1e, 0x2, 0x2, 0x8, 0x2, 0x8, 0x6, 0x2, 0x2, 0x4, 0x8,
	 0x4, 0x2, 0x4, 0x8, 0x2, 0x12,}, /* LARB16 */
	{0x30, 0x10, 0xe, 0x14, 0x6, 0x2, 0x6,}, /* LARB17 */
	{}, /* LARB18 */
	{0x2, 0x2, 0x2, 0x2,}, /* LARB19 */
	{}, /* LARB20 */
	{}, /* LARB21 */
	{0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1f, 0x10, 0x10, 0x13,
	 0x10, 0x8, 0x3, 0x5, 0x3, 0x1f, 0x1, 0x10, 0x1, 0x1,
	 0x10, 0x1, 0x1, 0x1f, 0x18, 0x2, 0x7, 0x5, 0x3, 0x40,}, /* LARB22 */
	{0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1f, 0x10, 0x10, 0x13,
	 0x10, 0x8, 0x3, 0x5, 0x3, 0x1f, 0x1, 0x10, 0x1, 0x1,
	 0x10, 0x1, 0x1, 0x1f, 0x18, 0x2, 0x7, 0x5, 0x3, 0x40,}, /* LARB23 */
	{}, /* LARB24 */
	{0x2, 0x2, 0x2, 0x8, 0x4, 0x2, 0x2, 0x2, 0x8, 0x4, 0x1,}, /* LARB25 */
	{0x2, 0x2, 0x2, 0x8, 0x4, 0x1, 0x1, 0x1, 0x1, 0x1, 0x2,}, /* LARB26 */
	{0x1e, 0x2, 0x2, 0x8, 0x2, 0x8, 0x6, 0x2, 0x2, 0x4, 0x8,
	 0x4, 0x2, 0x4, 0x8, 0x2, 0x12,}, /* LARB27 */
	{}, /* LARB28 */
	{0x30, 0x10, 0xe, 0x14, 0x6, 0x2, 0x6,}, /* LARB29 */
};

static struct mtk_smi_reg_pair
mtk_smi_larb_mt6873_misc[MTK_LARB_NR_MAX][SMI_LARB_MISC_NR] = {
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_FORCE_ULTRA, 0x8000},
	{SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_FORCE_ULTRA, 0x8000},
	{SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_FORCE_ULTRA, 0x8000},
	{SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
};

static struct mtk_smi_reg_pair
mtk_smi_larb_mt6853_misc[MTK_LARB_NR_MAX][SMI_LARB_MISC_NR] = {
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON,  0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256},  {SMI_LARB_FORCE_ULTRA, 0x8000},
		{SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256},  {SMI_LARB_FORCE_ULTRA, 0x8000},
		{SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256},  {SMI_LARB_FORCE_ULTRA, 0x8000},
		{SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
};

static struct mtk_smi_reg_pair
mtk_smi_larb_mt6893_misc[MTK_LARB_NR_MAX][SMI_LARB_MISC_NR] = {
	{{SMI_LARB_VC_PRI_MODE, 0x1}, {SMI_LARB_CMD_THRT_CON, 0x370256},
	{SMI_LARB_SW_FLAG, 0x1}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},},
	{{SMI_LARB_VC_PRI_MODE, 0x1}, {SMI_LARB_CMD_THRT_CON, 0x370256},
	{SMI_LARB_SW_FLAG, 0x1}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},
	{INT_SMI_LARB_CMD_THRT_CON, 0x370256},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},
	{INT_SMI_LARB_CMD_THRT_CON, 0x370256},},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},
	{INT_SMI_LARB_CMD_THRT_CON, 0x300256},},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},
	{INT_SMI_LARB_CMD_THRT_CON, 0x300256},},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},
	{SMI_LARB_DBG_CON, 0x1}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	{INT_SMI_LARB_DBG_CON, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},
	{SMI_LARB_DBG_CON, 0x1}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	{INT_SMI_LARB_DBG_CON, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},
	{SMI_LARB_FORCE_ULTRA, 0x8000}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},
	{SMI_LARB_FORCE_ULTRA, 0x8000}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},
	{SMI_LARB_FORCE_ULTRA, 0x8000}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
};

static struct mtk_smi_reg_pair
mtk_smi_larb_mt6983_misc[MTK_LARB_NR_MAX][SMI_LARB_MISC_NR] = {
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
};


static struct mtk_smi_reg_pair
mtk_smi_larb_mt6879_misc[MTK_LARB_NR_MAX][SMI_LARB_MISC_NR] = {
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {INT_SMI_LARB_CMD_THRT_CON, 0x370256},
	 {SMI_LARB_SW_FLAG, 0x1},},
};

static struct mtk_smi_reg_pair
mtk_smi_larb_mt8195_misc[MTK_LARB_NR_MAX][SMI_LARB_MISC_NR] = {
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
};

static u8
mtk_smi_larb_mt8195_bwl[MTK_LARB_NR_MAX][SMI_LARB_PORT_NR_MAX] = {
	{0x0a, 0xc, 0x22, 0x22, 0x01, 0x0a,},
	{0x0a, 0xc, 0x22, 0x22, 0x01, 0x0a,},
	{0x12, 0x12, 0x12, 0x12, 0x0a,},
	{0x12, 0x12, 0x12, 0x12, 0x28, 0x28, 0x0a,},
	{0x06, 0x01, 0x17, 0x06, 0x0a,},
	{0x06, 0x01, 0x17, 0x06, 0x06, 0x01, 0x06, 0x0a,},
	{0x06, 0x01, 0x06, 0x0a,},
	{0x0c, 0x0c, 0x12,},
	{0x0c, 0x0c, 0x12,},
	{0x0a, 0x08, 0x04, 0x06, 0x01, 0x01, 0x10, 0x18, 0x11, 0x0a,
	 0x08, 0x04, 0x11, 0x06, 0x02, 0x06, 0x01, 0x11, 0x11, 0x06,},
	{0x18, 0x08, 0x01, 0x01, 0x20, 0x12, 0x18, 0x06, 0x05, 0x10,
	 0x08, 0x08, 0x10, 0x08, 0x08, 0x18, 0x0c, 0x09, 0x0b, 0x0d,
	 0x0d, 0x06, 0x10, 0x10,},
	{0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x01, 0x01, 0x01, 0x01,},
	{0x09, 0x09, 0x05, 0x05, 0x0c, 0x18, 0x02, 0x02, 0x04, 0x02,},
	{0x02, 0x02, 0x12, 0x12, 0x02, 0x02, 0x02, 0x02, 0x08, 0x01,},
	{0x12, 0x12, 0x02, 0x02, 0x02, 0x02, 0x16, 0x01, 0x16, 0x01,
	 0x01, 0x02, 0x02, 0x08, 0x02,},
	{},
	{0x28, 0x02, 0x02, 0x12, 0x02, 0x12, 0x10, 0x02, 0x02, 0x0a,
	 0x12, 0x02, 0x0a, 0x16, 0x02, 0x04,},
	{0x1a, 0x0e, 0x0a, 0x0a, 0x0c, 0x0e, 0x10,},
	{0x12, 0x06, 0x12, 0x06,},
	{0x01, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x04, 0x01,
	 0x01, 0x01, 0x04, 0x0a, 0x06, 0x01, 0x01, 0x01, 0x0a, 0x06,
	 0x01, 0x01, 0x05, 0x03, 0x03, 0x04, 0x01,},
	{0x01, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x04, 0x01,
	 0x01, 0x01, 0x04, 0x0a, 0x06, 0x01, 0x01, 0x01, 0x0a, 0x06,
	 0x01, 0x01, 0x05, 0x03, 0x03, 0x04, 0x01,},
	{0x28, 0x19, 0x0c, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04,},
	{0x28, 0x19, 0x0c, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04,},
	{0x18, 0x01,},
	{0x01, 0x01, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x01,
	 0x01, 0x01,},
	{0x02, 0x02, 0x02, 0x28, 0x16, 0x02, 0x02, 0x02, 0x12, 0x16,
	 0x02, 0x01,},
	{0x02, 0x02, 0x02, 0x28, 0x16, 0x02, 0x02, 0x02, 0x12, 0x16,
	 0x02, 0x01,},
	{0x02, 0x02, 0x02, 0x28, 0x16, 0x02, 0x02, 0x02, 0x12, 0x16,
	 0x02, 0x01,},
	{0x1a, 0x0e, 0x0a, 0x0a, 0x0c, 0x0e, 0x10,},
};

static struct mtk_smi_reg_pair
mtk_smi_larb_mt8188_misc[MTK_LARB_NR_MAX][SMI_LARB_MISC_NR] = {
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
};

static u8
mtk_smi_larb_mt8188_bwl[MTK_LARB_NR_MAX][SMI_LARB_PORT_NR_MAX] = {
	/* larb name:port num */
	/* larb0:7 */
	{0x02, 0x18, 0x22, 0x22, 0x01, 0x02, 0x0a,},
	/* larb1:7 */
	{0x12, 0x02, 0x14, 0x14, 0x01, 0x18, 0x0a,},
	/* larb2:5 */
	{0x12, 0x12, 0x12, 0x12, 0x0a,},
	/* larb3:7 */
	{0x12, 0x12, 0x12, 0x12, 0x28, 0x28, 0x0a,},
	/* larb4:7 */
	{0x06, 0x01, 0x17, 0x06, 0x0a, 0x07, 0x07,},
	/* larb5:8 */
	{0x02, 0x01, 0x04, 0x02, 0x06, 0x01, 0x06, 0x0a,},
	/* larb6:4 */
	{0x06, 0x01, 0x06, 0x0a,},
	/* larb7:3 */
	{0x0c, 0x0c, 0x12,},
	/* larb9:25 */
	{0x0c, 0x01, 0x0a, 0x05, 0x02, 0x03, 0x01, 0x01, 0x14, 0x14,
	 0x0a, 0x14, 0x1e, 0x01, 0x0c, 0x0a, 0x05, 0x02, 0x02, 0x05,
	 0x03, 0x01, 0x1e, 0x01, 0x05,},
	/* larb10:20 */
	{0x1e, 0x01, 0x0a, 0x0a, 0x01, 0x01, 0x03, 0x1e, 0x1e, 0x10,
	 0x07, 0x01, 0x0a, 0x06, 0x03, 0x03, 0x0e, 0x01, 0x04, 0x28,},
	/* larb11A:30 */
	{0x03, 0x20, 0x01, 0x20, 0x01, 0x01, 0x14, 0x0a, 0x0a, 0x0c,
	 0x0a, 0x05, 0x02, 0x03, 0x02, 0x14, 0x0a, 0x0a, 0x14, 0x14,
	 0x14, 0x01, 0x01, 0x14, 0x1e, 0x01, 0x05, 0x03, 0x02, 0x28,},
	/* larb11B:30 */
	{0x03, 0x20, 0x01, 0x20, 0x01, 0x01, 0x14, 0x0a, 0x0a, 0x0c,
	 0x0a, 0x05, 0x02, 0x03, 0x02, 0x14, 0x0a, 0x0a, 0x14, 0x14,
	 0x14, 0x01, 0x01, 0x14, 0x1e, 0x01, 0x05, 0x03, 0x02, 0x28,},
	/* larb11C:30 */
	{0x03, 0x20, 0x01, 0x20, 0x01, 0x01, 0x14, 0x0a, 0x0a, 0x0c,
	 0x0a, 0x05, 0x02, 0x03, 0x02, 0x14, 0x0a, 0x0a, 0x14, 0x14,
	 0x14, 0x01, 0x01, 0x14, 0x1e, 0x01, 0x05, 0x03, 0x02, 0x28,},
	/* larb12:16 */
	{0x07, 0x02, 0x04, 0x02, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	 0x07, 0x02, 0x04, 0x02, 0x05, 0x05,},
	/* larb13:24 */
	{0x02, 0x02, 0x0c, 0x0c, 0x0c, 0x0c, 0x01, 0x01, 0x02, 0x02,
	 0x02, 0x02, 0x0c, 0x0c, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	 0x02, 0x02, 0x01, 0x01,},
	/* larb14:23 */
	{0x0c, 0x0c, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x0c, 0x0c,
	 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x02,
	 0x0c, 0x01, 0x01,},
	/* larb15:19 */
	{0x28, 0x28, 0x03, 0x01, 0x01, 0x03, 0x14, 0x14, 0x0a, 0x0d,
	 0x03, 0x05, 0x0e, 0x01, 0x01, 0x05, 0x06, 0x0d, 0x01,},
	/* larb16A:17 */
	{0x28, 0x02, 0x02, 0x12, 0x02, 0x12, 0x10, 0x02, 0x02, 0x0a,
	 0x12, 0x02, 0x02, 0x0a, 0x16, 0x02, 0x04,},
	/* larb16B:17 */
	{0x28, 0x02, 0x02, 0x12, 0x02, 0x12, 0x10, 0x02, 0x02, 0x0a,
	 0x12, 0x02, 0x02, 0x0a, 0x16, 0x02, 0x04,},
	/* larb17A:7 */
	{0x1a, 0x0e, 0x0a, 0x0a, 0x0c, 0x0e, 0x10,},
	/* larb17B:7 */
	{0x1a, 0x0e, 0x0a, 0x0a, 0x0c, 0x0e, 0x10,},
	/* larb19:27 */
	{0x01, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x04, 0x01,
	 0x01, 0x01, 0x04, 0x0a, 0x06, 0x01, 0x01, 0x01, 0x0a, 0x06,
	 0x01, 0x01, 0x05, 0x03, 0x03, 0x04, 0x01,},
	/* larb21:11 */
	{0x28, 0x19, 0x0c, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04,
	 0x01,},
	/* larb23:9 */
	{0x01, 0x01, 0x04, 0x01, 0x01, 0x01, 0x18, 0x01, 0x01,},
	/* larb27:4 */
	{0x12, 0x06, 0x12, 0x06,},
	/* larb28:0 */
	{},
};

static u32 mtk_smi_larb_sram_port[] = {
	BIT_ULL(5) | BIT_ULL(6) | BIT_ULL(11) | BIT_ULL(15) | BIT_ULL(16) |
	BIT_ULL(17),
	BIT_ULL(5) | BIT_ULL(6) | BIT_ULL(11) | BIT_ULL(15) | BIT_ULL(16) |
	BIT_ULL(17),
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt8173 = {
	/* mt8173 do not need the port in larb */
	.config_port = mtk_smi_larb_config_port_mt8173,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt8167 = {
	/* mt8167 do not need the port in larb */
	.config_port = mtk_smi_larb_config_port_mt8167,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt2701 = {
	.port_in_larb = {
		LARB0_PORT_OFFSET, LARB1_PORT_OFFSET,
		LARB2_PORT_OFFSET, LARB3_PORT_OFFSET
	},
	.config_port = mtk_smi_larb_config_port_gen1,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt2712 = {
	.config_port                = mtk_smi_larb_config_port_gen2_general,
	.larb_direct_to_common_mask = BIT(8) | BIT(9),      /* bdpsys */
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt6779 = {
	.config_port  = mtk_smi_larb_config_port_gen2_general,
	.larb_direct_to_common_mask =
		BIT(4) | BIT(6) | BIT(11) | BIT(12) | BIT(13),
		/* DUMMY | IPU0 | IPU1 | CCU | MDLA */
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt6873 = {
	.port_in_larb_gen2 = {6, 8, 5, 0, 11, 8, 0, 15, 0, 29, 0, 29,
			      0, 12, 6, 0, 17, 17, 17, 4, 6,},
	.config_port                = mtk_smi_larb_config_port_gen2_general,
	.larb_direct_to_common_mask = BIT(3) | BIT(6) | BIT(8) |
				      BIT(10) | BIT(12) | BIT(15) | BIT(21) |
					  BIT(22),
				      /*skip larb: 3,6,8,10,12,15,21,22*/
	.has_bwl                    = true,
	.bwl                        = (u8 *)mtk_smi_larb_mt6873_bwl,
	.misc = (struct mtk_smi_reg_pair *)mtk_smi_larb_mt6873_misc,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt6853 = {
	.port_in_larb_gen2 = {4, 5, 5, 0, 12, 0, 0, 13, 0, 29,
				0, 29, 0, 12, 6, 5, 17, 17, 17, 4, 6,},
	.config_port                = mtk_smi_larb_config_port_gen2_general,
	.larb_direct_to_common_mask = BIT(3) | BIT(5) | BIT(6) | BIT(8) |
				      BIT(10) | BIT(12) | BIT(15) | BIT(18) | BIT(21) |
					  BIT(22),
				      /*skip larb: 3,6,8,10,12,15,21,22*/
	.has_bwl                    = true,
	.bwl                        = (u8 *)mtk_smi_larb_mt6853_bwl,
	.misc = (struct mtk_smi_reg_pair *)mtk_smi_larb_mt6853_misc,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt6893 = {
	.port_in_larb_gen2 = {15, 15, 6, 6, 11, 8, 0, 27, 27, 29, 0, 29,
			     0, 12, 6, 5, 17, 17, 17, 4, 6,},
	.config_port                = mtk_smi_larb_config_port_gen2_general,
	.larb_direct_to_common_mask = BIT(6) | BIT(10) | BIT(12),
				      /*skip larb: 6,10,12*/
	.has_bwl                    = true,
	.has_grouping               = true,
	.has_bw_thrt                = true,
	.bwl                        = (u8 *)mtk_smi_larb_mt6893_bwl,
	.cmd_group                  = (u8 *)mtk_smi_larb_mt6893_cmd_group,
	.bw_thrt_en                 = (u8 *)mtk_smi_larb_mt6893_bw_thrt_en,
	.misc = (struct mtk_smi_reg_pair *)mtk_smi_larb_mt6893_misc,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt6983 = {
	.port_in_larb_gen2 = {16, 8, 9, 9, 11, 9, 4, 31, 31, 25,
				 20, 30, 16, 24, 23, 19, 17, 7, 8, 4, 16, 8,
				 30, 30, 0, 14, 14, 17, 17, 7, 7,},
	.config_port                = mtk_smi_larb_config_port_gen2_general,
	.larb_direct_to_common_mask = BIT(24),
				      /*skip larb: 24*/
	.has_bwl                    = true,
	.has_grouping               = true,
	.has_bw_thrt                = true,
	.bwl                        = (u8 *)mtk_smi_larb_mt6983_bwl,
	.misc = (struct mtk_smi_reg_pair *)mtk_smi_larb_mt6983_misc,
	.cmd_group                  = (u8 *)mtk_smi_larb_mt6983_cmd_group,
	.bw_thrt_en                 = (u8 *)mtk_smi_larb_mt6983_bw_thrt_en,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt6879 = {
	.port_in_larb_gen2 = {6, 8, 6, 0, 14, 0, 0, 15, 0, 25,
				 20, 30, 10, 24, 23, 19, 17, 7, 0, 4, 0, 0,
				 30, 30, 0, 11, 11, 17, 0, 7,},
	.config_port                = mtk_smi_larb_config_port_gen2_general,
	.larb_direct_to_common_mask = BIT(3) | BIT(5) | BIT(6) | BIT(8) |
					BIT(18) | BIT(20) | BIT(21) | BIT(24) | BIT(28),
				      /*skip larb: 3,5,6,8,18,20,21,24,28*/
	.has_bwl                    = true,
	.has_grouping               = true,
	.has_bw_thrt                = true,
	.bwl                        = (u8 *)mtk_smi_larb_mt6879_bwl,
	.misc = (struct mtk_smi_reg_pair *)mtk_smi_larb_mt6879_misc,
	.cmd_group                  = (u8 *)mtk_smi_larb_mt6879_cmd_group,
	.bw_thrt_en                 = (u8 *)mtk_smi_larb_mt6879_bw_thrt_en,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt8183 = {
	.has_gals                   = true,
	.config_port                = mtk_smi_larb_config_port_gen2_general,
	.larb_direct_to_common_mask = BIT(2) | BIT(3) | BIT(7),
				      /* IPU0 | IPU1 | CCU */
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt8188 = {
	.port_in_larb_gen2 = {7, 7, 5, 7, 7, 8, 4, 3, 25, 20,	/* L0~10 */
			      30, 30, 30, 16, 24, 23, 19, 17, 17, /* L11A~16B */
			      7, 7, 27, 11, 9, 4, 0}, /* L17A~28(fake) */
	.config_port                = mtk_smi_larb_config_port_mt8188,
	.has_bwl                    = true,
	.bwl                        = (u8 *)mtk_smi_larb_mt8188_bwl,
	.misc = (struct mtk_smi_reg_pair *)mtk_smi_larb_mt8188_misc,
	.sleep_ctrl = mtk_smi_larb_sleep_ctrl,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt8192 = {
	.config_port                = mtk_smi_larb_config_port_gen2_general,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt8195 = {
	.has_gals                   = true,
	.port_in_larb_gen2 = {6, 6, 5, 7, 5, 8, 4, 3, 3, 20,
			      24, 10, 10, 10, 15, 0, 16, 7, 4, 27,
			      27, 10, 10, 2, 12, 12, 12, 16, 7,},
	.config_port                = mtk_smi_larb_config_port_mt8195,
	.larb_direct_to_common_mask = BIT(29) | BIT(30),
	.sram_larb		    = BIT(19) | BIT(20),
	.sram_port		    = mtk_smi_larb_sram_port,
	.has_bwl                    = true,
	.bwl                        = (u8 *)mtk_smi_larb_mt8195_bwl,
	.misc = (struct mtk_smi_reg_pair *)mtk_smi_larb_mt8195_misc,
	.sleep_ctrl = mtk_smi_larb_sleep_ctrl,
};

static const struct of_device_id mtk_smi_larb_of_ids[] = {
	{
		.compatible = "mediatek,mt8167-smi-larb",
		.data = &mtk_smi_larb_mt8167
	},
	{
		.compatible = "mediatek,mt8173-smi-larb",
		.data = &mtk_smi_larb_mt8173
	},
	{
		.compatible = "mediatek,mt2701-smi-larb",
		.data = &mtk_smi_larb_mt2701
	},
	{
		.compatible = "mediatek,mt6873-smi-larb",
		.data = &mtk_smi_larb_mt6873
	},
	{
		.compatible = "mediatek,mt2712-smi-larb",
		.data = &mtk_smi_larb_mt2712
	},
	{
		.compatible = "mediatek,mt6779-smi-larb",
		.data = &mtk_smi_larb_mt6779
	},
	{
		.compatible = "mediatek,mt8183-smi-larb",
		.data = &mtk_smi_larb_mt8183
	},
	{
		.compatible = "mediatek,mt6853-smi-larb",
		.data = &mtk_smi_larb_mt6853
	},
	{
		.compatible = "mediatek,mt6893-smi-larb",
		.data = &mtk_smi_larb_mt6893
	},
	{
		.compatible = "mediatek,mt6983-smi-larb",
		.data = &mtk_smi_larb_mt6983
	},
	{
		.compatible = "mediatek,mt6879-smi-larb",
		.data = &mtk_smi_larb_mt6879
	},
	{
		.compatible = "mediatek,mt8188-smi-larb",
		.data = &mtk_smi_larb_mt8188
	},
	{
		.compatible = "mediatek,mt8192-smi-larb",
		.data = &mtk_smi_larb_mt8192
	},
	{
		.compatible = "mediatek,mt8195-smi-larb",
		.data = &mtk_smi_larb_mt8195
	},
	{}
};

static s32 smi_cmdq(void *data)
{
	struct device *dev = (struct device *)data;

	struct device_node *node = NULL;
	struct platform_device *pdev = NULL;

	node = of_parse_phandle(dev->of_node, "mediatek,cmdq", 0);
	if (node) {
		pdev = of_find_device_by_node(node);
		of_node_put(node);
		if (pdev) {
			while (!platform_get_drvdata(pdev)) {
				dev_notice(dev, "Failed to get [cmdq]\n");
				msleep(100);
			}
			if (device_link_add(dev, &pdev->dev,
				DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS))
				dev_notice(dev, "Success to link [cmdq]\n");
			else
				dev_notice(dev, "Failed to link [cmdq]\n");
		} else
			dev_notice(dev, "Failed to get [cmdq] pdev\n");
	} else
		dev_notice(dev, "Failed to get [cmdq] node\n");
	return 0;
}

static int mtk_smi_larb_probe(struct platform_device *pdev)
{
	struct mtk_smi_larb *larb;
	struct resource *res;
	struct device *dev = &pdev->dev;
	struct device_node *smi_node;
	struct platform_device *smi_pdev;
	struct device_link *link;
	struct mtk_smi *common;
	int ret;

	larb = devm_kzalloc(dev, sizeof(*larb), GFP_KERNEL);
	if (!larb)
		return -ENOMEM;

	larb->larb_gen = of_device_get_match_data(dev);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	larb->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(larb->base))
		return PTR_ERR(larb->base);

	larb->smi.clk_apb = devm_clk_get(dev, "apb");
	if (IS_ERR(larb->smi.clk_apb))
		return PTR_ERR(larb->smi.clk_apb);

	larb->smi.clk_smi = devm_clk_get(dev, "smi");
	if (IS_ERR(larb->smi.clk_smi))
		return PTR_ERR(larb->smi.clk_smi);

	if (larb->larb_gen->has_gals) {
		/* The larbs may still haven't gals even if the SoC support.*/
		larb->smi.clk_gals0 = devm_clk_get(dev, "gals");
		if (PTR_ERR(larb->smi.clk_gals0) == -ENOENT)
			larb->smi.clk_gals0 = NULL;
		else if (IS_ERR(larb->smi.clk_gals0))
			return PTR_ERR(larb->smi.clk_gals0);

		larb->smi.clk_gals1 = devm_clk_get(dev, "gals1");
		if (PTR_ERR(larb->smi.clk_gals1) == -ENOENT)
			larb->smi.clk_gals1 = NULL;
		else if (IS_ERR(larb->smi.clk_gals1))
			return PTR_ERR(larb->smi.clk_gals1);

		larb->smi.clk_gals2 = devm_clk_get(dev, "gals2");
		if (PTR_ERR(larb->smi.clk_gals2) == -ENOENT)
			larb->smi.clk_gals2 = NULL;
		else if (IS_ERR(larb->smi.clk_gals2))
			return PTR_ERR(larb->smi.clk_gals2);
	}
	larb->smi.dev = dev;

	smi_node = of_parse_phandle(dev->of_node, "mediatek,smi", 0);
	if (!smi_node)
		return -EINVAL;

	smi_pdev = of_find_device_by_node(smi_node);
	of_node_put(smi_node);
	if (smi_pdev) {
		common = platform_get_drvdata(smi_pdev);
		if (!common)
			return -EPROBE_DEFER;
		larb->smi_common_dev = &smi_pdev->dev;
		link = device_link_add(dev, larb->smi_common_dev,
				       DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);
		if (!link) {
			dev_notice(dev, "Unable to link smi_common device\n");
			return -ENODEV;
		}
	} else {
		dev_err(dev, "Failed to get the smi_common device\n");
		return -EINVAL;
	}

	smi_node = of_parse_phandle(dev->of_node, "mediatek,smi", 1);
	if (smi_node) {
		smi_pdev = of_find_device_by_node(smi_node);
		of_node_put(smi_node);
		if (smi_pdev) {
			if (!platform_get_drvdata(smi_pdev))
				return -EPROBE_DEFER;
			link = device_link_add(dev, &smi_pdev->dev,
					       DL_FLAG_PM_RUNTIME |
					       DL_FLAG_STATELESS);
			if (!link) {
				dev_notice(dev,
					"Unable to link sram smi-common dev\n");
				return -ENODEV;
			}
		} else {
			dev_notice(dev, "Failed to get sram smi_common device\n");
			return -EINVAL;
		}
	}

	pm_runtime_enable(dev);
	platform_set_drvdata(pdev, larb);
	ret = component_add(dev, &mtk_smi_larb_component_ops);
	of_property_read_u32(dev->of_node, "mediatek,larb-id", &larb->smi.smi_id);

	if (of_property_read_bool(dev->of_node, "init-power-on")) {
		/* SMI init-power-on:
		 * Get sync pm runtime status for SMI LARB.
		 * And then release SMI COMMON's clock/power reference count
		 * only once time in here because it has been linked with
		 * SMI LARB.
		 */
		pm_runtime_get_sync(dev);
		larb->smi.init_power_on = true;
		if (common->init_power_on) {
			pm_runtime_put_sync(common->dev);
			common->init_power_on = false;
		}
		list_add_tail(&larb->power_on, &init_power_on_list);
	}

	return ret;
}

static int mtk_smi_larb_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);
	component_del(&pdev->dev, &mtk_smi_larb_component_ops);
	return 0;
}

static int __maybe_unused mtk_smi_larb_resume(struct device *dev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	const struct mtk_smi_larb_gen *larb_gen = larb->larb_gen;
	int ret;

	ret = mtk_smi_clk_enable(&larb->smi);
	if (ret < 0) {
		dev_err(dev, "Failed to enable clock(%d).\n", ret);
		return ret;
	}

	if (log_level_with(log_pm_control))
		dev_info(dev, "resumed\n");

	if (larb_gen->sleep_ctrl)
		larb_gen->sleep_ctrl(dev, false);

	/* Configure the basic setting for this larb */
	larb_gen->config_port(dev);

	return 0;
}

static int __maybe_unused mtk_smi_larb_suspend(struct device *dev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	const struct mtk_smi_larb_gen *larb_gen = larb->larb_gen;

	if (larb_gen->sleep_ctrl)
		larb_gen->sleep_ctrl(dev, true);

	mtk_smi_clk_disable(&larb->smi);

	if (log_level_with(log_pm_control))
		dev_info(dev, "suspended\n");

	return 0;
}

static const struct dev_pm_ops smi_larb_pm_ops = {
	SET_RUNTIME_PM_OPS(mtk_smi_larb_suspend, mtk_smi_larb_resume, NULL)
	SET_LATE_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				     pm_runtime_force_resume)
};

static struct platform_driver mtk_smi_larb_driver = {
	.probe	= mtk_smi_larb_probe,
	.remove	= mtk_smi_larb_remove,
	.driver	= {
		.name = "mtk-smi-larb",
		.of_match_table = mtk_smi_larb_of_ids,
		.pm             = &smi_larb_pm_ops,
	}
};

static u16 mtk_smi_common_mt6873_bwl[SMI_COMMON_LARB_NR_MAX] = {
	0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000,
};

static u16 mtk_smi_common_mt6853_bwl[SMI_COMMON_LARB_NR_MAX] = {
	0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000,
};

static u16 mtk_smi_common_mt6893_bwl[MTK_COMMON_NR_MAX][SMI_COMMON_LARB_NR_MAX] = {
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
};

static u16 mtk_smi_common_mt6983_bwl[MTK_COMMON_NR_MAX][SMI_COMMON_LARB_NR_MAX] = {
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
};

static u16 mtk_smi_common_mt6879_bwl[MTK_COMMON_NR_MAX][SMI_COMMON_LARB_NR_MAX] = {
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
};

static struct mtk_smi_reg_pair
mtk_smi_common_mt6873_misc[SMI_COMMON_MISC_NR] = {
	{SMI_L1LEN, 0xb},
	{SMI_M4U_TH, 0xe100e10},
	{SMI_FIFO_TH1, 0x90a090a},
	{SMI_FIFO_TH2, 0x506090a},
	{SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},
};

static struct mtk_smi_reg_pair
mtk_smi_common_mt6853_misc[SMI_COMMON_MISC_NR] = {
	{SMI_L1LEN, 0xb},
	{SMI_M4U_TH, 0xe100e10},
	{SMI_FIFO_TH1, 0x9100910},
	{SMI_FIFO_TH2, 0x5060910},
	{SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},
};

static struct mtk_smi_reg_pair
mtk_smi_common_mt6893_misc[MTK_COMMON_NR_MAX][SMI_COMMON_MISC_NR] = {
	{{SMI_L1LEN, 0xb}, {SMI_BUS_SEL, 0x4514}, {SMI_M4U_TH, 0xe100e10},
	{SMI_FIFO_TH1, 0x506090a}, {SMI_FIFO_TH2, 0x506090a}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},
	{{SMI_L1LEN, 0x2}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},
	{{SMI_L1LEN, 0xb}, {SMI_BUS_SEL, 0x4514}, {SMI_M4U_TH, 0xe100e10},
	{SMI_FIFO_TH1, 0x506090a}, {SMI_FIFO_TH2, 0x506090a}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},
	{{SMI_L1LEN, 0x2}, {SMI_BUS_SEL, 0x1111}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},
};

static struct mtk_smi_reg_pair
mtk_smi_common_mt6983_misc[MTK_COMMON_NR_MAX][SMI_COMMON_MISC_NR] = {
	{{SMI_L1LEN, 0xb}, {SMI_BUS_SEL, 0x454}, {SMI_M4U_TH, 0xe100e10},
	{SMI_FIFO_TH1, 0x506090a}, {SMI_FIFO_TH2, 0x506090a}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},}, /* SMI_DISP_COMMON */
	{{SMI_L1LEN, 0xb}, {SMI_BUS_SEL, 0x4405}, {SMI_M4U_TH, 0xe100e10},
	{SMI_FIFO_TH1, 0x506090a}, {SMI_FIFO_TH2, 0x506090a}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},}, /* SMI_MDP_COMMON */
	{{SMI_L1LEN, 0x2}, {SMI_BUS_SEL, 0x4444}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},/* SMI_SYSRAM_COMMON */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},/* SMI_DISP_SUB_COMMON0 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},/* SMI_DISP_SUB_COMMON1 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},/* SMI_MDP_SUB_COMMON0 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},/* SMI_MDP_SUB_COMMON1 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},/* SMI_SYS_SUB_COMMON2 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},/* SMI_SYS_SUB_COMMON3 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},/* SMI_SYS_SUB_COMMON4 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},/* SMI_DISP0_SUB_COMMON0 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},/* SMI_DISP1_SUB_COMMON0 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},/* SMI_CAM_MM_SUB_COMMON0 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},/* SMI_CAM_SYS_SUB_COMMON0 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},/* SMI_CAM_MDP_SUB_COMMON0 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},/* SMI_IMG_SUB_COMMON0 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},},/* SMI_IMG_SUB_COMMON1 */
};

static struct mtk_smi_reg_pair
mtk_smi_common_mt6879_misc[MTK_COMMON_NR_MAX][SMI_COMMON_MISC_NR] = {
	{{SMI_L1LEN, 0xb}, {SMI_BUS_SEL, 0x1044}, {SMI_M4U_TH, 0xe100e10},
	 {SMI_FIFO_TH1, 0x506090a}, {SMI_FIFO_TH2, 0x506090a}, {SMI_DCM, 0x4f1},
	 {SMI_DUMMY, 0x1},},/* 0:SMI_DISP_COMMON */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DUMMY, 0x1},},
	/* 1:smi_infra_disp_subcommon0 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DUMMY, 0x1},},
	/* 2:smi_infra_disp_subcommon1 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DUMMY, 0x1},},
	/* 3:smi_mdp_subcommon0 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DUMMY, 0x1},},
	/* 4:smi_mdp_subcommon1 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DUMMY, 0x1},},
	/* 5:smi_img_subcommon0 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DUMMY, 0x1},},
	/* 6:smi_img_subcommon1 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DUMMY, 0x1},},
	/* 7:smi_cam_mm_subcommon0 */
	{{SMI_L1LEN, 0xa}, {SMI_PREULTRA_MASK1, 0x2105}, {SMI_DUMMY, 0x1},},
	/* 8:smi_cam_mm_subcommon0 */
};

static struct mtk_smi_reg_pair
mtk_smi_common_mt8188_misc[MTK_COMMON_NR_MAX][SMI_COMMON_MISC_NR] = {
	{	/* vdo smi-common */
		{SMI_L1LEN, 0xb},
		{SMI_M4U_TH, 0xe100e10},
		{SMI_FIFO_TH1, 0x506090a},
		{SMI_FIFO_TH2, 0x506090a},
		{SMI_DCM, 0x4f1},
		{SMI_DUMMY, 0x1},
	},
	{	/* vpp smi-common */
		{SMI_L1LEN, 0xb},
		{SMI_M4U_TH, 0xe100e10},
		{SMI_FIFO_TH1, 0x506090a},
		{SMI_FIFO_TH2, 0x506090a},
		{SMI_DCM, 0x4f1},
		{SMI_DUMMY, 0x1},
	},
	{	/* vpp sram smi-common */
		{SMI_L1LEN, 0x2},
		{SMI_DCM, 0x4f1},
		{SMI_DUMMY, 0x1},
	},
};

static struct mtk_smi_reg_pair
mtk_smi_common_mt8195_misc[MTK_COMMON_NR_MAX][SMI_COMMON_MISC_NR] = {
	{
		{SMI_L1LEN, 0xb},
		{SMI_M4U_TH, 0xe100e10},
		{SMI_FIFO_TH1, 0x506090a},
		{SMI_FIFO_TH2, 0x506090a},
		{SMI_DCM, 0x4f1},
		{SMI_DUMMY, 0x1},
		{SMI_BUS_SEL, (F_MMU1_LARB(1) | F_MMU1_LARB(3) |
		 F_MMU1_LARB(5) | F_MMU1_LARB(7))}
	},
	{
		{SMI_L1LEN, 0xb},
		{SMI_M4U_TH, 0xe100e10},
		{SMI_FIFO_TH1, 0x506090a},
		{SMI_FIFO_TH2, 0x506090a},
		{SMI_DCM, 0x4f1},
		{SMI_DUMMY, 0x1},
		{SMI_BUS_SEL, (F_MMU1_LARB(1) | F_MMU1_LARB(2) |
		 F_MMU1_LARB(7))}
	},
	{
		{SMI_L1LEN, 0x2},
		{SMI_DCM, 0x4f1},
		{SMI_DUMMY, 0x1},
	},
};

static const struct mtk_smi_common_plat mtk_smi_common_gen1 = {
	.gen = MTK_SMI_GEN1,
};

static const struct mtk_smi_common_plat mtk_smi_common_gen2 = {
	.gen = MTK_SMI_GEN2,
};

static const struct mtk_smi_common_plat mtk_smi_common_gen3 = {
	.gen = MTK_SMI_GEN3,
};

static const struct mtk_smi_common_plat mtk_smi_common_mt6779 = {
	.gen		= MTK_SMI_GEN2,
	.has_gals	= true,
	.bus_sel	= F_MMU1_LARB(1) | F_MMU1_LARB(2) | F_MMU1_LARB(4) |
			  F_MMU1_LARB(5) | F_MMU1_LARB(6) | F_MMU1_LARB(7),
};

static const struct mtk_smi_common_plat mtk_smi_common_mt6873 = {
	.gen      = MTK_SMI_GEN2,
	.has_gals = true,
	.bus_sel  = F_MMU1_LARB(1) | F_MMU1_LARB(2) | F_MMU1_LARB(4) |
		    F_MMU1_LARB(5) | F_MMU1_LARB(7),
	.has_bwl  = true,
	.bwl      = mtk_smi_common_mt6873_bwl,
	.misc     = mtk_smi_common_mt6873_misc,
};

static const struct mtk_smi_common_plat mtk_smi_common_mt6853 = {
	.gen      = MTK_SMI_GEN2,
	.has_gals = true,
	.bus_sel  = F_MMU1_LARB(1) | F_MMU1_LARB(2) | F_MMU1_LARB(4) |
		    F_MMU1_LARB(5) | F_MMU1_LARB(7),
	.has_bwl  = true,
	.bwl      = mtk_smi_common_mt6853_bwl,
	.misc     = mtk_smi_common_mt6853_misc,
};

static const struct mtk_smi_common_plat mtk_smi_common_mt6893 = {
	.gen      = MTK_SMI_GEN3,
	.has_gals = true,
	.bus_sel  = F_MMU1_LARB(1) | F_MMU1_LARB(2) | F_MMU1_LARB(4) |
		    F_MMU1_LARB(5) | F_MMU1_LARB(7),
	.has_bwl  = true,
	.bwl      = (u16 *)mtk_smi_common_mt6893_bwl,
	.misc     = (struct mtk_smi_reg_pair *)mtk_smi_common_mt6893_misc,
};

static const struct mtk_smi_common_plat mtk_smi_common_mt6983 = {
	.gen      = MTK_SMI_GEN3,
	.has_gals = true,
	.bus_sel  = F_MMU1_LARB(1) | F_MMU1_LARB(2) | F_MMU1_LARB(4) |
		    F_MMU1_LARB(5) | F_MMU1_LARB(7),
	.has_bwl  = true,
	.bwl      = (u16 *)mtk_smi_common_mt6983_bwl,
	.misc     = (struct mtk_smi_reg_pair *)mtk_smi_common_mt6983_misc,
};

static const struct mtk_smi_common_plat mtk_smi_common_mt6879 = {
	.gen      = MTK_SMI_GEN3,
	.has_gals = true,
	.has_bwl  = true,
	.bwl      = (u16 *)mtk_smi_common_mt6879_bwl,
	.misc     = (struct mtk_smi_reg_pair *)mtk_smi_common_mt6879_misc,
};

static const struct mtk_smi_common_plat mtk_smi_common_mt8183 = {
	.gen      = MTK_SMI_GEN2,
	.has_gals = true,
	.bus_sel  = F_MMU1_LARB(1) | F_MMU1_LARB(2) | F_MMU1_LARB(5) |
		    F_MMU1_LARB(7),
};

static const struct mtk_smi_common_plat mtk_smi_common_mt8188 = {
	.gen      = MTK_SMI_GEN3,
	.bus_sel  = F_MMU1_LARB(5),
	.has_bwl  = true,
	.misc     = (struct mtk_smi_reg_pair *)mtk_smi_common_mt8188_misc,
};

static const struct mtk_smi_common_plat mtk_smi_common_mt8192 = {
	.gen      = MTK_SMI_GEN2,
	.has_gals = true,
	.bus_sel  = F_MMU1_LARB(1) | F_MMU1_LARB(2) | F_MMU1_LARB(5) |
		    F_MMU1_LARB(6),
};

static const struct mtk_smi_common_plat mtk_smi_common_mt8195 = {
	.gen      = MTK_SMI_GEN3,
	.has_gals = true,
	/* bus_sel is set with misc to distinguish vdo/vpp smi_common */
	.has_bwl  = true,
	.misc     = (struct mtk_smi_reg_pair *)mtk_smi_common_mt8195_misc,
};

static const struct of_device_id mtk_smi_common_of_ids[] = {
	{
		.compatible = "mediatek,mt8173-smi-common",
		.data = &mtk_smi_common_gen2,
	},
	{
		.compatible = "mediatek,mt8167-smi-common",
		.data = &mtk_smi_common_gen2,
	},
	{
		.compatible = "mediatek,mt2701-smi-common",
		.data = &mtk_smi_common_gen1,
	},
	{
		.compatible = "mediatek,mt2712-smi-common",
		.data = &mtk_smi_common_gen2,
	},
	{
		.compatible = "mediatek,mt6779-smi-common",
		.data = &mtk_smi_common_mt6779,
	},
	{
		.compatible = "mediatek,mt6873-smi-common",
		.data = &mtk_smi_common_mt6873,
	},
	{
		.compatible = "mediatek,mt8183-smi-common",
		.data = &mtk_smi_common_mt8183,
	},
	{
		.compatible = "mediatek,mt6853-smi-common",
		.data = &mtk_smi_common_mt6853,
	},
	{
		.compatible = "mediatek,mt6893-smi-common",
		.data = &mtk_smi_common_mt6893,
	},
	{
		.compatible = "mediatek,mt6983-smi-common",
		.data = &mtk_smi_common_mt6983,
	},
	{
		.compatible = "mediatek,mt6879-smi-common",
		.data = &mtk_smi_common_mt6879,
	},
	{
		.compatible = "mediatek,mt8188-smi-common",
		.data = &mtk_smi_common_mt8188,
	},
	{
		.compatible = "mediatek,mt8192-smi-common",
		.data = &mtk_smi_common_mt8192,
	},
	{
		.compatible = "mediatek,mt8195-smi-common",
		.data = &mtk_smi_common_mt8195,
	},
	{}
};

static int mtk_smi_common_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_smi *common;
	struct resource *res;
	struct task_struct *kthr;
	struct device_node *smi_node;
	struct platform_device *smi_pdev;
	struct device_link *link;
	int ret;

	common = devm_kzalloc(dev, sizeof(*common), GFP_KERNEL);
	if (!common)
		return -ENOMEM;
	common->dev = dev;
	common->plat = of_device_get_match_data(dev);

	common->clk_apb = devm_clk_get(dev, "apb");
	if (IS_ERR(common->clk_apb))
		return PTR_ERR(common->clk_apb);

	common->clk_smi = devm_clk_get(dev, "smi");
	if (IS_ERR(common->clk_smi))
		return PTR_ERR(common->clk_smi);

	if (common->plat->has_gals) {
		common->clk_gals0 = devm_clk_get(dev, "gals0");
		if (IS_ERR(common->clk_gals0))
			return PTR_ERR(common->clk_gals0);

		common->clk_gals1 = devm_clk_get(dev, "gals1");
		if (IS_ERR(common->clk_gals1))
			return PTR_ERR(common->clk_gals1);
	}

	common->clk_reorder = devm_clk_get(dev, "reorder");
	if (PTR_ERR(common->clk_reorder) == -ENOENT)
		common->clk_reorder = NULL;
	else if (IS_ERR(common->clk_reorder))
		return PTR_ERR(common->clk_reorder);
	/*
	 * for mtk smi gen 1, we need to get the ao(always on) base to config
	 * m4u port, and we need to enable the aync clock for transform the smi
	 * clock into emi clock domain, but for mtk smi gen2, there's no smi ao
	 * base.
	 */
	if (common->plat->gen == MTK_SMI_GEN1) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		common->smi_ao_base = devm_ioremap_resource(dev, res);
		if (IS_ERR(common->smi_ao_base))
			return PTR_ERR(common->smi_ao_base);

		common->clk_async = devm_clk_get(dev, "async");
		if (IS_ERR(common->clk_async))
			return PTR_ERR(common->clk_async);

		ret = clk_prepare_enable(common->clk_async);
		if (ret)
			return ret;
	} else {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		common->base = devm_ioremap_resource(dev, res);
		if (IS_ERR(common->base))
			return PTR_ERR(common->base);
	}

	smi_node = of_parse_phandle(dev->of_node, "mediatek,smi", 0);
	if (smi_node) {
		smi_pdev = of_find_device_by_node(smi_node);
		of_node_put(smi_node);
		if (smi_pdev) {
			if (!platform_get_drvdata(smi_pdev))
				return -EPROBE_DEFER;
			link = device_link_add(dev, &smi_pdev->dev,
					       DL_FLAG_PM_RUNTIME |
					       DL_FLAG_STATELESS);
			if (!link) {
				dev_notice(dev,
					"Unable to link sram smi-common dev\n");
				return -ENODEV;
			}
		} else {
			dev_notice(dev, "Failed to get sram smi_common device\n");
			return -EINVAL;
		}
	}

	smi_node = of_parse_phandle(dev->of_node, "mediatek,smi", 1);
	if (smi_node) {
		smi_pdev = of_find_device_by_node(smi_node);
		of_node_put(smi_node);
		if (smi_pdev) {
			if (!platform_get_drvdata(smi_pdev))
				return -EPROBE_DEFER;
			link = device_link_add(dev, &smi_pdev->dev,
					       DL_FLAG_PM_RUNTIME |
					       DL_FLAG_STATELESS);
			if (!link) {
				dev_notice(dev,
					"Unable to link sram smi-common dev\n");
				return -ENODEV;
			}
		} else {
			dev_notice(dev, "Failed to get sram smi_common device\n");
			return -EINVAL;
		}
	}

	if (of_property_read_u32(dev->of_node, "mediatek,common-id",
	    &common->smi_id))
		common->smi_id = 0;

	pm_runtime_enable(dev);
	platform_set_drvdata(pdev, common);

	if (of_parse_phandle(dev->of_node, "mediatek,cmdq", 0))
		kthr = kthread_run(smi_cmdq, dev, __func__);

	if (of_property_read_bool(dev->of_node, "init-power-on")) {
		pm_runtime_get_sync(dev);
		common->init_power_on = true;
	}

	return 0;
}

static int mtk_smi_common_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);
	return 0;
}

static int __maybe_unused mtk_smi_common_resume(struct device *dev)
{
	struct mtk_smi *common = dev_get_drvdata(dev);
	u32 bus_sel = common->plat->bus_sel;
	int i, i_base, ret;

	ret = mtk_smi_clk_enable(common);
	if (ret) {
		dev_err(common->dev, "Failed to enable clock(%d).\n", ret);
		return ret;
	}

	if (log_level_with(log_pm_control))
		dev_info(dev, "resumed\n");

	if ((common->plat->gen == MTK_SMI_GEN2 || common->plat->gen == MTK_SMI_GEN3)
		&& bus_sel)
		writel(bus_sel, common->base + SMI_BUS_SEL);
	if ((common->plat->gen != MTK_SMI_GEN2 && common->plat->gen != MTK_SMI_GEN3)
		|| !common->plat->has_bwl)
		return 0;

	if (common->plat->bwl) {
		i_base = (common->plat->gen == MTK_SMI_GEN3) ?
			 (common->smi_id * SMI_COMMON_LARB_NR_MAX) : 0;

		for (i = 0; i < SMI_COMMON_LARB_NR_MAX; i++)
			writel_relaxed(common->plat->bwl[i_base + i],
				       common->base + SMI_L1ARB(i));
	}
	if (common->plat->misc) {
		i_base = (common->plat->gen == MTK_SMI_GEN3) ?
			 (common->smi_id * SMI_COMMON_MISC_NR) : 0;

		for (i = 0; i < SMI_COMMON_MISC_NR; i++)
			writel_relaxed(common->plat->misc[i_base + i].value,
				       common->base +
				       common->plat->misc[i_base + i].offset);
	}

	wmb(); /* make sure settings are written */

	return 0;
}

static int __maybe_unused mtk_smi_common_suspend(struct device *dev)
{
	struct mtk_smi *common = dev_get_drvdata(dev);

	mtk_smi_clk_disable(common);

	if (log_level_with(log_pm_control))
		dev_info(dev, "suspended\n");

	return 0;
}

static const struct dev_pm_ops smi_common_pm_ops = {
	SET_RUNTIME_PM_OPS(mtk_smi_common_suspend, mtk_smi_common_resume, NULL)
	SET_LATE_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				     pm_runtime_force_resume)
};

static struct platform_driver mtk_smi_common_driver = {
	.probe	= mtk_smi_common_probe,
	.remove = mtk_smi_common_remove,
	.driver	= {
		.name = "mtk-smi-common",
		.of_match_table = mtk_smi_common_of_ids,
		.pm             = &smi_common_pm_ops,
	}
};

static struct platform_driver * const smidrivers[] = {
	&mtk_smi_common_driver,
	&mtk_smi_larb_driver,
};

static int __init mtk_smi_init(void)
{
	return platform_register_drivers(smidrivers, ARRAY_SIZE(smidrivers));
}
module_init(mtk_smi_init);

static void __exit mtk_smi_exit(void)
{
	platform_unregister_drivers(smidrivers, ARRAY_SIZE(smidrivers));
}
module_exit(mtk_smi_exit);

module_param(log_level, uint, 0644);
MODULE_PARM_DESC(log_level, "smi log level");

MODULE_DESCRIPTION("MediaTek SMI driver");
MODULE_LICENSE("GPL v2");
