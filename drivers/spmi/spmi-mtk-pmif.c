// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2020 MediaTek Inc.

#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/spmi.h>

#define SWINF_IDLE	0x00
#define SWINF_WFVLDCLR	0x06

#define GET_SWINF(x)	(((x) >> 1) & 0x7)

#define PMIF_CMD_REG_0		0
#define PMIF_CMD_REG		1
#define PMIF_CMD_EXT_REG	2
#define PMIF_CMD_EXT_REG_LONG	3

#define PMIF_DELAY_US   10
#define PMIF_TIMEOUT    (10 * 1000)

#define PMIF_CHAN_OFFSET 0x5

#define SPMI_OP_ST_BUSY 1

struct ch_reg {
	u32 ch_sta;
	u32 wdata;
	u32 rdata;
	u32 ch_send;
	u32 ch_rdy;
};

enum {
	/* MT8188 series */
	IRQ_PMIF_ACC_VIO_V3 = 27,
	IRQ_PMIC_ACC_VIO_V3 = 28,
	IRQ_HW_MONITOR_V3 = 14,
	IRQ_WDT_V3 = 15,
	IRQ_ALL_PMIC_MPU_VIO_V3 = 16,
};
struct pmif_irq_desc {
	const char *name;
	irq_handler_t irq_handler;
	int irq;
};

#define PMIF_IRQDESC(name) { #name, pmif_##name##_irq_handler, -1}

struct pmif {
	void __iomem	*base;
	const u32	*regs;
	void __iomem	*spmimst_base;
	const u32	*spmimst_regs;
	u32		soc_chan;
	int		grpid;
	int		irq, irq_m;
	raw_spinlock_t	lock;
	struct wakeup_source *pmifThread_lock;
	struct wakeup_source *pmifThread_lock_m;
	struct mutex pmif_mutex, pmif_mutex_m;
	struct clk	*pmif_sys_ck;
	struct clk	*pmif_tmr_ck;
	struct clk	*spmimst_clk_mux;
	struct ch_reg chan;
};

enum pmif_regs {
	PMIF_INIT_DONE,
	PMIF_INF_EN,
	PMIF_ARB_EN,
	PMIF_CMDISSUE_EN,
	PMIF_TIMER_CTRL,
	PMIF_SPI_MODE_CTRL,
	PMIF_IRQ_EVENT_EN_0,
	PMIF_IRQ_FLAG_0,
	PMIF_IRQ_CLR_0,
	PMIF_IRQ_EVENT_EN_1,
	PMIF_IRQ_FLAG_1,
	PMIF_IRQ_CLR_1,
	PMIF_IRQ_EVENT_EN_2,
	PMIF_IRQ_FLAG_2,
	PMIF_IRQ_CLR_2,
	PMIF_IRQ_EVENT_EN_3,
	PMIF_IRQ_FLAG_3,
	PMIF_IRQ_CLR_3,
	PMIF_IRQ_EVENT_EN_4,
	PMIF_IRQ_FLAG_4,
	PMIF_IRQ_CLR_4,
	PMIF_WDT_EVENT_EN_0,
	PMIF_WDT_FLAG_0,
	PMIF_WDT_EVENT_EN_1,
	PMIF_WDT_FLAG_1,
	PMIF_SWINF_0_STA,
	PMIF_SWINF_0_WDATA_31_0,
	PMIF_SWINF_0_RDATA_31_0,
	PMIF_SWINF_0_ACC,
	PMIF_SWINF_0_VLD_CLR,
	PMIF_SWINF_1_STA,
	PMIF_SWINF_1_WDATA_31_0,
	PMIF_SWINF_1_RDATA_31_0,
	PMIF_SWINF_1_ACC,
	PMIF_SWINF_1_VLD_CLR,
	PMIF_SWINF_2_STA,
	PMIF_SWINF_2_WDATA_31_0,
	PMIF_SWINF_2_RDATA_31_0,
	PMIF_SWINF_2_ACC,
	PMIF_SWINF_2_VLD_CLR,
	PMIF_SWINF_3_STA,
	PMIF_SWINF_3_WDATA_31_0,
	PMIF_SWINF_3_RDATA_31_0,
	PMIF_SWINF_3_ACC,
	PMIF_SWINF_3_VLD_CLR,
};

