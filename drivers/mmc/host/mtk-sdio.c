// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2014-2021 MediaTek Inc.
 * Author: Chaotian.Jing <chaotian.jing@mediatek.com>
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/slot-gpio.h>

#include "mtk-sdio.h"

static const struct of_device_id sdio_of_ids[] = {
	{ .compatible = "mediatek,mt8135-mmc", .data = &mt8135_compat},
	{ .compatible = "mediatek,mt8173-mmc", .data = &mt8173_compat},
	{ .compatible = "mediatek,mt8167-sdio", .data = &mt8167_compat},
	{ .compatible = "mediatek,mt8168-sdio", .data = &mt8168_compat},
	{ .compatible = "mediatek,mt8169-sdio", .data = &mt8169_compat},
	{ .compatible = "mediatek,mt2701-mmc", .data = &mt2701_compat},
	{ .compatible = "mediatek,mt2712-mmc", .data = &mt2712_compat},
	{ .compatible = "mediatek,mt7622-mmc", .data = &mt7622_compat},
	{ .compatible = "mediatek,mt8183-sdio", .data = &mt8183_compat},
	{ .compatible = "mediatek,mt8195-sdio", .data = &mt8195_compat},
	{ .compatible = "mediatek,mt8185-sdio", .data = &mt8185_compat},
	{ .compatible = "mediatek,mt8188-sdio", .data = &mt8188_compat},
	{}
};
MODULE_DEVICE_TABLE(of, sdio_of_ids);

static void sdr_set_bits(void __iomem *reg, u32 bs)
{
	u32 val = readl(reg);

	val |= bs;
	writel(val, reg);
}

static void sdr_clr_bits(void __iomem *reg, u32 bs)
{
	u32 val = readl(reg);

	val &= ~bs;
	writel(val, reg);
}

static void sdr_set_field(void __iomem *reg, u32 field, u32 val)
{
	unsigned int tv = readl(reg);

	tv &= ~field;
	tv |= ((val) << (ffs((unsigned int)field) - 1));
	writel(tv, reg);
}

static void sdr_get_field(void __iomem *reg, u32 field, u32 *val)
{
	unsigned int tv = readl(reg);

	*val = ((tv & field) >> (ffs((unsigned int)field) - 1));
}

static void msdc_reset_hw(struct msdc_host *host)
{
	u32 val;

	sdr_set_bits(host->base + MSDC_CFG, MSDC_CFG_RST);
	while (readl(host->base + MSDC_CFG) & MSDC_CFG_RST)
		cpu_relax();

	sdr_set_bits(host->base + MSDC_FIFOCS, MSDC_FIFOCS_CLR);
	while (readl(host->base + MSDC_FIFOCS) & MSDC_FIFOCS_CLR)
		cpu_relax();

	val = readl(host->base + MSDC_INT);
	writel(val, host->base + MSDC_INT);
}

static void msdc_dump_all_register(struct msdc_host *host);
static void msdc_cmd_next(struct msdc_host *host,
		struct mmc_request *mrq, struct mmc_command *cmd);

static const u32 cmd_ints_mask = MSDC_INTEN_CMDRDY | MSDC_INTEN_RSPCRCERR |
			MSDC_INTEN_CMDTMO | MSDC_INTEN_ACMDRDY |
			MSDC_INTEN_ACMDCRCERR | MSDC_INTEN_ACMDTMO;
static const u32 data_ints_mask = MSDC_INTEN_XFER_COMPL | MSDC_INTEN_DATTMO |
			MSDC_INTEN_DATCRCERR | MSDC_INTEN_DMA_BDCSERR |
			MSDC_INTEN_DMA_GPDCSERR | MSDC_INTEN_DMA_PROTECT;

static u8 msdc_dma_calcs(u8 *buf, u32 len)
{
	u32 i, sum = 0;

	for (i = 0; i < len; i++)
		sum += buf[i];
	return 0xff - (u8) sum;
}

static inline void msdc_dma_setup(struct msdc_host *host, struct msdc_dma *dma,
		struct mmc_data *data)
{
	unsigned int j, dma_len;
	dma_addr_t dma_address;
	u32 dma_ctrl;
	struct scatterlist *sg;
	struct mt_gpdma_desc *gpd;
	struct mt_bdma_desc *bd;

	sg = data->sg;

	gpd = dma->gpd;
	bd = dma->bd;

	/* modify gpd */
	gpd->gpd_info |= GPDMA_DESC_HWO;
	gpd->gpd_info |= GPDMA_DESC_BDP;
	/* need to clear first. use these bits to calc checksum */
	gpd->gpd_info &= ~GPDMA_DESC_CHECKSUM;
	gpd->gpd_info |= msdc_dma_calcs((u8 *) gpd, 16) << 8;

	/* modify bd */
	for_each_sg(data->sg, sg, data->sg_count, j) {
		dma_address = sg_dma_address(sg);
		dma_len = sg_dma_len(sg);

		/* init bd */
		bd[j].bd_info &= ~BDMA_DESC_BLKPAD;
		bd[j].bd_info &= ~BDMA_DESC_DWPAD;
		bd[j].ptr = lower_32_bits(dma_address);
		if (host->dev_comp->support_64g) {
			bd[j].bd_info &= ~BDMA_DESC_PTR_H4;
			bd[j].bd_info |= (upper_32_bits(dma_address) & 0xf)
					 << 28;
		}
		if (host->dev_comp->support_64g) {
			bd[j].bd_data_len &= ~BDMA_DESC_BUFLEN_EXT;
			bd[j].bd_data_len |= (dma_len & BDMA_DESC_BUFLEN_EXT);
		} else {
			bd[j].bd_data_len &= ~BDMA_DESC_BUFLEN;
			bd[j].bd_data_len |= (dma_len & BDMA_DESC_BUFLEN);
		}

		if (j == data->sg_count - 1) /* the last bd */
			bd[j].bd_info |= BDMA_DESC_EOL;
		else
			bd[j].bd_info &= ~BDMA_DESC_EOL;

		/* checksume need to clear first */
		bd[j].bd_info &= ~BDMA_DESC_CHECKSUM;
		bd[j].bd_info |= msdc_dma_calcs((u8 *)(&bd[j]), 16) << 8;
	}

	sdr_set_field(host->base + MSDC_DMA_CFG, MSDC_DMA_CFG_DECSEN, 1);
	dma_ctrl = readl_relaxed(host->base + MSDC_DMA_CTRL);
	dma_ctrl &= ~(MSDC_DMA_CTRL_BRUSTSZ | MSDC_DMA_CTRL_MODE);
	dma_ctrl |= (MSDC_BURST_64B << 12 | 1 << 8);
	writel_relaxed(dma_ctrl, host->base + MSDC_DMA_CTRL);
	if (host->dev_comp->support_64g)
		sdr_set_field(host->base + DMA_SA_H4BIT, DMA_ADDR_HIGH_4BIT,
			      upper_32_bits(dma->gpd_addr) & 0xf);
	writel(lower_32_bits(dma->gpd_addr), host->base + MSDC_DMA_SA);
}

static void msdc_prepare_data(struct msdc_host *host, struct mmc_request *mrq)
{
	struct mmc_data *data = mrq->data;

	if (!(data->host_cookie & MSDC_PREPARE_FLAG)) {
		data->host_cookie |= MSDC_PREPARE_FLAG;
		data->sg_count = dma_map_sg(host->dev, data->sg, data->sg_len,
					    mmc_get_dma_dir(data));
	}
}

static void msdc_unprepare_data(struct msdc_host *host, struct mmc_request *mrq)
{
	struct mmc_data *data = mrq->data;

	if (data->host_cookie & MSDC_ASYNC_FLAG)
		return;

	if (data->host_cookie & MSDC_PREPARE_FLAG) {
		dma_unmap_sg(host->dev, data->sg, data->sg_len,
			     mmc_get_dma_dir(data));
		data->host_cookie &= ~MSDC_PREPARE_FLAG;
	}
}

/* clock control primitives */
static void msdc_set_timeout(struct msdc_host *host, u32 ns, u32 clks)
{
	u32 timeout, clk_ns;
	u32 mode = 0;

	host->timeout_ns = ns;
	host->timeout_clks = clks;
	if (host->sclk == 0) {
		timeout = 0;
	} else {
		clk_ns  = 1000000000UL / host->sclk;
		timeout = (ns + clk_ns - 1) / clk_ns + clks;
		/* in 1048576 sclk cycle unit */
		timeout = (timeout + (0x1 << 20) - 1) >> 20;
		if (host->dev_comp->clk_div_bits == 8)
			sdr_get_field(host->base + MSDC_CFG,
				      MSDC_CFG_CKMOD, &mode);
		else
			sdr_get_field(host->base + MSDC_CFG,
				      MSDC_CFG_CKMOD_EXTRA, &mode);
		/*DDR mode will double the clk cycles for data timeout */
		timeout = mode >= 2 ? timeout * 2 : timeout;
		timeout = timeout > 1 ? timeout - 1 : 0;
		timeout = timeout > 255 ? 255 : timeout;
	}
	sdr_set_field(host->base + SDC_CFG, SDC_CFG_DTOC, timeout);
}

static void msdc_gate_clock(struct msdc_host *host)
{
	clk_disable_unprepare(host->src_clk_cg);
	clk_disable_unprepare(host->src_clk);
	clk_disable_unprepare(host->h_clk);
	clk_disable_unprepare(host->bus_clk);
	clk_disable_unprepare(host->ext_clk);
}

static void msdc_ungate_clock(struct msdc_host *host)
{
	clk_prepare_enable(host->bus_clk);
	clk_prepare_enable(host->ext_clk);
	clk_prepare_enable(host->h_clk);
	clk_prepare_enable(host->src_clk);
	clk_prepare_enable(host->src_clk_cg);
	while (!(readl(host->base + MSDC_CFG) & MSDC_CFG_CKSTB))
		cpu_relax();
}