static const u32 mt6873_regs[] = {
	[PMIF_INIT_DONE] =	0x0000,
	[PMIF_INF_EN] =		0x0024,
	[PMIF_ARB_EN] =		0x0150,
	[PMIF_CMDISSUE_EN] =	0x03B4,
	[PMIF_TIMER_CTRL] =	0x03E0,
	[PMIF_SPI_MODE_CTRL] =	0x0400,
	[PMIF_IRQ_EVENT_EN_0] =	0x0418,
	[PMIF_IRQ_FLAG_0] =	0x0420,
	[PMIF_IRQ_CLR_0] =	0x0424,
	[PMIF_IRQ_EVENT_EN_1] =	0x0428,
	[PMIF_IRQ_FLAG_1] =	0x0430,
	[PMIF_IRQ_CLR_1] =	0x0434,
	[PMIF_IRQ_EVENT_EN_2] =	0x0438,
	[PMIF_IRQ_FLAG_2] =	0x0440,
	[PMIF_IRQ_CLR_2] =	0x0444,
	[PMIF_IRQ_EVENT_EN_3] =	0x0448,
	[PMIF_IRQ_FLAG_3] =	0x0450,
	[PMIF_IRQ_CLR_3] =	0x0454,
	[PMIF_IRQ_EVENT_EN_4] =	0x0458,
	[PMIF_IRQ_FLAG_4] =	0x0460,
	[PMIF_IRQ_CLR_4] =	0x0464,
	[PMIF_WDT_EVENT_EN_0] =	0x046C,
	[PMIF_WDT_FLAG_0] =	0x0470,
	[PMIF_WDT_EVENT_EN_1] =	0x0474,
	[PMIF_WDT_FLAG_1] =	0x0478,
	[PMIF_SWINF_0_ACC] =	0x0C00,
	[PMIF_SWINF_0_WDATA_31_0] =	0x0C04,
	[PMIF_SWINF_0_RDATA_31_0] =	0x0C14,
	[PMIF_SWINF_0_VLD_CLR] =	0x0C24,
	[PMIF_SWINF_0_STA] =	0x0C28,
	[PMIF_SWINF_1_ACC] =	0x0C40,
	[PMIF_SWINF_1_WDATA_31_0] =	0x0C44,
	[PMIF_SWINF_1_RDATA_31_0] =	0x0C54,
	[PMIF_SWINF_1_VLD_CLR] =	0x0C64,
	[PMIF_SWINF_1_STA] =	0x0C68,
	[PMIF_SWINF_2_ACC] =	0x0C80,
	[PMIF_SWINF_2_WDATA_31_0] =	0x0C84,
	[PMIF_SWINF_2_RDATA_31_0] =	0x0C94,
	[PMIF_SWINF_2_VLD_CLR] =	0x0CA4,
	[PMIF_SWINF_2_STA] =	0x0CA8,
	[PMIF_SWINF_3_ACC] =	0x0CC0,
	[PMIF_SWINF_3_WDATA_31_0] =	0x0CC4,
	[PMIF_SWINF_3_RDATA_31_0] =	0x0CD4,
	[PMIF_SWINF_3_VLD_CLR] =	0x0CE4,
	[PMIF_SWINF_3_STA] =	0x0CE8,
};

static const u32 mt8188_regs[] = {
	[PMIF_INIT_DONE] =			0x0000,
	[PMIF_INF_EN] =				0x0024,
	[PMIF_ARB_EN] =				0x0150,
	[PMIF_CMDISSUE_EN] =			0x03B8,
	[PMIF_TIMER_CTRL] =			0x03E4,
	[PMIF_SPI_MODE_CTRL] =			0x0408,
	[PMIF_IRQ_EVENT_EN_0] =			0x0420,
	[PMIF_IRQ_FLAG_0] =			0x0428,
	[PMIF_IRQ_CLR_0] =			0x042C,
	[PMIF_IRQ_EVENT_EN_1] =			0x0430,
	[PMIF_IRQ_FLAG_1] =			0x0438,
	[PMIF_IRQ_CLR_1] =			0x043C,
	[PMIF_IRQ_EVENT_EN_2] =			0x0440,
	[PMIF_IRQ_FLAG_2] =			0x0448,
	[PMIF_IRQ_CLR_2] =			0x044C,
	[PMIF_IRQ_EVENT_EN_3] =			0x0450,
	[PMIF_IRQ_FLAG_3] =			0x0458,
	[PMIF_IRQ_CLR_3] =			0x045C,
	[PMIF_IRQ_EVENT_EN_4] =			0x0460,
	[PMIF_IRQ_FLAG_4] =			0x0468,
	[PMIF_IRQ_CLR_4] =			0x046C,
	[PMIF_WDT_EVENT_EN_0] =			0x0474,
	[PMIF_WDT_FLAG_0] =			0x0478,
	[PMIF_WDT_EVENT_EN_1] =			0x047C,
	[PMIF_WDT_FLAG_1] =			0x0480,
	[PMIF_SWINF_0_ACC] =			0x0800,
	[PMIF_SWINF_0_WDATA_31_0] =		0x0804,
	[PMIF_SWINF_0_RDATA_31_0] =		0x0814,
	[PMIF_SWINF_0_VLD_CLR] =		0x0824,
	[PMIF_SWINF_0_STA] =			0x0828,
	[PMIF_SWINF_1_ACC] =			0x0840,
	[PMIF_SWINF_1_WDATA_31_0] =		0x0844,
	[PMIF_SWINF_1_RDATA_31_0] =		0x0854,
	[PMIF_SWINF_1_VLD_CLR] =		0x0864,
	[PMIF_SWINF_1_STA] =			0x0868,
	[PMIF_SWINF_2_ACC] =			0x0880,
	[PMIF_SWINF_2_WDATA_31_0] =		0x0884,
	[PMIF_SWINF_2_RDATA_31_0] =		0x0894,
	[PMIF_SWINF_2_VLD_CLR] =		0x08A4,
	[PMIF_SWINF_2_STA] =			0x08A8,
	[PMIF_SWINF_3_ACC] =			0x08C0,
	[PMIF_SWINF_3_WDATA_31_0] =		0x08C4,
	[PMIF_SWINF_3_RDATA_31_0] =		0x08D4,
	[PMIF_SWINF_3_VLD_CLR] =		0x08E4,
	[PMIF_SWINF_3_STA] =			0x08E8,
};

enum spmi_regs {
	SPMI_OP_ST_CTRL,
	SPMI_GRP_ID_EN,
	SPMI_OP_ST_STA,
	SPMI_MST_SAMPL,
	SPMI_MST_REQ_EN,
	SPMI_REC_CTRL,
	SPMI_REC0,
	SPMI_REC1,
	SPMI_REC2,
	SPMI_REC3,
	SPMI_REC4,
	SPMI_MST_DBG,

	/* MT8188 spmi regs */
	SPMI_MST_RCS_CTRL,
	SPMI_SLV_3_0_EINT,
	SPMI_SLV_7_4_EINT,
	SPMI_SLV_B_8_EINT,
	SPMI_SLV_F_C_EINT,
	SPMI_REC_CMD_DEC,
	SPMI_DEC_DBG,
};

static const u32 mt6873_spmi_regs[] = {
	[SPMI_OP_ST_CTRL] =	0x0000,
	[SPMI_GRP_ID_EN] =	0x0004,
	[SPMI_OP_ST_STA] =	0x0008,
	[SPMI_MST_SAMPL] =	0x000c,
	[SPMI_MST_REQ_EN] =	0x0010,
	[SPMI_REC_CTRL] =	0x0040,
	[SPMI_REC0] =		0x0044,
	[SPMI_REC1] =		0x0048,
	[SPMI_REC2] =		0x004c,
	[SPMI_REC3] =		0x0050,
	[SPMI_REC4] =		0x0054,
	[SPMI_MST_DBG] =	0x00fc,
};

static const u32 mt8188_spmi_regs[] = {
	[SPMI_OP_ST_CTRL] =	0x0000,
	[SPMI_GRP_ID_EN] =	0x0004,
	[SPMI_OP_ST_STA] =	0x0008,
	[SPMI_MST_SAMPL] =	0x000C,
	[SPMI_MST_REQ_EN] =	0x0010,
	[SPMI_MST_RCS_CTRL] =	0x0014,
	[SPMI_SLV_3_0_EINT] =	0x0020,
	[SPMI_SLV_7_4_EINT] =	0x0024,
	[SPMI_SLV_B_8_EINT] =	0x0028,
	[SPMI_SLV_F_C_EINT] =	0x002C,
	[SPMI_REC_CTRL] =	0x0040,
	[SPMI_REC0] =		0x0044,
	[SPMI_REC1] =		0x0048,
	[SPMI_REC2] =		0x004C,
	[SPMI_REC3] =		0x0050,
	[SPMI_REC4] =		0x0054,
	[SPMI_REC_CMD_DEC] =	0x005C,
	[SPMI_DEC_DBG] =	0x00F8,
	[SPMI_MST_DBG] =	0x00FC,
};

static u32 pmif_readl(struct pmif *arb, enum pmif_regs reg)
{
	return readl(arb->base + arb->regs[reg]);
}