static void msdc_set_mclk(struct msdc_host *host, unsigned char timing, u32 hz)
{
	u32 mode;
	u32 flags;
	u32 div;
	u32 sclk;
	u32 tune_reg = host->dev_comp->pad_tune_reg;

	if (!hz) {
		dev_dbg(host->dev, "set mclk to 0\n");
		host->mclk = 0;
		sdr_clr_bits(host->base + MSDC_CFG, MSDC_CFG_CKPDN);
		return;
	}

	flags = readl(host->base + MSDC_INTEN);
	sdr_clr_bits(host->base + MSDC_INTEN, flags);
	if (host->dev_comp->clk_div_bits == 8)
		sdr_clr_bits(host->base + MSDC_CFG, MSDC_CFG_HS400_CK_MODE);
	else
		sdr_clr_bits(host->base + MSDC_CFG,
			     MSDC_CFG_HS400_CK_MODE_EXTRA);
	if (timing == MMC_TIMING_UHS_DDR50 ||
	    timing == MMC_TIMING_MMC_DDR52 ||
	    timing == MMC_TIMING_MMC_HS400) {
		if (timing == MMC_TIMING_MMC_HS400)
			mode = 0x3;
		else
			mode = 0x2; /* ddr mode and use divisor */

		if (hz >= (host->src_clk_freq >> 2)) {
			div = 0; /* mean div = 1/4 */
			sclk = host->src_clk_freq >> 2; /* sclk = clk / 4 */
		} else {
			div = (host->src_clk_freq + ((hz << 2) - 1)) /
					(hz << 2);
			sclk = (host->src_clk_freq >> 2) / div;
			div = (div >> 1);
		}

		if (timing == MMC_TIMING_MMC_HS400 &&
		    hz >= (host->src_clk_freq >> 1)) {
			if (host->dev_comp->clk_div_bits == 8)
				sdr_set_bits(host->base + MSDC_CFG,
					     MSDC_CFG_HS400_CK_MODE);
			else
				sdr_set_bits(host->base + MSDC_CFG,
					     MSDC_CFG_HS400_CK_MODE_EXTRA);
			sclk = host->src_clk_freq >> 1;
			div = 0; /* div is ignore when bit18 is set */
		}
	} else if (hz >= host->src_clk_freq) {
		mode = 0x1; /* no divisor */
		div = 0;
		sclk = host->src_clk_freq;
	} else {
		mode = 0x0; /* use divisor */
		if (hz >= (host->src_clk_freq >> 1)) {
			div = 0; /* mean div = 1/2 */
			sclk = host->src_clk_freq >> 1; /* sclk = clk / 2 */
		} else {
			div = (host->src_clk_freq + ((hz << 2) - 1)) /
					(hz << 2);
			sclk = (host->src_clk_freq >> 2) / div;
		}
	}
	sdr_clr_bits(host->base + MSDC_CFG, MSDC_CFG_CKPDN);
	/*
	 * As src_clk/HCLK use the same bit to gate/ungate,
	 * So if want to only gate src_clk, need gate its parent(mux).
	 */
	if (host->src_clk_cg)
		clk_disable_unprepare(host->src_clk_cg);
	else
		clk_disable_unprepare(clk_get_parent(host->src_clk));
	if (host->dev_comp->clk_div_bits == 8)
		sdr_set_field(host->base + MSDC_CFG,
			      MSDC_CFG_CKMOD | MSDC_CFG_CKDIV,
			      (mode << 8) | div);
	else
		sdr_set_field(host->base + MSDC_CFG,
			      MSDC_CFG_CKMOD_EXTRA | MSDC_CFG_CKDIV_EXTRA,
			      (mode << 12) | div);
	if (host->src_clk_cg)
		clk_prepare_enable(host->src_clk_cg);
	else
		clk_prepare_enable(clk_get_parent(host->src_clk));

	while (!(readl(host->base + MSDC_CFG) & MSDC_CFG_CKSTB))
		cpu_relax();
	sdr_set_bits(host->base + MSDC_CFG, MSDC_CFG_CKPDN);
	host->sclk = sclk;
	host->mclk = hz;
	host->timing = timing;
	/* need because clk changed. */
	msdc_set_timeout(host, host->timeout_ns, host->timeout_clks);
	sdr_set_bits(host->base + MSDC_INTEN, flags);

	/*
	 * mmc_select_hs400() will drop to 50Mhz and High speed mode,
	 * tune result of hs200/200Mhz is not suitable for 50Mhz
	 */
	if (host->sclk <= 52000000) {
		writel(host->def_tune_para.iocon, host->base + MSDC_IOCON);
		writel(host->def_tune_para.pad_tune, host->base + tune_reg);
		if (host->top_base != NULL) {
			writel(host->def_tune_para.sd_top_control,
			       host->top_base + SD_TOP_CONTROL);
			writel(host->def_tune_para.sd_top_cmd,
			       host->top_base + SD_TOP_CMD);
		}
	} else {
		writel(host->saved_tune_para.iocon, host->base + MSDC_IOCON);
		writel(host->saved_tune_para.pad_tune, host->base + tune_reg);
		writel(host->saved_tune_para.pad_cmd_tune,
		       host->base + PAD_CMD_TUNE);
		if (host->top_base != NULL) {
			writel(host->saved_tune_para.sd_top_control,
			       host->top_base + SD_TOP_CONTROL);
			writel(host->saved_tune_para.sd_top_cmd,
			       host->top_base + SD_TOP_CMD);
		}
	}

	if (timing == MMC_TIMING_MMC_HS400 &&
	    host->dev_comp->hs400_tune)
		sdr_set_field(host->base + PAD_CMD_TUNE,
			      MSDC_PAD_TUNE_CMDRRDLY,
			      host->hs400_cmd_int_delay);
	dev_info(host->dev, "sclk: %d, timing: %d\n", host->sclk, timing);
}

static inline u32 msdc_cmd_find_resp(struct msdc_host *host,
		struct mmc_request *mrq, struct mmc_command *cmd)
{
	u32 resp;

	switch (mmc_resp_type(cmd)) {
		/* Actually, R1, R5, R6, R7 are the same */
	case MMC_RSP_R1:
		resp = 0x1;
		break;
	case MMC_RSP_R1B:
		resp = 0x7;
		break;
	case MMC_RSP_R2:
		resp = 0x2;
		break;
	case MMC_RSP_R3:
		resp = 0x3;
		break;
	case MMC_RSP_NONE:
	default:
		resp = 0x0;
		break;
	}

	return resp;
}

static inline u32 msdc_cmd_prepare_raw_cmd(struct msdc_host *host,
		struct mmc_request *mrq, struct mmc_command *cmd)
{
	/* rawcmd :
	 * vol_swt << 30 | auto_cmd << 28 | blklen << 16 | go_irq << 15 |
	 * stop << 14 | rw << 13 | dtype << 11 | rsptyp << 7 | brk << 6 | opcode
	 */
	u32 opcode = cmd->opcode;
	u32 resp = msdc_cmd_find_resp(host, mrq, cmd);
	u32 rawcmd = (opcode & 0x3f) | ((resp & 0x7) << 7);

	host->cmd_rsp = resp;

	if ((opcode == SD_IO_RW_DIRECT && cmd->flags == (unsigned int) -1) ||
	    opcode == MMC_STOP_TRANSMISSION)
		rawcmd |= (0x1 << 14);
	else if (opcode == SD_SWITCH_VOLTAGE)
		rawcmd |= (0x1 << 30);
	else if (opcode == SD_APP_SEND_SCR ||
		 opcode == SD_APP_SEND_NUM_WR_BLKS ||
		 (opcode == SD_SWITCH &&
		(mmc_cmd_type(cmd) == MMC_CMD_ADTC)) ||
		 (opcode == SD_APP_SD_STATUS &&
		(mmc_cmd_type(cmd) == MMC_CMD_ADTC)) ||
		 (opcode == MMC_SEND_EXT_CSD &&
		(mmc_cmd_type(cmd) == MMC_CMD_ADTC)))
		rawcmd |= (0x1 << 11);

	if (cmd->data) {
		struct mmc_data *data = cmd->data;

		if (mmc_op_multi(opcode)) {
			if (mmc_card_mmc(host->mmc->card) && mrq->sbc &&
			    !(mrq->sbc->arg & 0xFFFF0000))
				rawcmd |= 0x2 << 28; /* AutoCMD23 */
		}

		rawcmd |= ((data->blksz & 0xFFF) << 16);
		if (data->flags & MMC_DATA_WRITE)
			rawcmd |= (0x1 << 13);
		if (data->blocks > 1)
			rawcmd |= (0x2 << 11);
		else
			rawcmd |= (0x1 << 11);

		/* Always use dma mode */
		sdr_clr_bits(host->base + MSDC_CFG, MSDC_CFG_PIO);

		if (host->timeout_ns != data->timeout_ns ||
		    host->timeout_clks != data->timeout_clks)
			msdc_set_timeout(host, data->timeout_ns,
					data->timeout_clks);

		writel(data->blocks, host->base + SDC_BLK_NUM);
	}
	return rawcmd;
}

static void msdc_start_data(struct msdc_host *host, struct mmc_request *mrq,
			    struct mmc_command *cmd, struct mmc_data *data)
{
	bool read;

	WARN_ON(host->data);
	host->data = data;
	read = data->flags & MMC_DATA_READ;

	mod_delayed_work(system_wq, &host->req_timeout, DAT_TIMEOUT);
	msdc_dma_setup(host, &host->dma, data);
	sdr_set_bits(host->base + MSDC_INTEN, data_ints_mask);
	sdr_set_field(host->base + MSDC_DMA_CTRL, MSDC_DMA_CTRL_START, 1);
	dev_dbg(host->dev, "DMA start\n");
	dev_dbg(host->dev, "%s: cmd=%d DMA data: %d blocks; read=%d\n",
			__func__, cmd->opcode, data->blocks, read);
}

static int msdc_auto_cmd_done(struct msdc_host *host, int events,
		struct mmc_command *cmd)
{
	u32 *rsp = cmd->resp;

	rsp[0] = readl(host->base + SDC_ACMD_RESP);

	if (events & MSDC_INT_ACMDRDY) {
		cmd->error = 0;
	} else {
		msdc_reset_hw(host);
		if (events & MSDC_INT_ACMDCRCERR) {
			cmd->error = -EILSEQ;
			host->error |= REQ_STOP_EIO;
		} else if (events & MSDC_INT_ACMDTMO) {
			cmd->error = -ETIMEDOUT;
			host->error |= REQ_STOP_TMO;
		}
		dev_info(host->dev,
			"%s: AUTO_CMD%d arg=%08X; rsp %08X; cmd_error=%d\n",
			__func__, cmd->opcode, cmd->arg, rsp[0], cmd->error);
	}
	return cmd->error;
}

static void msdc_track_cmd_data(struct msdc_host *host,
				struct mmc_command *cmd, struct mmc_data *data)
{
	if (host->error) {
		dev_info(host->dev, "%s: cmd=%d arg=%08X; host->error=0x%08X\n",
			__func__, cmd->opcode, cmd->arg, host->error);

		/* Dump all register for CRC error, but only once */
		if (host->should_dump_register &&
			host->error != REQ_CMD_TMO &&
			cmd->opcode != MMC_SEND_TUNING_BLOCK) {

			msdc_dump_all_register(host);
			host->should_dump_register = false;
		}
	}
}

static void msdc_request_done(struct msdc_host *host, struct mmc_request *mrq)
{
	unsigned long flags;
	bool ret;

	ret = cancel_delayed_work(&host->req_timeout);
	if (!ret) {
		/* delay work already running */
		return;
	}
	spin_lock_irqsave(&host->lock, flags);
	host->mrq = NULL;
	spin_unlock_irqrestore(&host->lock, flags);

	msdc_track_cmd_data(host, mrq->cmd, mrq->data);
	if (mrq->data)
		msdc_unprepare_data(host, mrq);
	if (host->error)
		msdc_reset_hw(host);
	mmc_request_done(host->mmc, mrq);
}

/* returns true if command is fully handled; returns false otherwise */
static bool msdc_cmd_done(struct msdc_host *host, int events,
			  struct mmc_request *mrq, struct mmc_command *cmd)
{
	bool done = false;
	bool sbc_error;
	unsigned long flags;
	u32 *rsp = cmd->resp;