static void pmif_writel(struct pmif *arb, u32 val, enum pmif_regs reg)
{
	writel(val, arb->base + arb->regs[reg]);
}

static u32 mtk_spmi_readl(struct pmif *arb, enum spmi_regs reg)
{
	return readl(arb->spmimst_base + arb->spmimst_regs[reg]);
}

static void mtk_spmi_writel(struct pmif *arb, u32 val, enum spmi_regs reg)
{
	writel(val, arb->spmimst_base + arb->spmimst_regs[reg]);
}

static bool pmif_is_fsm_vldclr(struct pmif *arb)
{
	u32 reg_rdata;

	reg_rdata = pmif_readl(arb, arb->chan.ch_sta);
	return GET_SWINF(reg_rdata) == SWINF_WFVLDCLR;
}

static int pmif_arb_cmd(struct spmi_controller *ctrl, u8 opc, u8 sid)
{
	struct pmif *arb = spmi_controller_get_drvdata(ctrl);
	u32 rdata;
	u8 cmd;
	int ret;

	/* Check the opcode */
	if (opc == SPMI_CMD_RESET)
		cmd = 0;
	else if (opc == SPMI_CMD_SLEEP)
		cmd = 1;
	else if (opc == SPMI_CMD_SHUTDOWN)
		cmd = 2;
	else if (opc == SPMI_CMD_WAKEUP)
		cmd = 3;
	else
		return -EINVAL;

	mtk_spmi_writel(arb, (cmd << 0x4) | sid, SPMI_OP_ST_CTRL);
	ret = readl_poll_timeout_atomic(arb->spmimst_base + arb->spmimst_regs[SPMI_OP_ST_STA],
					rdata, (rdata & SPMI_OP_ST_BUSY) == SPMI_OP_ST_BUSY,
					PMIF_DELAY_US, PMIF_TIMEOUT);
	if (ret < 0)
		dev_err(&ctrl->dev, "timeout, err = %d\r\n", ret);

	return ret;
}

static int pmif_spmi_read_cmd(struct spmi_controller *ctrl, u8 opc, u8 sid,
			      u16 addr, u8 *buf, size_t len)
{
	struct pmif *arb = spmi_controller_get_drvdata(ctrl);
	struct ch_reg *inf_reg = NULL;
	int ret;
	u32 data = 0;
	u8 bc = len - 1;
	unsigned long flags;

	/* Check for argument validation. */
	if (sid & ~(0xf))
		return -EINVAL;

	if (!arb)
		return -EINVAL;

	inf_reg = &arb->chan;
	/* Check the opcode */
	if (opc >= 0x60 && opc <= 0x7f)
		opc = PMIF_CMD_REG;
	else if (opc >= 0x20 && opc <= 0x2f)
		opc = PMIF_CMD_EXT_REG_LONG;
	else if (opc >= 0x38 && opc <= 0x3f)
		opc = PMIF_CMD_EXT_REG_LONG;
	else
		return -EINVAL;

	raw_spin_lock_irqsave(&arb->lock, flags);
	/* Wait for Software Interface FSM state to be IDLE. */
	ret = readl_poll_timeout_atomic(arb->base + arb->regs[inf_reg->ch_sta],
					data, GET_SWINF(data) == SWINF_IDLE,
					PMIF_DELAY_US, PMIF_TIMEOUT);
	if (ret < 0) {
		/* set channel ready if the data has transferred */
		if (pmif_is_fsm_vldclr(arb))
			pmif_writel(arb, 1, inf_reg->ch_rdy);
		raw_spin_unlock_irqrestore(&arb->lock, flags);
		return ret;
	}

	/* Send the command. */
	pmif_writel(arb,
		    (opc << 30) | (sid << 24) | (bc << 16) | addr,
		    inf_reg->ch_send);

	/* Wait for Software Interface FSM state to be WFVLDCLR,
	 *
	 * read the data and clear the valid flag.
	 */
	ret = readl_poll_timeout_atomic(arb->base + arb->regs[inf_reg->ch_sta],
					data, GET_SWINF(data) == SWINF_WFVLDCLR,
					PMIF_DELAY_US, PMIF_TIMEOUT);
	if (ret < 0) {
		raw_spin_unlock_irqrestore(&arb->lock, flags);
		return ret;
	}

	data = pmif_readl(arb, inf_reg->rdata);
	memcpy(buf, &data, (bc & 3) + 1);
	pmif_writel(arb, 1, inf_reg->ch_rdy);
	raw_spin_unlock_irqrestore(&arb->lock, flags);

	return 0;
}

static int pmif_spmi_write_cmd(struct spmi_controller *ctrl, u8 opc, u8 sid,
			       u16 addr, const u8 *buf, size_t len)
{
	struct pmif *arb = spmi_controller_get_drvdata(ctrl);
	struct ch_reg *inf_reg = NULL;
	int ret;
	u32 data = 0;
	u8 bc = len - 1;
	unsigned long flags = 0;

	/* Check for argument validation. */
	if (sid & ~(0xf))
		return -EINVAL;

	if (!arb)
		return -EINVAL;

	inf_reg = &arb->chan;

	/* Check the opcode */
	if (opc >= 0x40 && opc <= 0x5F)
		opc = PMIF_CMD_REG;
	else if (opc <= 0x0F)
		opc = PMIF_CMD_EXT_REG_LONG;
	else if (opc >= 0x30 && opc <= 0x37)
		opc = PMIF_CMD_EXT_REG_LONG;
	else if (opc >= 0x80)
		opc = PMIF_CMD_REG_0;
	else
		return -EINVAL;

	raw_spin_lock_irqsave(&arb->lock, flags);
	/* Wait for Software Interface FSM state to be IDLE. */
	ret = readl_poll_timeout_atomic(arb->base + arb->regs[inf_reg->ch_sta],
					data, GET_SWINF(data) == SWINF_IDLE,
					PMIF_DELAY_US, PMIF_TIMEOUT);
	if (ret < 0) {
		/* set channel ready if the data has transferred */
		if (pmif_is_fsm_vldclr(arb))
			pmif_writel(arb, 1, inf_reg->ch_rdy);
		raw_spin_unlock_irqrestore(&arb->lock, flags);
		return ret;
	}

	/* Set the write data. */
	memcpy(&data, buf, (bc & 3) + 1);
	pmif_writel(arb, data, inf_reg->wdata);

	/* Send the command. */
	pmif_writel(arb,
		    (opc << 30) | BIT(29) | (sid << 24) | (bc << 16) | addr,
		    inf_reg->ch_send);
	raw_spin_unlock_irqrestore(&arb->lock, flags);

	return 0;
}

/*
 * PMIF Exception IRQ Handler
 */
static void pmif_pmif_acc_vio_irq_handler(int irq, void *data)
{
	pr_notice("PMIF:pmif_acc_vio");
}

static void pmif_pmic_acc_vio_irq_handler(int irq, void *data)
{
	pr_notice("PMIF:pmic_acc_vio");
}

static void pmif_hw_monitor_irq_handler(int irq, void *data)
{
	pr_notice("PMIF:pmif_hw_monitor_match");
}

static void pmif_wdt_irq_handler(int irq, void *data)
{
	pr_notice("PMIF:pmif_wdt_irq");
}

static irqreturn_t pmif_event_0_irq_handler(int irq, void *data)
{
	struct pmif *arb = data;
	int irq_f = 0, idx = 0;

	__pm_stay_awake(arb->pmifThread_lock);
	mutex_lock(&arb->pmif_mutex);
	irq_f = pmif_readl(arb, PMIF_IRQ_FLAG_0);
	if (irq_f == 0) {
		mutex_unlock(&arb->pmif_mutex);
		__pm_relax(arb->pmifThread_lock);
		return IRQ_NONE;
	}

	for (idx = 0; idx < 31; idx++) {
		if ((irq_f & (0x1 << idx)) != 0) {
			switch (idx) {
			case IRQ_PMIF_ACC_VIO_V3:
				pmif_pmif_acc_vio_irq_handler(irq, data);
			break;
			case IRQ_PMIC_ACC_VIO_V3:
				pmif_pmic_acc_vio_irq_handler(irq, data);
			break;
			default:
				pr_notice("%s IRQ[%d] triggered\n",
					__func__, idx);
			break;
			}
			pmif_writel(arb, irq_f, PMIF_IRQ_CLR_0);
			break;
		}
	}
	mutex_unlock(&arb->pmif_mutex);
	__pm_relax(arb->pmifThread_lock);

	return IRQ_HANDLED;
}