	if (mrq->sbc && cmd == mrq->cmd &&
	    (events & (MSDC_INT_ACMDRDY | MSDC_INT_ACMDCRCERR
				   | MSDC_INT_ACMDTMO)))
		msdc_auto_cmd_done(host, events, mrq->sbc);

	sbc_error = mrq->sbc && mrq->sbc->error;

	if (!sbc_error && !(events & (MSDC_INT_CMDRDY
					| MSDC_INT_RSPCRCERR
					| MSDC_INT_CMDTMO)))
		return done;

	spin_lock_irqsave(&host->lock, flags);
	done = !host->cmd;
	host->cmd = NULL;
	spin_unlock_irqrestore(&host->lock, flags);

	if (done)
		return true;

	spin_lock_irqsave(&host->lock, flags);
	sdr_clr_bits(host->base + MSDC_INTEN, cmd_ints_mask);
	spin_unlock_irqrestore(&host->lock, flags);

	if (cmd->flags & MMC_RSP_PRESENT) {
		if (cmd->flags & MMC_RSP_136) {
			rsp[0] = readl(host->base + SDC_RESP3);
			rsp[1] = readl(host->base + SDC_RESP2);
			rsp[2] = readl(host->base + SDC_RESP1);
			rsp[3] = readl(host->base + SDC_RESP0);
		} else {
			rsp[0] = readl(host->base + SDC_RESP0);
		}
	}

	if (!sbc_error && !(events & MSDC_INT_CMDRDY)) {
		if (cmd->opcode != MMC_SEND_TUNING_BLOCK &&
		    cmd->opcode != MMC_SEND_TUNING_BLOCK_HS200)
			/*
			 * should not clear fifo/interrupt as the tune data
			 * may have already come.
			 */
			msdc_reset_hw(host);
		if (events & MSDC_INT_RSPCRCERR) {
			cmd->error = -EILSEQ;
			host->error |= REQ_CMD_EIO;
		} else if (events & MSDC_INT_CMDTMO) {
			cmd->error = -ETIMEDOUT;
			host->error |= REQ_CMD_TMO;
		}
	}
	if (cmd->error &&
	    cmd->opcode != MMC_SEND_TUNING_BLOCK &&
	    cmd->opcode != MMC_SEND_TUNING_BLOCK_HS200)
		dev_info(host->dev,
				"%s: cmd=%d arg=%08X; rsp %08X; cmd_error=%d\n",
				__func__, cmd->opcode, cmd->arg, rsp[0],
				cmd->error);

	msdc_cmd_next(host, mrq, cmd);
	return true;
}

/* It is the core layer's responsibility to ensure card status
 * is correct before issue a request. but host design do below
 * checks recommended.
 */
static inline bool msdc_cmd_is_ready(struct msdc_host *host,
		struct mmc_request *mrq, struct mmc_command *cmd)
{
	/* The max busy time we can endure is 20ms */
	unsigned long tmo = jiffies + msecs_to_jiffies(20);

	while ((readl(host->base + SDC_STS) & SDC_STS_CMDBUSY) &&
			time_before(jiffies, tmo))
		cpu_relax();
	if (readl(host->base + SDC_STS) & SDC_STS_CMDBUSY) {
		dev_info(host->dev, "CMD bus busy detected\n");
		host->error |= REQ_CMD_BUSY;
		msdc_cmd_done(host, MSDC_INT_CMDTMO, mrq, cmd);
		return false;
	}

	if (mmc_resp_type(cmd) == MMC_RSP_R1B || cmd->data) {
		tmo = jiffies + msecs_to_jiffies(20);
		/* R1B or with data, should check SDCBUSY */
		while ((readl(host->base + SDC_STS) & SDC_STS_SDCBUSY) &&
				time_before(jiffies, tmo))
			cpu_relax();
		if (readl(host->base + SDC_STS) & SDC_STS_SDCBUSY) {
			dev_info(host->dev, "Controller busy detected\n");
			host->error |= REQ_CMD_BUSY;
			msdc_cmd_done(host, MSDC_INT_CMDTMO, mrq, cmd);
			return false;
		}
	}
	return true;
}

static void msdc_start_command(struct msdc_host *host,
		struct mmc_request *mrq, struct mmc_command *cmd)
{
	u32 rawcmd;
	unsigned long flags;

	WARN_ON(host->cmd);
	host->cmd = cmd;

	if (!msdc_cmd_is_ready(host, mrq, cmd))
		return;

	if ((readl(host->base + MSDC_FIFOCS) & MSDC_FIFOCS_TXCNT) >> 16 ||
	    readl(host->base + MSDC_FIFOCS) & MSDC_FIFOCS_RXCNT) {
		dev_info(host->dev, "TX/RX FIFO non-empty before start of IO. Reset\n");
		msdc_reset_hw(host);
	}

	cmd->error = 0;
	rawcmd = msdc_cmd_prepare_raw_cmd(host, mrq, cmd);
	mod_delayed_work(system_wq, &host->req_timeout, DAT_TIMEOUT);

	spin_lock_irqsave(&host->lock, flags);
	sdr_set_bits(host->base + MSDC_INTEN, cmd_ints_mask);
	spin_unlock_irqrestore(&host->lock, flags);

	writel(cmd->arg, host->base + SDC_ARG);
	writel(rawcmd, host->base + SDC_CMD);
}

static void msdc_cmd_next(struct msdc_host *host,
		struct mmc_request *mrq, struct mmc_command *cmd)
{
	if ((cmd->error &&
	    !(cmd->error == -EILSEQ &&
	      (cmd->opcode == MMC_SEND_TUNING_BLOCK ||
	       cmd->opcode == MMC_SEND_TUNING_BLOCK_HS200))) ||
	    (mrq->sbc && mrq->sbc->error))
		msdc_request_done(host, mrq);
	else if (cmd == mrq->sbc)
		msdc_start_command(host, mrq, mrq->cmd);
	else if (!cmd->data)
		msdc_request_done(host, mrq);
	else
		msdc_start_data(host, mrq, cmd, cmd->data);
}

static void msdc_ops_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct msdc_host *host = mmc_priv(mmc);

	host->error = 0;
	WARN_ON(host->mrq);
	host->mrq = mrq;

	if (mrq->data)
		msdc_prepare_data(host, mrq);

	/* if SBC is required, we have HW option and SW option.
	 * if HW option is enabled, and SBC does not have "special" flags,
	 * use HW option,  otherwise use SW option
	 */
	if (mrq->sbc && (!mmc_card_mmc(mmc->card) ||
	    (mrq->sbc->arg & 0xFFFF0000)))
		msdc_start_command(host, mrq, mrq->sbc);
	else
		msdc_start_command(host, mrq, mrq->cmd);
}

static void msdc_pre_req(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct msdc_host *host = mmc_priv(mmc);
	struct mmc_data *data = mrq->data;

	if (!data)
		return;

	msdc_prepare_data(host, mrq);
	data->host_cookie |= MSDC_ASYNC_FLAG;
}

static void msdc_post_req(struct mmc_host *mmc, struct mmc_request *mrq,
		int err)
{
	struct msdc_host *host = mmc_priv(mmc);
	struct mmc_data *data;

	data = mrq->data;
	if (!data)
		return;
	if (data->host_cookie) {
		data->host_cookie &= ~MSDC_ASYNC_FLAG;
		msdc_unprepare_data(host, mrq);
	}
}

static void msdc_data_xfer_next(struct msdc_host *host,
				struct mmc_request *mrq, struct mmc_data *data)
{
	if (mmc_op_multi(mrq->cmd->opcode) && mrq->stop && !mrq->stop->error &&
	    !mrq->sbc)
		msdc_start_command(host, mrq, mrq->stop);
	else
		msdc_request_done(host, mrq);
}

static bool msdc_data_xfer_done(struct msdc_host *host, u32 events,
				struct mmc_request *mrq, struct mmc_data *data)
{
	struct mmc_command *stop = data->stop;
	unsigned long flags;
	bool done;
	unsigned int check_data = events &
	    (MSDC_INT_XFER_COMPL | MSDC_INT_DATCRCERR | MSDC_INT_DATTMO
	     | MSDC_INT_DMA_BDCSERR | MSDC_INT_DMA_GPDCSERR
	     | MSDC_INT_DMA_PROTECT);

	spin_lock_irqsave(&host->lock, flags);
	done = !host->data;
	if (check_data)
		host->data = NULL;
	spin_unlock_irqrestore(&host->lock, flags);

	if (done)
		return true;

	if (check_data || (stop && stop->error)) {
		dev_dbg(host->dev, "DMA status: 0x%8X\n",
				readl(host->base + MSDC_DMA_CFG));
		sdr_set_field(host->base + MSDC_DMA_CTRL, MSDC_DMA_CTRL_STOP,
				1);
		while (readl(host->base + MSDC_DMA_CFG) & MSDC_DMA_CFG_STS)
			cpu_relax();
		spin_lock_irqsave(&host->lock, flags);
		sdr_clr_bits(host->base + MSDC_INTEN, data_ints_mask);
		spin_unlock_irqrestore(&host->lock, flags);
		dev_dbg(host->dev, "DMA stop\n");

		if ((events & MSDC_INT_XFER_COMPL) && (!stop || !stop->error)) {
			data->bytes_xfered = data->blocks * data->blksz;
		} else {
			dev_dbg(host->dev, "interrupt events: %x\n", events);
			msdc_reset_hw(host);
			host->error |= REQ_DAT_ERR;
			data->bytes_xfered = 0;

			if (events & MSDC_INT_DATTMO)
				data->error = -ETIMEDOUT;
			else if (events & MSDC_INT_DATCRCERR)
				data->error = -EILSEQ;
			if (mrq->cmd->opcode != MMC_SEND_TUNING_BLOCK &&
			    mrq->cmd->opcode != MMC_SEND_TUNING_BLOCK_HS200) {
				dev_info(host->dev, "%s: cmd=%d; blocks=%d",
					__func__, mrq->cmd->opcode,
					data->blocks);
				dev_info(host->dev, "data_error=%d xfer_size=%d\n",
					(int)data->error, data->bytes_xfered);
			}
		}

		msdc_data_xfer_next(host, mrq, data);
		done = true;
	}
	return done;
}

static void msdc_set_buswidth(struct msdc_host *host, u32 width)
{
	u32 val = readl(host->base + SDC_CFG);

	val &= ~SDC_CFG_BUSWIDTH;

	switch (width) {
	default:
	case MMC_BUS_WIDTH_1:
		val |= (MSDC_BUS_1BITS << 16);
		break;
	case MMC_BUS_WIDTH_4:
		val |= (MSDC_BUS_4BITS << 16);
		break;
	case MMC_BUS_WIDTH_8:
		val |= (MSDC_BUS_8BITS << 16);
		break;
	}

	writel(val, host->base + SDC_CFG);
	dev_dbg(host->dev, "Bus Width = %d", width);
}

static int msdc_ops_switch_volt(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct msdc_host *host = mmc_priv(mmc);
	int ret = 0;

	if (!IS_ERR(mmc->supply.vqmmc)) {
		if (ios->signal_voltage != MMC_SIGNAL_VOLTAGE_330 &&
		    ios->signal_voltage != MMC_SIGNAL_VOLTAGE_180) {
			dev_info(host->dev, "Unsupported signal voltage!\n");
			return -EINVAL;
		}

		ret = mmc_regulator_set_vqmmc(mmc, ios);
		if (ret < 0) {
			dev_dbg(host->dev, "Regulator set error %d (%d)\n",
				ret, ios->signal_voltage);
		} else {
			/* different pinctrl settings for different voltage */
			if (ios->signal_voltage == MMC_SIGNAL_VOLTAGE_180)
				pinctrl_select_state(host->pinctrl,
						host->pins_uhs);
			else
				pinctrl_select_state(host->pinctrl,
						host->pins_default);
		}
	}

	return (ret < 0) ? ret : 0;
}

static int msdc_card_busy(struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);
	u32 status = readl(host->base + MSDC_PS);

	/* only check if data0 is low */
	return !(status & BIT(16));
}

static void msdc_request_timeout(struct work_struct *work)
{
	struct msdc_host *host = container_of(work, struct msdc_host,
			req_timeout.work);

	/* simulate HW timeout status */
	dev_info(host->dev, "%s: aborting cmd/data/mrq\n", __func__);
	if (host->mrq) {
		dev_info(host->dev, "%s: aborting mrq=%p cmd=%d\n", __func__,
				host->mrq, host->mrq->cmd->opcode);
		if (host->cmd) {
			dev_info(host->dev, "%s: aborting cmd=%d\n",
					__func__, host->cmd->opcode);
			msdc_cmd_done(host, MSDC_INT_CMDTMO, host->mrq,
					host->cmd);
		} else if (host->data) {
			dev_info(host->dev, "%s: abort data: cmd%d; %d blocks\n",
					__func__, host->mrq->cmd->opcode,
					host->data->blocks);
			msdc_data_xfer_done(host, MSDC_INT_DATTMO, host->mrq,
					host->data);
		}
	}
}

static void __msdc_enable_sdio_irq(struct mmc_host *mmc, int enb)
{
	unsigned long flags;
	struct msdc_host *host = mmc_priv(mmc);

	spin_lock_irqsave(&host->lock, flags);
	if (enb) {
		sdr_set_bits(host->base + MSDC_INTEN, MSDC_INTEN_SDIOIRQ);
		sdr_set_bits(host->base + SDC_CFG, SDC_CFG_SDIOIDE);
	} else
		sdr_clr_bits(host->base + MSDC_INTEN, MSDC_INTEN_SDIOIRQ);
	spin_unlock_irqrestore(&host->lock, flags);
}

static void msdc_enable_sdio_irq(struct mmc_host *mmc, int enb)
{
	struct msdc_host *host = mmc_priv(mmc);

	__msdc_enable_sdio_irq(mmc, enb);

	if (mmc->card && (mmc->card->cccr.enable_async_int == 0)) {
		if (enb)
			pm_runtime_get_noresume(host->dev);
		else
			pm_runtime_put_noidle(host->dev);
	}
}

static irqreturn_t msdc_irq(int irq, void *dev_id)
{
	struct msdc_host *host = (struct msdc_host *) dev_id;

	while (true) {
		unsigned long flags;
		struct mmc_request *mrq;
		struct mmc_command *cmd;
		struct mmc_data *data;
		u32 events, event_mask;

		spin_lock_irqsave(&host->lock, flags);
		events = readl(host->base + MSDC_INT);
		event_mask = readl(host->base + MSDC_INTEN);
		if ((events & event_mask) & MSDC_INT_SDIOIRQ)
			sdr_clr_bits(host->base + SDC_CFG, SDC_CFG_SDIOIDE);
		/* clear interrupts */
		writel(events & event_mask, host->base + MSDC_INT);

		mrq = host->mrq;
		cmd = host->cmd;
		data = host->data;
		spin_unlock_irqrestore(&host->lock, flags);

		if ((events & event_mask) & MSDC_INT_SDIOIRQ) {
			__msdc_enable_sdio_irq(host->mmc, 0);
			sdio_signal_irq(host->mmc);
		}

		if (!(events & (event_mask & ~MSDC_INT_SDIOIRQ)))
			break;

		if (!mrq) {
			dev_info(host->dev,
				"%s: MRQ=NULL; events=%08X; event_mask=%08X\n",
				__func__, events, event_mask);
			WARN_ON(1);
			break;
		}

		dev_dbg(host->dev, "%s: events=%08X\n", __func__, events);

		if (cmd)
			msdc_cmd_done(host, events, mrq, cmd);
		else if (data)
			msdc_data_xfer_done(host, events, mrq, data);
	}

	return IRQ_HANDLED;
}

static void msdc_init_hw(struct msdc_host *host)
{
	u32 val;
	u32 tune_reg = host->dev_comp->pad_tune_reg;

	/* Configure to MMC/SD mode */
	sdr_set_bits(host->base + MSDC_CFG, MSDC_CFG_MODE);

	/* Reset */
	msdc_reset_hw(host);

	/* Disable card detection */
	sdr_clr_bits(host->base + MSDC_PS, MSDC_PS_CDEN);

	/* Disable and clear all interrupts */
	writel(0, host->base + MSDC_INTEN);
	val = readl(host->base + MSDC_INT);
	writel(val, host->base + MSDC_INT);

	writel(0, host->base + tune_reg);
	if (host->top_base != NULL) {
		writel(0, host->top_base + SD_TOP_CONTROL);
		writel(0, host->top_base + SD_TOP_CMD);
	}
	writel(0, host->base + MSDC_IOCON);
	sdr_set_field(host->base + MSDC_IOCON, MSDC_IOCON_DDLSEL, 0);
	writel(0x403c0046, host->base + MSDC_PATCH_BIT0);
	sdr_set_field(host->base + MSDC_PATCH_BIT0,
		MSDC_PB0_CKGEN_MSDC_DLY_SEL, 1);
	writel(0xffff4089, host->base + MSDC_PATCH_BIT1);
	sdr_set_bits(host->base + EMMC50_CFG0, EMMC50_CFG_CFCSTS_SEL);

	/* For SDIO3.0+ IP, this bit should be set to 0 */
	if (host->dev_comp->v3_plus)
		sdr_clr_bits(host->base + MSDC_PATCH_BIT1,
			MSDC_PB1_SINGLE_BURST);

	if (host->dev_comp->stop_clk_fix) {
		sdr_set_field(host->base + MSDC_PATCH_BIT1,
			      MSDC_PB1_STOP_DLY_SEL, 3);
		sdr_clr_bits(host->base + SDC_FIFO_CFG,
			     SDC_FIFO_CFG_WRVALIDSEL);
		sdr_clr_bits(host->base + SDC_FIFO_CFG,
			     SDC_FIFO_CFG_RDVALIDSEL);
	}

	if (host->dev_comp->busy_check)
		sdr_clr_bits(host->base + MSDC_PATCH_BIT1, (1 << 7));

	if (host->dev_comp->async_fifo) {
		sdr_set_field(host->base + MSDC_PATCH_BIT2,
			      MSDC_PB2_RESPWAITCNT, 3);
		if (host->dev_comp->enhance_rx) {
			if (host->top_base != NULL)
				sdr_set_bits(host->top_base + SD_TOP_CONTROL,
					     SDC_RX_ENH_EN);
			else
				sdr_set_bits(host->base + SDC_ADV_CFG0,
					     SDC_RX_ENHANCE_EN);
		} else {
			sdr_set_field(host->base + MSDC_PATCH_BIT2,
				      MSDC_PB2_RESPSTENSEL, 2);
			sdr_set_field(host->base + MSDC_PATCH_BIT2,
				      MSDC_PB2_CRCSTSENSEL, 2);
		}
		/* use async fifo, then no need tune internal delay */
		sdr_clr_bits(host->base + MSDC_PATCH_BIT2,
			     MSDC_PB2_CFGRESP);
		sdr_set_bits(host->base + MSDC_PATCH_BIT2,
			     MSDC_PB2_CFGCRCSTS);
	}

	if (host->dev_comp->support_64g)
		sdr_set_bits(host->base + MSDC_PATCH_BIT2,
			     MSDC_PB2_SUPPORT64G);
	if (host->dev_comp->data_tune) {
		if (host->top_base != NULL) {
			sdr_set_bits(host->top_base + SD_TOP_CONTROL,
				     PAD_DAT_RD_RXDLY_SEL);
			sdr_clr_bits(host->top_base + SD_TOP_CONTROL,
				     DATA_K_VALUE_SEL);
			sdr_set_bits(host->top_base + SD_TOP_CMD,
				     PAD_CMD_RD_RXDLY_SEL);

			if (host->dev_comp->tune_64_step) {
				sdr_set_bits(host->top_base + SD_TOP_CONTROL,
					     PAD_DAT_RD_RXDLY2_SEL);
				sdr_set_bits(host->top_base + SD_TOP_CMD,
					     PAD_CMD_RD_RXDLY2_SEL);
			}
		} else {
			sdr_set_bits(host->base + tune_reg,
				     MSDC_PAD_TUNE_RD_SEL |
				     MSDC_PAD_TUNE_CMD_SEL);
		}
	} else {
		/* choose clock tune */
		if (host->top_base != NULL)
			sdr_set_bits(host->top_base + SD_TOP_CONTROL,
				     PAD_RXDLY_SEL);
		else
			sdr_set_bits(host->base + tune_reg,
				     MSDC_PAD_TUNE_RXDLYSEL);
	}

	/* For SDIO3.0+ IP, this bit should be set to 0 */
	if (host->dev_comp->v3_plus)
		sdr_clr_bits(host->base + MSDC_PATCH_BIT1,
			MSDC_PB1_SINGLE_BURST);

	/* Configure to enable SDIO mode.
	 * it's must otherwise sdio cmd5 failed
	 */
	sdr_set_bits(host->base + SDC_CFG, SDC_CFG_SDIO);

	/* Config SDIO device detect interrupt function */
	sdr_clr_bits(host->base + SDC_CFG, SDC_CFG_SDIOIDE);

	sdr_set_bits(host->base + SDC_ADV_CFG0, SDC_IRQ_ENHANCE_EN);

	/* For SDIO which do not support INCR1 */
	if (host->no_sdio_incr1)
		sdr_set_bits(host->base + MSDC_PATCH_BIT1,
			MSDC_PB1_SINGLE_BURST);

	/* Configure to default data timeout */
	sdr_set_field(host->base + SDC_CFG, SDC_CFG_DTOC, 3);

	host->def_tune_para.iocon = readl(host->base + MSDC_IOCON);
	host->def_tune_para.pad_tune = readl(host->base + tune_reg);
	host->saved_tune_para.iocon = readl(host->base + MSDC_IOCON);
	host->saved_tune_para.pad_tune = readl(host->base + tune_reg);
	if (host->top_base != NULL) {
		host->def_tune_para.sd_top_control =
			readl(host->top_base + SD_TOP_CONTROL);
		host->def_tune_para.sd_top_cmd =
			readl(host->top_base + SD_TOP_CMD);
		host->saved_tune_para.sd_top_control =
			readl(host->top_base + SD_TOP_CONTROL);
		host->saved_tune_para.sd_top_cmd =
			readl(host->top_base + SD_TOP_CMD);
	}
	dev_dbg(host->dev, "init hardware done!");
}