static irqreturn_t pmif_event_1_irq_handler(int irq, void *data)
{
	struct pmif *arb = data;
	int irq_f = 0, idx = 0;

	__pm_stay_awake(arb->pmifThread_lock);
	mutex_lock(&arb->pmif_mutex);
	irq_f = pmif_readl(arb, PMIF_IRQ_FLAG_1);
	if (irq_f == 0) {
		mutex_unlock(&arb->pmif_mutex);
		__pm_relax(arb->pmifThread_lock);
		return IRQ_NONE;
	}

	for (idx = 0; idx < 31; idx++) {
		if ((irq_f & (0x1 << idx)) != 0) {
			switch (idx) {
			case IRQ_HW_MONITOR_V3:
				pmif_hw_monitor_irq_handler(irq, data);
			break;
			case IRQ_WDT_V3:
				pmif_wdt_irq_handler(irq, data);
			break;
			case IRQ_ALL_PMIC_MPU_VIO_V3:
				pmif_pmif_acc_vio_irq_handler(irq, data);
			break;
			default:
				pr_notice("%s IRQ[%d] triggered\n",
					__func__, idx);
			break;
			}
			pmif_writel(arb, irq_f, PMIF_IRQ_CLR_1);
			break;
		}
	}
	mutex_unlock(&arb->pmif_mutex);
	__pm_relax(arb->pmifThread_lock);

	return IRQ_HANDLED;
}

static irqreturn_t pmif_event_2_irq_handler(int irq, void *data)
{
	struct pmif *arb = data;
	int irq_f = 0, idx = 0;

	__pm_stay_awake(arb->pmifThread_lock);
	mutex_lock(&arb->pmif_mutex);
	irq_f = pmif_readl(arb, PMIF_IRQ_FLAG_2);
	if (irq_f == 0) {
		mutex_unlock(&arb->pmif_mutex);
		__pm_relax(arb->pmifThread_lock);
		return IRQ_NONE;
	}

	for (idx = 0; idx < 31; idx++) {
		if ((irq_f & (0x1 << idx)) != 0) {
			switch (idx) {
			default:
				pr_notice("%s IRQ[%d] triggered\n",
					__func__, idx);
			break;
			}
			pmif_writel(arb, irq_f, PMIF_IRQ_CLR_2);
			break;
		}
	}
	mutex_unlock(&arb->pmif_mutex);
	__pm_relax(arb->pmifThread_lock);

	return IRQ_HANDLED;
}

static irqreturn_t pmif_event_3_irq_handler(int irq, void *data)
{
	struct pmif *arb = data;
	int irq_f = 0, idx = 0;

	__pm_stay_awake(arb->pmifThread_lock);
	mutex_lock(&arb->pmif_mutex);
	irq_f = pmif_readl(arb, PMIF_IRQ_FLAG_3);
	if (irq_f == 0) {
		mutex_unlock(&arb->pmif_mutex);
		__pm_relax(arb->pmifThread_lock);
		return IRQ_NONE;
	}

	for (idx = 0; idx < 31; idx++) {
		if ((irq_f & (0x1 << idx)) != 0) {
			switch (idx) {
			default:
				pr_notice("%s IRQ[%d] triggered\n",
					__func__, idx);
			break;
			}
			pmif_writel(arb, irq_f, PMIF_IRQ_CLR_3);
			break;
		}
	}
	mutex_unlock(&arb->pmif_mutex);
	__pm_relax(arb->pmifThread_lock);

	return IRQ_HANDLED;
}

static irqreturn_t pmif_event_4_irq_handler(int irq, void *data)
{
	struct pmif *arb = data;
	int irq_f = 0, idx = 0;

	__pm_stay_awake(arb->pmifThread_lock);
	mutex_lock(&arb->pmif_mutex);
	irq_f = pmif_readl(arb, PMIF_IRQ_FLAG_4);
	if (irq_f == 0) {
		mutex_unlock(&arb->pmif_mutex);
		__pm_relax(arb->pmifThread_lock);
		return IRQ_NONE;
	}

	for (idx = 0; idx < 31; idx++) {
		if ((irq_f & (0x1 << idx)) != 0) {
			switch (idx) {
			default:
				pr_notice("%s IRQ[%d] triggered\n",
					__func__, idx);
			break;
			}
			pmif_writel(arb, irq_f, PMIF_IRQ_CLR_4);
			break;
		}
	}
	mutex_unlock(&arb->pmif_mutex);
	__pm_relax(arb->pmifThread_lock);

	return IRQ_HANDLED;
}

static struct pmif_irq_desc pmif_event_irq[] = {
	PMIF_IRQDESC(event_0),
	PMIF_IRQDESC(event_1),
	PMIF_IRQDESC(event_2),
	PMIF_IRQDESC(event_3),
	PMIF_IRQDESC(event_4),
};

static void pmif_irq_register(struct platform_device *pdev,
		struct pmif *arb, int irq)
{
	int i = 0, ret = 0;
	u32 irq_event_en[5] = {0};