static void msdc_deinit_hw(struct msdc_host *host)
{
	u32 val;
	/* Disable and clear all interrupts */
	writel(0, host->base + MSDC_INTEN);

	val = readl(host->base + MSDC_INT);
	writel(val, host->base + MSDC_INT);
}

/* init gpd and bd list in msdc_drv_probe */
static void msdc_init_gpd_bd(struct msdc_host *host, struct msdc_dma *dma)
{
	struct mt_gpdma_desc *gpd = dma->gpd;
	struct mt_bdma_desc *bd = dma->bd;
	dma_addr_t dma_addr;
	int i;

	memset(gpd, 0, sizeof(struct mt_gpdma_desc) * 2);

	dma_addr = dma->gpd_addr + sizeof(struct mt_gpdma_desc);
	gpd->gpd_info = GPDMA_DESC_BDP; /* hwo, cs, bd pointer */
	/* gpd->next is must set for desc DMA
	 * That's why must alloc 2 gpd structure.
	 */
	gpd->next = lower_32_bits(dma_addr);
	if (host->dev_comp->support_64g)
		gpd->gpd_info |= (upper_32_bits(dma_addr) & 0xf) << 24;

	dma_addr = dma->bd_addr;
	gpd->ptr = lower_32_bits(dma->bd_addr); /* physical address */
	if (host->dev_comp->support_64g)
		gpd->gpd_info |= (upper_32_bits(dma_addr) & 0xf) << 28;

	memset(bd, 0, sizeof(struct mt_bdma_desc) * MAX_BD_NUM);
	for (i = 0; i < (MAX_BD_NUM - 1); i++) {
		dma_addr = dma->bd_addr + sizeof(*bd) * (i + 1);
		bd[i].next = lower_32_bits(dma_addr);
		if (host->dev_comp->support_64g)
			bd[i].bd_info |= (upper_32_bits(dma_addr) & 0xf) << 24;
	}
}

static void msdc_ops_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct msdc_host *host = mmc_priv(mmc);
	int ret;

	msdc_set_buswidth(host, ios->bus_width);

	/* Suspend/Resume will do power off/on */
	switch (ios->power_mode) {
	case MMC_POWER_UP:
		if (!IS_ERR(mmc->supply.vmmc)) {
			msdc_init_hw(host);
			ret = mmc_regulator_set_ocr(mmc, mmc->supply.vmmc,
					ios->vdd);
			if (ret) {
				dev_info(host->dev, "Failed to set vmmc power!\n");
				return;
			}
		}
		break;
	case MMC_POWER_ON:
		if (!IS_ERR(mmc->supply.vqmmc) && !host->vqmmc_enabled) {
			ret = regulator_enable(mmc->supply.vqmmc);
			if (ret)
				dev_info(host->dev, "Failed to set vqmmc power!\n");
			else
				host->vqmmc_enabled = true;
		}
		break;
	case MMC_POWER_OFF:
		if (!IS_ERR(mmc->supply.vmmc))
			mmc_regulator_set_ocr(mmc, mmc->supply.vmmc, 0);

		if (!IS_ERR(mmc->supply.vqmmc) && host->vqmmc_enabled) {
			regulator_disable(mmc->supply.vqmmc);
			host->vqmmc_enabled = false;
		}
		break;
	default:
		break;
	}

	if (host->mclk != ios->clock || host->timing != ios->timing)
		msdc_set_mclk(host, ios->timing, ios->clock);
}

static u64 test_delay_bit(u64 delay, u32 bit)
{
	bit %= TUNE_STEP_MAX;
	return delay & (1ULL << bit);
}

static int get_delay_len(u64 delay, u32 start_bit)
{
	int i;

	for (i = 0; i < (TUNE_STEP_MAX - start_bit); i++) {
		if (test_delay_bit(delay, start_bit + i) == 0)
			return i;
	}
	return TUNE_STEP_MAX - start_bit;
}

static struct msdc_delay_phase get_best_delay(struct msdc_host *host, u64 delay)
{
	int start = 0, len = 0;
	int start_final = 0, len_final = 0;
	u8 final_phase = 0xff;
	struct msdc_delay_phase delay_phase = { 0, };

	if (delay == 0) {
		dev_info(host->dev, "phase error: [map: %016llx]\n", delay);
		delay_phase.final_phase = final_phase;
		return delay_phase;
	}

	while (start < TUNE_STEP_MAX) {
		len = get_delay_len(delay, start);
		if (len_final < len) {
			start_final = start;
			len_final = len;
		}
		start += len ? len : 1;
		if (!host->dev_comp->tune_64_step) {
			if (len >= 12 && start_final < 4)
				break;
		}
	}

	/* The rule is that to find the smallest delay cell */
	if (start_final == 0)
		final_phase = (start_final + len_final / 3) % TUNE_STEP_MAX;
	else
		final_phase = (start_final + len_final / 2) % TUNE_STEP_MAX;
	dev_info(host->dev, "phase: [map: %016llx] [maxlen:%d] [final:%d]\n",
		 delay, len_final, final_phase);

	delay_phase.maxlen = len_final;
	delay_phase.start = start_final;
	delay_phase.final_phase = final_phase;
	return delay_phase;
}

/*
 * dly_line: 0 --> delay line 0
 * dly_line: 1 --> delay line 1
 */
static inline void msdc_set_cmd_delay(struct msdc_host *host, u32 value,
				      u8 dly_line)
{
	u32 tune_reg = host->dev_comp->pad_tune_reg;

	if (host->top_base) {
		sdr_set_field(host->top_base + SD_TOP_CMD,
			      dly_line ? PAD_CMD_RXDLY2 : PAD_CMD_RXDLY, value);
	} else {
		if (dly_line)
			sdr_set_field(host->base + tune_reg + 4,
				      MSDC_PAD_TUNE_CMDRDLY2, value);
		else
			sdr_set_field(host->base + tune_reg,
				      MSDC_PAD_TUNE_CMDRDLY, value);
	}
}

static inline void msdc_set_data_delay(struct msdc_host *host, u32 value,
				       u8 dly_line)
{
	u32 tune_reg = host->dev_comp->pad_tune_reg;

	if (host->top_base) {
		sdr_set_field(host->top_base + SD_TOP_CONTROL,
			      dly_line ? PAD_DAT_RD_RXDLY2 : PAD_DAT_RD_RXDLY,
			      value);
	} else {
		if (dly_line)
			sdr_set_field(host->base + tune_reg + 4,
				      MSDC_PAD_TUNE_DATRRDLY2, value);
		else
			sdr_set_field(host->base + tune_reg,
				      MSDC_PAD_TUNE_DATRRDLY, value);
	}
}

static void msdc_set_final_window(struct msdc_host *host, u8 delay, u8 edge)
{
	u32 dly1, dly2 = 0;

	dly1 = delay > 31 ? 31 : delay;
	dly2 = delay > 31 ? delay - 31 : 0;

	sdr_set_field(host->base + MSDC_IOCON, MSDC_IOCON_RSPL, edge);
	sdr_set_field(host->base + MSDC_IOCON, MSDC_IOCON_DSPL, edge);
	sdr_set_field(host->base + MSDC_IOCON, MSDC_IOCON_W_DSPL, edge);

	msdc_set_cmd_delay(host, dly1, 0);
	msdc_set_data_delay(host, dly1, 0);
	msdc_set_cmd_delay(host, dly2, 1);
	msdc_set_data_delay(host, dly2, 1);
}

static int msdc_tune_resp_data(struct mmc_host *mmc, u32 opcode)
{
	struct msdc_host *host = mmc_priv(mmc);
	u64 rise_delay = 0, fall_delay = 0;
	struct msdc_delay_phase final_rise_delay, final_fall_delay = { 0,};
	u8 final_delay, final_maxlen, final_edge;
	int err;
	int i, j;

	sdr_set_field(host->base + MSDC_PATCH_BIT0,
			MSDC_PB0_INT_DAT_LATCH_CK_SEL,
			host->latch_ck);
	sdr_clr_bits(host->base + MSDC_IOCON, MSDC_IOCON_RSPL);
	sdr_clr_bits(host->base + MSDC_IOCON, MSDC_IOCON_DSPL);
	sdr_clr_bits(host->base + MSDC_IOCON, MSDC_IOCON_W_DSPL);

	/* clear RX DLY2 in case it is not the 1st tuning */
	msdc_set_cmd_delay(host, 0, 1);
	msdc_set_data_delay(host, 0, 1);

	for (i = 0 ; i < PAD_DELAY_MAX; i++) {
		msdc_set_cmd_delay(host, i, 0);
		msdc_set_data_delay(host, i, 0);
		/*
		 * Using the same parameters, it may sometimes pass the test,
		 * but sometimes it may fail. To make sure the parameters are
		 * more stable, we test each set of parameters 3 times.
		 */
		for (j = 0; j < 3; j++) {
			err = mmc_send_tuning(mmc, opcode, NULL);
			if (!err) {
				rise_delay |= (1ULL << i);
			} else {
				rise_delay &= ~(1ULL << i);
				break;
			}
		}
	}

	if (host->dev_comp->tune_64_step) {
		for (i = 0 ; i < PAD_DELAY_MAX; i++) {
			msdc_set_cmd_delay(host, i, 1);
			msdc_set_data_delay(host, i, 1);
			for (j = 0; j < 3; j++) {
				err = mmc_send_tuning(mmc, opcode, NULL);
				if (!err) {
					rise_delay |= (1ULL << (i + 32));
				} else {
					rise_delay &= ~(1ULL << (i + 32));
					break;
				}
			}
		}
	}

	final_rise_delay = get_best_delay(host, rise_delay);
	/* if rising edge has enough margin, then do not scan falling edge */
	if (final_rise_delay.maxlen >= 12 ||
	    (final_rise_delay.start == 0 && final_rise_delay.maxlen >= 4))
		goto skip_fall;

	sdr_set_bits(host->base + MSDC_IOCON, MSDC_IOCON_RSPL);
	sdr_set_bits(host->base + MSDC_IOCON, MSDC_IOCON_DSPL);
	sdr_set_bits(host->base + MSDC_IOCON, MSDC_IOCON_W_DSPL);
	for (i = 0; i < PAD_DELAY_MAX; i++) {
		msdc_set_cmd_delay(host, i, 0);
		msdc_set_data_delay(host, i, 0);

		for (j = 0; j < 3; j++) {
			err = mmc_send_tuning(mmc, opcode, NULL);
			if (!err) {
				fall_delay |= (1ULL << i);
			} else {
				fall_delay &= ~(1ULL << i);
				break;
			}
		}
	}
	if (host->dev_comp->tune_64_step) {
		for (i = 0 ; i < PAD_DELAY_MAX; i++) {
			msdc_set_cmd_delay(host, i, 1);
			msdc_set_data_delay(host, i, 1);
			for (j = 0; j < 3; j++) {
				err = mmc_send_tuning(mmc, opcode, NULL);
				if (!err) {
					fall_delay |= (1ULL << (i + 32));
				} else {
					fall_delay &= ~(1ULL << (i + 32));
					break;
				}
			}
		}
	}
	final_fall_delay = get_best_delay(host, fall_delay);

skip_fall:
	final_maxlen = max(final_rise_delay.maxlen, final_fall_delay.maxlen);
	if (final_fall_delay.maxlen >= 12 && final_fall_delay.start < 4)
		final_maxlen = final_fall_delay.maxlen;

	if (final_maxlen == final_rise_delay.maxlen) {
		final_delay = final_rise_delay.final_phase;
		final_edge = 0;
	} else {
		final_delay = final_fall_delay.final_phase;
		final_edge = 1;
	}
	msdc_set_final_window(host, final_delay, final_edge);

	dev_info(host->dev, "Final cmd/data pad delay: %x\n", final_delay);
	return final_delay == 0xff ? -EIO : 0;
}