	for (i = 0; i < ARRAY_SIZE(pmif_event_irq); i++) {
		if (!pmif_event_irq[i].name)
			continue;
		ret = devm_request_threaded_irq(&pdev->dev, irq, NULL,
				pmif_event_irq[i].irq_handler,
				IRQF_TRIGGER_HIGH | IRQF_ONESHOT | IRQF_SHARED,
				pmif_event_irq[i].name, arb);
		if (ret < 0) {
			dev_notice(&pdev->dev, "request %s irq fail\n",
				pmif_event_irq[i].name);
			continue;
		}
		pmif_event_irq[i].irq = irq;
	}

	ret = of_property_read_u32_array(pdev->dev.of_node, "irq_event_en",
		irq_event_en, ARRAY_SIZE(irq_event_en));

	pmif_writel(arb, irq_event_en[0] | pmif_readl(arb, PMIF_IRQ_EVENT_EN_0),
			PMIF_IRQ_EVENT_EN_0);
	pmif_writel(arb, irq_event_en[1] | pmif_readl(arb, PMIF_IRQ_EVENT_EN_1),
			PMIF_IRQ_EVENT_EN_1);
	pmif_writel(arb, irq_event_en[2] | pmif_readl(arb, PMIF_IRQ_EVENT_EN_2),
			PMIF_IRQ_EVENT_EN_2);
	pmif_writel(arb, irq_event_en[3] | pmif_readl(arb, PMIF_IRQ_EVENT_EN_3),
			PMIF_IRQ_EVENT_EN_3);
	pmif_writel(arb, irq_event_en[4] | pmif_readl(arb, PMIF_IRQ_EVENT_EN_4),
			PMIF_IRQ_EVENT_EN_4);
}

static irqreturn_t pmif_irq_m_handler(int irq, void *data)
{
	struct pmif *arb = data;
	int irq_f = 0, idx = 0;

	__pm_stay_awake(arb->pmifThread_lock_m);
	mutex_lock(&arb->pmif_mutex_m);

	for (idx = SPMI_REC0; idx < SPMI_REC4; idx++) {
		irq_f = mtk_spmi_readl(arb, idx);
		pr_notice("spmimst 0x%x = 0x%x\n",
			  arb->spmimst_regs[idx], irq_f);
	}

	mtk_spmi_writel(arb, 0x3, SPMI_REC_CTRL);

	mutex_unlock(&arb->pmif_mutex_m);
	__pm_relax(arb->pmifThread_lock_m);

	return IRQ_HANDLED;
}

static void pmif_irq_m_register(struct platform_device *pdev,
		struct pmif *arb, int irq)
{
	int ret = 0;

	ret = devm_request_threaded_irq(&pdev->dev, irq, NULL,
			pmif_irq_m_handler,
			IRQF_TRIGGER_HIGH | IRQF_ONESHOT | IRQF_SHARED,
			"pmif_irq_m", arb);
	if (ret < 0)
		pr_notice("request irq fail\n");
}

static struct pmif mt6873_pmif_arb[] = {
	{
		.regs = mt6873_regs,
		.spmimst_regs = mt6873_spmi_regs,
		.soc_chan = 2,
	},
};

static struct pmif mt8188_pmif_arb[] = {
	{
		.regs = mt8188_regs,
		.spmimst_regs = mt8188_spmi_regs,
		.soc_chan = 2,
	},
};

static int mtk_spmi_probe(struct platform_device *pdev)
{
	struct pmif *arb;
	struct resource *res;
	struct spmi_controller *ctrl;
	int err = 0;

	ctrl = spmi_controller_alloc(&pdev->dev, sizeof(*arb));
	if (!ctrl)
		return -ENOMEM;

	ctrl->cmd = pmif_arb_cmd;
	ctrl->read_cmd = pmif_spmi_read_cmd;
	ctrl->write_cmd = pmif_spmi_write_cmd;

	/* re-assign of_id->data */
	spmi_controller_set_drvdata(ctrl, (void *)of_device_get_match_data(&pdev->dev));
	arb = spmi_controller_get_drvdata(ctrl);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pmif");
	arb->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(arb->base)) {
		err = PTR_ERR(arb->base);
		goto err_put_ctrl;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "spmimst");
	arb->spmimst_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(arb->spmimst_base)) {
		err = PTR_ERR(arb->spmimst_base);
		goto err_put_ctrl;
	}

	arb->pmif_sys_ck = devm_clk_get(&pdev->dev, "pmif_sys_ck");
	if (IS_ERR(arb->pmif_sys_ck)) {
		dev_err(&pdev->dev, "[PMIF]:failed to get ap clock: %ld\n",
			PTR_ERR(arb->pmif_sys_ck));
		goto err_put_ctrl;
	}

	arb->pmif_tmr_ck = devm_clk_get(&pdev->dev, "pmif_tmr_ck");
	if (IS_ERR(arb->pmif_tmr_ck)) {
		dev_err(&pdev->dev, "[PMIF]:failed to get tmr clock: %ld\n",
			PTR_ERR(arb->pmif_tmr_ck));
		goto err_put_ctrl;
	}

	arb->spmimst_clk_mux = devm_clk_get(&pdev->dev, "spmimst_clk_mux");
	if (IS_ERR(arb->spmimst_clk_mux)) {
		dev_err(&pdev->dev, "[SPMIMST]:failed to get clock: %ld\n",
			PTR_ERR(arb->spmimst_clk_mux));
		goto err_put_ctrl;
	}

	err = clk_prepare_enable(arb->spmimst_clk_mux);
	if (err) {
		dev_err(&pdev->dev, "[PMIF]:failed to enable spmi master clk.\n");
		goto err_put_ctrl;
	}

	arb->chan.ch_sta = PMIF_SWINF_0_STA + (PMIF_CHAN_OFFSET * arb->soc_chan);
	arb->chan.wdata = PMIF_SWINF_0_WDATA_31_0 + (PMIF_CHAN_OFFSET * arb->soc_chan);
	arb->chan.rdata = PMIF_SWINF_0_RDATA_31_0 + (PMIF_CHAN_OFFSET * arb->soc_chan);
	arb->chan.ch_send = PMIF_SWINF_0_ACC + (PMIF_CHAN_OFFSET * arb->soc_chan);
	arb->chan.ch_rdy = PMIF_SWINF_0_VLD_CLR + (PMIF_CHAN_OFFSET * arb->soc_chan);

	raw_spin_lock_init(&arb->lock);

	arb->pmifThread_lock =
		wakeup_source_register(NULL, "pmif wakelock");
	mutex_init(&arb->pmif_mutex);

	arb->pmifThread_lock_m =
		wakeup_source_register(NULL, "pmif_m wakelock");
	mutex_init(&arb->pmif_mutex_m);

	arb->irq = platform_get_irq(pdev, 0);
	if (arb->irq < 0) {
		err = arb->irq;
		dev_notice(&pdev->dev, "get irq 0 fail %d\n",
				arb->irq);
		goto err_put_ctrl;
	}

	arb->irq_m = platform_get_irq(pdev, 1);
	if (arb->irq_m < 0) {
		err = arb->irq_m;
		dev_notice(&pdev->dev, "get irq_m fail %d\n",
				arb->irq_m);
		goto err_put_ctrl;
	}

	pmif_irq_register(pdev, arb, arb->irq);
	pmif_irq_m_register(pdev, arb, arb->irq_m);

	platform_set_drvdata(pdev, ctrl);

	err = spmi_controller_add(ctrl);
	if (err)
		goto err_domain_remove;

	return 0;

err_domain_remove:
	clk_disable_unprepare(arb->spmimst_clk_mux);
err_put_ctrl:
	spmi_controller_put(ctrl);
	return err;
}

static int mtk_spmi_remove(struct platform_device *pdev)
{
	struct spmi_controller *ctrl = platform_get_drvdata(pdev);

	spmi_controller_remove(ctrl);
	spmi_controller_put(ctrl);
	return 0;
}

static const struct of_device_id mtk_spmi_match_table[] = {
	{
		.compatible = "mediatek,mt6873-spmi",
		.data = &mt6873_pmif_arb,
	}, {
		.compatible = "mediatek,mt8188-spmi",
		.data = &mt8188_pmif_arb,
	}, {
		/* sentinel */
	},
};
MODULE_DEVICE_TABLE(of, mtk_spmi_match_table);

static struct platform_driver mtk_spmi_driver = {
	.driver		= {
		.name	= "spmi-mtk",
		.of_match_table = of_match_ptr(mtk_spmi_match_table),
	},
	.probe		= mtk_spmi_probe,
	.remove		= mtk_spmi_remove,
};
module_platform_driver(mtk_spmi_driver);

MODULE_AUTHOR("Hsin-Hsiung Wang <hsin-hsiung.wang@mediatek.com>");
MODULE_DESCRIPTION("MediaTek SPMI Driver");
MODULE_LICENSE("GPL");