static int msdc_tune_response(struct mmc_host *mmc, u32 opcode)
{
	struct msdc_host *host = mmc_priv(mmc);
	u64 rise_delay = 0, fall_delay = 0;
	struct msdc_delay_phase final_rise_delay, final_fall_delay = { 0, };
	struct msdc_delay_phase internal_delay_phase = { 0, };
	u8 final_delay, final_maxlen;
	u32 internal_delay = 0;
	u32 tune_reg = host->dev_comp->pad_tune_reg;
	int cmd_err;
	int i, j;

	if (mmc->ios.timing == MMC_TIMING_MMC_HS200 ||
	    mmc->ios.timing == MMC_TIMING_UHS_SDR104)
		sdr_set_field(host->base + tune_reg,
			      MSDC_PAD_TUNE_CMDRRDLY,
			      host->hs200_cmd_int_delay);

	sdr_clr_bits(host->base + MSDC_IOCON, MSDC_IOCON_RSPL);
	for (i = 0 ; i < PAD_DELAY_MAX; i++) {
		msdc_set_cmd_delay(host, i, 0);
		/*
		 * Using the same parameters, it may sometimes pass the test,
		 * but sometimes it may fail. To make sure the parameters are
		 * more stable, we test each set of parameters 3 times.
		 */
		for (j = 0; j < 3; j++) {
			mmc_send_tuning(mmc, opcode, &cmd_err);
			if (!cmd_err) {
				rise_delay |= (1ULL << i);
			} else {
				rise_delay &= ~(1ULL << i);
				break;
			}
		}
	}
	final_rise_delay = get_best_delay(host, rise_delay);
	/* if rising edge has enough margin, then do not scan falling edge */
	if (final_rise_delay.maxlen >= 12 ||
	    (final_rise_delay.start == 0 && final_rise_delay.maxlen >= 4))
		goto skip_fall;

	sdr_set_bits(host->base + MSDC_IOCON, MSDC_IOCON_RSPL);
	for (i = 0; i < PAD_DELAY_MAX; i++) {
		msdc_set_cmd_delay(host, i, 0);
		/*
		 * Using the same parameters, it may sometimes pass the test,
		 * but sometimes it may fail. To make sure the parameters are
		 * more stable, we test each set of parameters 3 times.
		 */
		for (j = 0; j < 3; j++) {
			mmc_send_tuning(mmc, opcode, &cmd_err);
			if (!cmd_err) {
				fall_delay |= (1ULL << i);
			} else {
				fall_delay &= ~(1ULL << i);
				break;
			}
		}
	}
	final_fall_delay = get_best_delay(host, fall_delay);

skip_fall:
	final_maxlen = max(final_rise_delay.maxlen, final_fall_delay.maxlen);
	if (final_fall_delay.maxlen >= 12 && final_fall_delay.start < 4)
		final_maxlen = final_fall_delay.maxlen;
	if (final_maxlen == final_rise_delay.maxlen) {
		sdr_clr_bits(host->base + MSDC_IOCON, MSDC_IOCON_RSPL);
		final_delay = final_rise_delay.final_phase;
	} else {
		sdr_set_bits(host->base + MSDC_IOCON, MSDC_IOCON_RSPL);
		final_delay = final_fall_delay.final_phase;
	}

	msdc_set_cmd_delay(host, final_delay, 0);

	if (host->dev_comp->async_fifo || host->hs200_cmd_int_delay)
		goto skip_internal;

	for (i = 0; i < PAD_DELAY_MAX; i++) {
		sdr_set_field(host->base + tune_reg,
			      MSDC_PAD_TUNE_CMDRRDLY, i);
		mmc_send_tuning(mmc, opcode, &cmd_err);
		if (!cmd_err)
			internal_delay |= (1ULL << i);
	}
	dev_info(host->dev, "Final internal delay: 0x%x\n", internal_delay);
	internal_delay_phase = get_best_delay(host, (u64)internal_delay);
	sdr_set_field(host->base + tune_reg, MSDC_PAD_TUNE_CMDRRDLY,
		      internal_delay_phase.final_phase);
skip_internal:
	dev_info(host->dev, "Final cmd pad delay: %x\n", final_delay);
	return final_delay == 0xff ? -EIO : 0;
}

static int hs400_tune_response(struct mmc_host *mmc, u32 opcode)
{
	struct msdc_host *host = mmc_priv(mmc);
	u64 cmd_delay = 0;
	struct msdc_delay_phase final_cmd_delay = { 0,};
	u8 final_delay;
	int cmd_err;
	int i, j;

	/* select EMMC50 PAD CMD tune */
	sdr_set_bits(host->base + PAD_CMD_TUNE, BIT(0));

	if (mmc->ios.timing == MMC_TIMING_MMC_HS200 ||
	    mmc->ios.timing == MMC_TIMING_UHS_SDR104)
		sdr_set_field(host->base + MSDC_PAD_TUNE,
			      MSDC_PAD_TUNE_CMDRRDLY,
			      host->hs200_cmd_int_delay);

	if (host->hs400_cmd_resp_sel_rising)
		sdr_clr_bits(host->base + MSDC_IOCON, MSDC_IOCON_RSPL);
	else
		sdr_set_bits(host->base + MSDC_IOCON, MSDC_IOCON_RSPL);
	for (i = 0 ; i < PAD_DELAY_MAX; i++) {
		sdr_set_field(host->base + PAD_CMD_TUNE,
			      PAD_CMD_TUNE_RX_DLY3, i);
		/*
		 * Using the same parameters, it may sometimes pass the test,
		 * but sometimes it may fail. To make sure the parameters are
		 * more stable, we test each set of parameters 3 times.
		 */
		for (j = 0; j < 3; j++) {
			mmc_send_tuning(mmc, opcode, &cmd_err);
			if (!cmd_err) {
				cmd_delay |= (1ULL << i);
			} else {
				cmd_delay &= ~(1ULL << i);
				break;
			}
		}
	}
	final_cmd_delay = get_best_delay(host, cmd_delay);
	sdr_set_field(host->base + PAD_CMD_TUNE, PAD_CMD_TUNE_RX_DLY3,
		      final_cmd_delay.final_phase);
	final_delay = final_cmd_delay.final_phase;

	dev_info(host->dev, "Final cmd pad delay: %x\n", final_delay);
	return final_delay == 0xff ? -EIO : 0;
}

static int msdc_tune_data(struct mmc_host *mmc, u32 opcode)
{
	struct msdc_host *host = mmc_priv(mmc);
	u32 rise_delay = 0, fall_delay = 0;
	struct msdc_delay_phase final_rise_delay, final_fall_delay = { 0, };
	u8 final_delay, final_maxlen;
	int i, ret;

	sdr_set_field(host->base + MSDC_PATCH_BIT0,
			MSDC_PB0_INT_DAT_LATCH_CK_SEL,
			host->latch_ck);
	sdr_clr_bits(host->base + MSDC_IOCON, MSDC_IOCON_DSPL);
	sdr_clr_bits(host->base + MSDC_IOCON, MSDC_IOCON_W_DSPL);
	for (i = 0 ; i < PAD_DELAY_MAX; i++) {
		msdc_set_data_delay(host, i, 0);
		ret = mmc_send_tuning(mmc, opcode, NULL);
		if (!ret)
			rise_delay |= (1ULL << i);
	}
	final_rise_delay = get_best_delay(host, rise_delay);
	/* if rising edge has enough margin, then do not scan falling edge */
	if (final_rise_delay.maxlen >= 12 ||
	    (final_rise_delay.start == 0 && final_rise_delay.maxlen >= 4))
		goto skip_fall;

	sdr_set_bits(host->base + MSDC_IOCON, MSDC_IOCON_DSPL);
	sdr_set_bits(host->base + MSDC_IOCON, MSDC_IOCON_W_DSPL);
	for (i = 0; i < PAD_DELAY_MAX; i++) {
		msdc_set_data_delay(host, i, 0);
		ret = mmc_send_tuning(mmc, opcode, NULL);
		if (!ret)
			fall_delay |= (1ULL << i);
	}
	final_fall_delay = get_best_delay(host, fall_delay);

skip_fall:
	final_maxlen = max(final_rise_delay.maxlen, final_fall_delay.maxlen);
	if (final_maxlen == final_rise_delay.maxlen) {
		sdr_clr_bits(host->base + MSDC_IOCON, MSDC_IOCON_DSPL);
		sdr_clr_bits(host->base + MSDC_IOCON, MSDC_IOCON_W_DSPL);
		final_delay = final_rise_delay.final_phase;
	} else {
		sdr_set_bits(host->base + MSDC_IOCON, MSDC_IOCON_DSPL);
		sdr_set_bits(host->base + MSDC_IOCON, MSDC_IOCON_W_DSPL);
		final_delay = final_fall_delay.final_phase;
	}

	msdc_set_data_delay(host, final_delay, 0);

	dev_info(host->dev, "Final data pad delay: %x\n", final_delay);
	return final_delay == 0xff ? -EIO : 0;
}

static int msdc_execute_tuning(struct mmc_host *mmc, u32 opcode)
{
	struct msdc_host *host = mmc_priv(mmc);
	int ret;
	u32 tune_reg = host->dev_comp->pad_tune_reg;

	if (host->dev_comp->tune_resp_data_together) {
		if (host->vcore_reg) {
			/* force lock voltage before tuning */
			ret = regulator_set_voltage(host->vcore_reg, host->vcore_max_volt,
						    host->vcore_max_volt);
			if (ret < 0)
				dev_info(host->dev, "failed to lock vcore: %d\n", ret);
		}

		ret = msdc_tune_resp_data(mmc, opcode);

		if (ret == -EIO)
			dev_info(host->dev, "Tune cmd/data fail!\n");

		if (host->hs400_mode) {
			sdr_clr_bits(host->base + MSDC_IOCON, MSDC_IOCON_DSPL);
			sdr_clr_bits(host->base + MSDC_IOCON,
				     MSDC_IOCON_W_DSPL);
			if (host->top_base != NULL) {
				sdr_set_field(host->top_base + SD_TOP_CONTROL,
					      PAD_DAT_RD_RXDLY, 0);
				sdr_set_field(host->top_base + SD_TOP_CONTROL,
					      PAD_DAT_RD_RXDLY2, 0);
			} else {
				sdr_set_field(host->base + tune_reg,
					      MSDC_PAD_TUNE_DATRRDLY, 0);
				sdr_set_field(host->base + tune_reg + 4,
					      MSDC_PAD_TUNE_DATRRDLY2, 0);
			}
		}
	} else {
		if (host->hs400_mode &&
		    host->dev_comp->hs400_tune)
			ret = hs400_tune_response(mmc, opcode);
		else
			ret = msdc_tune_response(mmc, opcode);
		if (ret == -EIO) {
			dev_info(host->dev, "Tune response fail!\n");
			return ret;
		}
		if (host->hs400_mode == false) {
			ret = msdc_tune_data(mmc, opcode);
			if (ret == -EIO)
				dev_info(host->dev, "Tune data fail!\n");
		}

	}

	host->saved_tune_para.iocon = readl(host->base + MSDC_IOCON);
	host->saved_tune_para.pad_tune = readl(host->base + tune_reg);
	host->saved_tune_para.pad_cmd_tune = readl(host->base + PAD_CMD_TUNE);
	if (host->top_base != NULL) {
		host->saved_tune_para.sd_top_control = readl(host->top_base +
				SD_TOP_CONTROL);
		host->saved_tune_para.sd_top_cmd = readl(host->top_base +
				SD_TOP_CMD);
	}
	return ret;
}

/* TODO ddr208 mode tuning */
static int msdc_prepare_hs400_tuning(struct mmc_host *mmc, struct mmc_ios *ios)
{
	return 0;
}

static void msdc_dump_all_register(struct msdc_host *host)
{
	void __iomem *base = host->base;
	int i;
	unsigned int left_cnt;
	unsigned int byte16_align_cnt;

	byte16_align_cnt = MAX_REGISTER_ADDR / 16;
	for (i = 0; i < byte16_align_cnt; i++)
		pr_info("SDIO reg[%.2x]=0x%.8x reg[%.2x]=0x%.8x reg[%.2x]=0x%.8x reg[%.2x]=0x%.8x\n",
			i * 16, readl(base + i * 16),
			i * 16 + 4, readl(base + i * 16 + 4),
			i * 16 + 8, readl(base + i * 16 + 8),
			i * 16 + 12, readl(base + i * 16 + 12));

	left_cnt = (MAX_REGISTER_ADDR - byte16_align_cnt * 16) / 4 + 1;
	for (i = 0; i < left_cnt; i++)
		pr_info("SDIO reg[%.2x]=0x%.8x\n",
		       byte16_align_cnt * 16 + i * 4,
		       readl(base + byte16_align_cnt * 16 + i * 4));

	if (host->top_base)
		for (i = 0; i < 8; i++)
			pr_info("SDIO top reg[%.2x]=0x%.8x\n", i * 4,
				readl(host->top_base + i * 4));
}

static void msdc_hw_reset(struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);

	sdr_set_bits(host->base + EMMC_IOCON, 1);
	udelay(10); /* 10us is enough */
	sdr_clr_bits(host->base + EMMC_IOCON, 1);
}

static void msdc_ack_sdio_irq(struct mmc_host *mmc)
{
	__msdc_enable_sdio_irq(mmc, 1);
}

static int msdc_select_drive_strength(struct mmc_card *card,
	unsigned int max_dtr, int host_drv,	int card_drv, int *drv_type)
{
	struct mmc_host *mmc = card->host;
	struct msdc_host *host = mmc_priv(mmc);

	of_property_read_u32(host->dev->of_node, "drv-type", drv_type);
	if (card_drv & (1 << (*drv_type)))
		return *drv_type;
	else
		return 0;
}

static const struct mmc_host_ops mt_msdc_ops = {
	.post_req = msdc_post_req,
	.pre_req = msdc_pre_req,
	.request = msdc_ops_request,
	.set_ios = msdc_ops_set_ios,
	.get_ro = mmc_gpio_get_ro,
	.get_cd = mmc_gpio_get_cd,
	.enable_sdio_irq = msdc_enable_sdio_irq,
	.ack_sdio_irq = msdc_ack_sdio_irq,
	.select_drive_strength = msdc_select_drive_strength,
	.start_signal_voltage_switch = msdc_ops_switch_volt,
	.card_busy = msdc_card_busy,
	.execute_tuning = msdc_execute_tuning,
	.prepare_hs400_tuning = msdc_prepare_hs400_tuning,
	.hw_reset = msdc_hw_reset,
};

static irqreturn_t sdio_eint_irq(int irq, void *dev_id)
{
	unsigned long flags;
	struct msdc_host *host = (struct msdc_host *)dev_id;

	spin_lock_irqsave(&host->lock, flags);
	if (likely(host->sdio_irq_cnt > 0)) {
		disable_irq_nosync(host->eint_irq);
		disable_irq_wake(host->eint_irq);
		host->sdio_irq_cnt--;
	}
	spin_unlock_irqrestore(&host->lock, flags);

	sdio_signal_irq(host->mmc);

	return IRQ_HANDLED;
}

static int request_dat1_eint_irq(struct msdc_host *host)
{
	struct gpio_desc *desc;
	int ret = 0;
	int irq;

	desc = devm_gpiod_get_index(host->dev, "eint", 0, GPIOD_IN);
	if (IS_ERR(desc))
		return PTR_ERR(desc);

	irq = gpiod_to_irq(desc);
	if (irq >= 0) {
		irq_set_status_flags(irq, IRQ_NOAUTOEN);
		ret = devm_request_threaded_irq(host->dev, irq,
				NULL, sdio_eint_irq,
				IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				"sdio-eint", host);
	} else
		ret = irq;

	host->eint_irq = irq;
	return ret;
}

static void msdc_of_property_parse(struct platform_device *pdev,
				   struct msdc_host *host)
{
	of_property_read_u32(pdev->dev.of_node, "mediatek,latch-ck",
			     &host->latch_ck);

	of_property_read_u32(pdev->dev.of_node, "hs400-ds-delay",
			     &host->hs400_ds_delay);

	of_property_read_u32(pdev->dev.of_node, "mediatek,hs200-cmd-int-delay",
			     &host->hs200_cmd_int_delay);

	of_property_read_u32(pdev->dev.of_node, "mediatek,hs400-cmd-int-delay",
			     &host->hs400_cmd_int_delay);

	if (of_property_read_bool(pdev->dev.of_node,
				  "mediatek,hs400-cmd-resp-sel-rising"))
		host->hs400_cmd_resp_sel_rising = true;
	else
		host->hs400_cmd_resp_sel_rising = false;

	if (of_property_read_bool(pdev->dev.of_node, "no-sdio-incr1"))
		host->no_sdio_incr1 = true;
	else
		host->no_sdio_incr1 = false;

	of_property_read_u32(pdev->dev.of_node, "max-vcore-volt",
			     &host->vcore_max_volt);
}

static int msdc_drv_probe(struct platform_device *pdev)
{
	struct mmc_host *mmc;
	struct msdc_host *host;
	struct resource *res;
	const struct of_device_id *of_id;
	int ret;

	if (!pdev->dev.of_node) {
		dev_info(&pdev->dev, "No DT found\n");
		return -EINVAL;
	}

	of_id = of_match_node(sdio_of_ids, pdev->dev.of_node);
	if (!of_id)
		return -EINVAL;
	/* Allocate MMC host for this device */
	mmc = mmc_alloc_host(sizeof(struct msdc_host), &pdev->dev);
	if (!mmc)
		return -ENOMEM;

	host = mmc_priv(mmc);
	host->should_dump_register = true;
	ret = mmc_of_parse(mmc);
	if (ret)
		goto host_free;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	host->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(host->base)) {
		ret = PTR_ERR(host->base);
		goto host_free;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	host->top_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(host->top_base))
		host->top_base = NULL;

	ret = mmc_regulator_get_supply(mmc);
	if (ret)
		goto host_free;

	host->src_clk = devm_clk_get(&pdev->dev, "source");
	if (IS_ERR(host->src_clk)) {
		ret = PTR_ERR(host->src_clk);
		goto host_free;
	}

	host->h_clk = devm_clk_get(&pdev->dev, "hclk");
	if (IS_ERR(host->h_clk)) {
		ret = PTR_ERR(host->h_clk);
		goto host_free;
	}

	/*source clock control gate is optional clock*/
	host->src_clk_cg = devm_clk_get(&pdev->dev, "source_cg");
	if (IS_ERR(host->src_clk_cg))
		host->src_clk_cg = NULL;

	/*bus clock control gate is optional clock*/
	host->bus_clk = devm_clk_get(&pdev->dev, "bus_clk");
	if (IS_ERR(host->bus_clk))
		host->bus_clk = NULL;

	/*bus clock control gate is optional clock*/
	host->ext_clk = devm_clk_get(&pdev->dev, "ext_clk");
	if (IS_ERR(host->ext_clk))
		host->ext_clk = NULL;

	host->irq = platform_get_irq(pdev, 0);
	if (host->irq < 0) {
		ret = -EINVAL;
		goto host_free;
	}

	host->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(host->pinctrl)) {
		ret = PTR_ERR(host->pinctrl);
		dev_info(&pdev->dev, "Cannot find pinctrl!\n");
		goto host_free;
	}

	host->pins_default = pinctrl_lookup_state(host->pinctrl, "default");
	if (IS_ERR(host->pins_default)) {
		ret = PTR_ERR(host->pins_default);
		dev_info(&pdev->dev, "Cannot find pinctrl default!\n");
		goto host_free;
	}

	host->pins_uhs = pinctrl_lookup_state(host->pinctrl, "state_uhs");
	if (IS_ERR(host->pins_uhs)) {
		ret = PTR_ERR(host->pins_uhs);
		dev_info(&pdev->dev, "Cannot find pinctrl uhs!\n");
		goto host_free;
	}

	host->pins_eint = pinctrl_lookup_state(host->pinctrl, "state_eint");
	if (IS_ERR(host->pins_eint)) {
		ret = PTR_ERR(host->pins_eint);
		dev_info(&pdev->dev, "Cannot find pinctrl eint!\n");
		goto host_free;
	}

	host->pins_dat1 = pinctrl_lookup_state(host->pinctrl, "state_dat1");
	if (IS_ERR(host->pins_dat1)) {
		ret = PTR_ERR(host->pins_dat1);
		dev_info(&pdev->dev, "Cannot find pinctrl dat1!\n");
		goto host_free;
	}

	host->pins_low = pinctrl_lookup_state(host->pinctrl, "state_low");
	if (IS_ERR(host->pins_low)) {
		ret = PTR_ERR(host->pins_low);
		dev_info(&pdev->dev, "Cannot find pinctrl low!\n");
		host->pins_low = NULL;
	}

	msdc_of_property_parse(pdev, host);

	host->dev = &pdev->dev;
	host->dev_comp = of_id->data;
	host->mmc = mmc;
	host->src_clk_freq = clk_get_rate(host->src_clk);
	/* Set host parameters to mmc */
	mmc->ops = &mt_msdc_ops;
	if (host->dev_comp->clk_div_bits == 8)
		mmc->f_min = DIV_ROUND_UP(host->src_clk_freq, 4 * 255);
	else
		mmc->f_min = DIV_ROUND_UP(host->src_clk_freq, 4 * 4095);

	if (mmc->caps & MMC_CAP_SDIO_IRQ)
		mmc->caps2 |= MMC_CAP2_SDIO_IRQ_NOTHREAD;

	mmc->caps |= MMC_CAP_CMD23;
	/* MMC core transfer sizes tunable parameters */
	mmc->max_segs = MAX_BD_NUM;
	if (host->dev_comp->support_64g)
		mmc->max_seg_size = BDMA_DESC_BUFLEN_EXT;
	else
		mmc->max_seg_size = BDMA_DESC_BUFLEN;
	mmc->max_blk_size = 2048;
	mmc->max_req_size = 512 * 1024;
	mmc->max_blk_count = mmc->max_req_size / 512;
	if (host->dev_comp->support_64g)
		host->dma_mask = DMA_BIT_MASK(36);
	else
		host->dma_mask = DMA_BIT_MASK(32);
	mmc_dev(mmc)->dma_mask = &host->dma_mask;

	host->timeout_clks = 3 * 1048576;
	host->dma.gpd = dma_alloc_coherent(&pdev->dev,
				2 * sizeof(struct mt_gpdma_desc),
				&host->dma.gpd_addr, GFP_KERNEL);
	host->dma.bd = dma_alloc_coherent(&pdev->dev,
				MAX_BD_NUM * sizeof(struct mt_bdma_desc),
				&host->dma.bd_addr, GFP_KERNEL);
	if (!host->dma.gpd || !host->dma.bd) {
		ret = -ENOMEM;
		goto release_mem;
	}
	msdc_init_gpd_bd(host, &host->dma);
	INIT_DELAYED_WORK(&host->req_timeout, msdc_request_timeout);
	spin_lock_init(&host->lock);

	platform_set_drvdata(pdev, mmc);
	msdc_ungate_clock(host);
	msdc_init_hw(host);

	ret = devm_request_irq(&pdev->dev, host->irq, msdc_irq,
		IRQF_ONESHOT, pdev->name, host);
	if (ret)
		goto release;

	ret = request_dat1_eint_irq(host);
	if (ret) {
		dev_info(host->dev, "failed to register data1 eint irq!\n");
		goto release;
	}

	pinctrl_select_state(host->pinctrl, host->pins_dat1);

	host->vcore_reg = devm_regulator_get(&pdev->dev, "dvfsrc-vcore");
	if (IS_ERR(host->vcore_reg)) {
		dev_info(host->dev, "failed to get dvfsrc-vcore regulator!\n");
		host->vcore_reg = NULL;
	}

	cpu_latency_qos_add_request(&host->pm_qos_req, PM_QOS_DEFAULT_VALUE);

	pm_runtime_set_active(host->dev);
	pm_runtime_set_autosuspend_delay(host->dev, MTK_MMC_AUTOSUSPEND_DELAY);
	pm_runtime_use_autosuspend(host->dev);
	pm_runtime_enable(host->dev);
	ret = mmc_add_host(mmc);

	if (ret)
		goto end;

	sdio_proc_init(mmc);

	return 0;
end:
	pm_runtime_disable(host->dev);
	cpu_latency_qos_remove_request(&host->pm_qos_req);
release:
	platform_set_drvdata(pdev, NULL);
	msdc_deinit_hw(host);
	msdc_gate_clock(host);
release_mem:
	if (host->dma.gpd)
		dma_free_coherent(&pdev->dev,
			2 * sizeof(struct mt_gpdma_desc),
			host->dma.gpd, host->dma.gpd_addr);
	if (host->dma.bd)
		dma_free_coherent(&pdev->dev,
			MAX_BD_NUM * sizeof(struct mt_bdma_desc),
			host->dma.bd, host->dma.bd_addr);
host_free:
	mmc_free_host(mmc);

	return ret;
}

static int msdc_drv_remove(struct platform_device *pdev)
{
	struct mmc_host *mmc;
	struct msdc_host *host;

	mmc = platform_get_drvdata(pdev);
	host = mmc_priv(mmc);

	pm_runtime_get_sync(host->dev);

	platform_set_drvdata(pdev, NULL);
	mmc_remove_host(host->mmc);
	msdc_deinit_hw(host);
	msdc_gate_clock(host);

	cpu_latency_qos_remove_request(&host->pm_qos_req);
	pm_runtime_disable(host->dev);
	pm_runtime_put_noidle(host->dev);
	dma_free_coherent(&pdev->dev,
			2 * sizeof(struct mt_gpdma_desc),
			host->dma.gpd, host->dma.gpd_addr);
	dma_free_coherent(&pdev->dev, MAX_BD_NUM * sizeof(struct mt_bdma_desc),
			host->dma.bd, host->dma.bd_addr);

	mmc_free_host(host->mmc);

	return 0;
}

#ifdef CONFIG_PM
static void msdc_save_reg(struct msdc_host *host)
{
	u32 tune_reg = host->dev_comp->pad_tune_reg;

	host->save_para.msdc_cfg = readl(host->base + MSDC_CFG);
	host->save_para.iocon = readl(host->base + MSDC_IOCON);
	host->save_para.sdc_cfg = readl(host->base + SDC_CFG);
	host->save_para.pad_tune = readl(host->base + tune_reg);
	host->save_para.patch_bit0 = readl(host->base + MSDC_PATCH_BIT0);
	host->save_para.patch_bit1 = readl(host->base + MSDC_PATCH_BIT1);
	host->save_para.patch_bit2 = readl(host->base + MSDC_PATCH_BIT2);
	host->save_para.pad_ds_tune = readl(host->base + PAD_DS_TUNE);
	host->save_para.pad_cmd_tune = readl(host->base + PAD_CMD_TUNE);
	host->save_para.emmc50_cfg0 = readl(host->base + EMMC50_CFG0);
	host->save_para.emmc50_cfg3 = readl(host->base + EMMC50_CFG3);
	host->save_para.sdc_fifo_cfg = readl(host->base + SDC_FIFO_CFG);
	host->save_para.msdc_inten = readl(host->base + MSDC_INTEN);
	if (host->top_base != NULL) {
		host->save_para.sd_top_control =
			readl(host->top_base + SD_TOP_CONTROL);
		host->save_para.sd_top_cmd =
			readl(host->top_base + SD_TOP_CMD);

	}
}

static void msdc_restore_reg(struct msdc_host *host)
{
	u32 tune_reg = host->dev_comp->pad_tune_reg;

	writel(host->save_para.msdc_cfg, host->base + MSDC_CFG);
	writel(host->save_para.iocon, host->base + MSDC_IOCON);
	writel(host->save_para.sdc_cfg, host->base + SDC_CFG);
	writel(host->save_para.pad_tune, host->base + tune_reg);
	writel(host->save_para.patch_bit0, host->base + MSDC_PATCH_BIT0);
	writel(host->save_para.patch_bit1, host->base + MSDC_PATCH_BIT1);
	writel(host->save_para.patch_bit2, host->base + MSDC_PATCH_BIT2);
	writel(host->save_para.pad_ds_tune, host->base + PAD_DS_TUNE);
	writel(host->save_para.pad_cmd_tune, host->base + PAD_CMD_TUNE);
	writel(host->save_para.emmc50_cfg0, host->base + EMMC50_CFG0);
	writel(host->save_para.emmc50_cfg3, host->base + EMMC50_CFG3);
	writel(host->save_para.sdc_fifo_cfg, host->base + SDC_FIFO_CFG);
	writel(host->save_para.msdc_inten, host->base + MSDC_INTEN);
	if (host->top_base != NULL) {
		writel(host->save_para.sd_top_control,
				host->top_base + SD_TOP_CONTROL);
		writel(host->save_para.sd_top_cmd,
				host->top_base + SD_TOP_CMD);
	}
}

static int msdc_runtime_suspend(struct device *dev)
{
	unsigned long flags;
	struct mmc_host *mmc = dev_get_drvdata(dev);
	struct msdc_host *host = mmc_priv(mmc);
	int ret;

	msdc_save_reg(host);
	disable_irq(host->irq);

	/* Card is removed, no need eint irq, pull low all IO pins */
	if (!mmc->card && host->pins_low) {
		msdc_gate_clock(host);
		pinctrl_select_state(host->pinctrl, host->pins_low);
		sdr_clr_bits(host->base + SDC_CFG, SDC_CFG_SDIOIDE);
	} else {
		pinctrl_select_state(host->pinctrl, host->pins_eint);
		spin_lock_irqsave(&host->lock, flags);
		if (host->sdio_irq_cnt == 0) {
			enable_irq(host->eint_irq);
			enable_irq_wake(host->eint_irq);
			host->sdio_irq_cnt++;
		}
		sdr_clr_bits(host->base + SDC_CFG, SDC_CFG_SDIOIDE);
		spin_unlock_irqrestore(&host->lock, flags);
		msdc_gate_clock(host);
	}

	if (host->vcore_reg) {
		ret = regulator_set_voltage(host->vcore_reg, 0, INT_MAX);
		if (ret < 0)
			dev_info(dev, "msdc runtime suspend set voltage returns: %d\n", ret);
	}

	cpu_latency_qos_update_request(&host->pm_qos_req, PM_QOS_DEFAULT_VALUE);

	return 0;
}

static int msdc_runtime_resume(struct device *dev)
{
	unsigned long flags;
	struct mmc_host *mmc = dev_get_drvdata(dev);
	struct msdc_host *host = mmc_priv(mmc);
	int ret;

	cpu_latency_qos_update_request(&host->pm_qos_req, 0);

	/* Restore all pin state for prepare of card recognition */
	if (!mmc->card && host->pins_low) {
		pinctrl_select_state(host->pinctrl, host->pins_default);
		sdr_set_bits(host->base + SDC_CFG, SDC_CFG_SDIOIDE);
	}

	msdc_ungate_clock(host);
	msdc_restore_reg(host);

	if (mmc->card) {
		spin_lock_irqsave(&host->lock, flags);
		if (host->sdio_irq_cnt > 0) {
			disable_irq_nosync(host->eint_irq);
			disable_irq_wake(host->eint_irq);
			host->sdio_irq_cnt--;
			sdr_set_bits(host->base + SDC_CFG, SDC_CFG_SDIOIDE);
		} else
			sdr_clr_bits(host->base + MSDC_INTEN, MSDC_INTEN_SDIOIRQ);
		spin_unlock_irqrestore(&host->lock, flags);
		pinctrl_select_state(host->pinctrl, host->pins_uhs);
	}
	enable_irq(host->irq);

	if (host->vcore_reg) {
		ret = regulator_set_voltage(host->vcore_reg, host->vcore_max_volt,
					    host->vcore_max_volt);
		if (ret < 0)
			dev_info(dev, "msdc runtime resume set voltage returns: %d\n", ret);
	}

	return 0;
}
#endif

static const struct dev_pm_ops msdc_dev_pm_ops = {
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(msdc_runtime_suspend, msdc_runtime_resume, NULL)
};

static struct platform_driver mt_sdio_driver = {
	.probe = msdc_drv_probe,
	.remove = msdc_drv_remove,
	.driver = {
		.name = "mtk-sdio",
		.of_match_table = sdio_of_ids,
		.pm = &msdc_dev_pm_ops,
	},
};

module_platform_driver(mt_sdio_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek SDIO Card Driver");
