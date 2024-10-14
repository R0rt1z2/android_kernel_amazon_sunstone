// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/thermal.h>
#include <linux/uaccess.h>

#include "apu_log.h"
#include "apu_top.h"
#include "aputop_rpmsg.h"
#include "mt8188_apupwr.h"
#include "mt8188_apupwr_prot.h"

#define LOCAL_DBG	(1)

static void __iomem *spare_reg_base;

/* for saving data after sync with remote site */
static struct apu_pwr_curr_info curr_info;
static const char * const pll_name[] = {
				"PLL_CONN", "PLL_RV33", "PLL_VPU", "PLL_MDLA"};
static const char * const buck_name[] = {
				"BUCK_VAPU", "BUCK_VSRAM", "BUCK_VCORE"};
static const char * const cluster_name[] = {
				"ACX0", "RCX"};

#define _OPP_LMT_TBL(_opp_lmt_reg) {    \
	.opp_lmt_reg = _opp_lmt_reg,    \
}
static struct cluster_dev_opp_info opp_limit_tbl[CLUSTER_NUM] = {
	_OPP_LMT_TBL(ACX0_LIMIT_OPP_REG),
};

static inline int over_range_check(int opp)
{
	/* we treat opp -1 as a special hint regard to unlimit opp ! */
	if (opp == -1)
		return -1;
	else if (opp < USER_MAX_OPP_VAL)
		return USER_MAX_OPP_VAL;
	else if (opp > opp_tbl->tbl_size - 1)
		return opp_tbl->tbl_size - 1;
	else
		return opp;
}

static void _opp_limiter(int vpu_max, int vpu_min, int dla_max, int dla_min,
		enum apu_opp_limit_type type)
{
	int i;
	unsigned int reg_data;
	unsigned int reg_offset;

	vpu_max = over_range_check(vpu_max);
	vpu_min = over_range_check(vpu_min);
	dla_max = over_range_check(dla_max);
	dla_min = over_range_check(dla_min);

#if LOCAL_DBG
	pr_info("%s type:%d, %d/%d/%d/%d\n", __func__, type,
			vpu_max, vpu_min, dla_max, dla_min);
#endif
	for (i = 0 ; i < CLUSTER_NUM ; i++) {
		opp_limit_tbl[i].dev_opp_lmt.vpu_max = vpu_max & 0x3f;
		opp_limit_tbl[i].dev_opp_lmt.vpu_min = vpu_min & 0x3f;
		opp_limit_tbl[i].dev_opp_lmt.dla_max = dla_max & 0x3f;
		opp_limit_tbl[i].dev_opp_lmt.dla_min = dla_min & 0x3f;
		opp_limit_tbl[i].dev_opp_lmt.lmt_type = type & 0xff;

		reg_data = 0x0;
		reg_data = (
			((vpu_max & 0x3f) << 0) |	/* [5:0] */
			((vpu_min & 0x3f) << 6) |	/* [11:6] */
			((dla_max & 0x3f) << 12) |	/* [17:12] */
			((dla_min & 0x3f) << 18) |	/* [23:18] */
			((type & 0xff) << 24));		/* dedicate 1 byte */

		reg_offset = opp_limit_tbl[i].opp_lmt_reg;

		apu_writel(reg_data, spare_reg_base + reg_offset);
#if LOCAL_DBG
		pr_info("%s cluster%d write:0x%08x, readback:0x%08x\n",
				__func__, i, reg_data,
				apu_readl(spare_reg_base + reg_offset));
#endif
	}

}

void mt8188_aputop_opp_limit(struct aputop_func_param *aputop,
		enum apu_opp_limit_type type)
{
	int vpu_max, vpu_min, dla_max, dla_min;

	vpu_max = aputop->param1;
	vpu_min = aputop->param2;
	dla_max = aputop->param3;
	dla_min = aputop->param4;

	_opp_limiter(vpu_max, vpu_min, dla_max, dla_min, type);
}

#if IS_ENABLED(CONFIG_DEBUG_FS)
static void limit_opp_to_all_devices(int opp)
{
	int c_id, d_id;

	for (c_id = 0 ; c_id < CLUSTER_NUM ; c_id++)
		for (d_id = 0 ; d_id < DEVICE_NUM ; d_id++)
			_opp_limiter(opp, opp, opp, opp, OPP_LIMIT_DEBUG);
}

static int aputop_dbg_set_parameter(int param, int argc, int *args)
{
	int ret = 0, i;
	struct aputop_rpmsg_data rpmsg_data;

	for (i = 0 ; i < argc ; i++) {
		if (args[i] < -1 || args[i] >= INT_MAX) {
			pr_info("%s invalid args[%d]\n", __func__, i);
			return -EINVAL;
		}
	}

	memset(&rpmsg_data, 0, sizeof(struct aputop_rpmsg_data));

	switch (param) {
	case APUPWR_DBG_DEV_CTL:
		if (argc == 3) {
			rpmsg_data.cmd = APUTOP_DEV_CTL;
			rpmsg_data.data0 = args[0]; /* cluster_id */
			rpmsg_data.data1 = args[1]; /* device_id */
			rpmsg_data.data2 = args[2]; /* POWER_ON/POWER_OFF */
			aputop_send_tx_rpmsg(&rpmsg_data, 100);
		} else {
			pr_info("%s invalid param num:%d\n", __func__, argc);
			ret = -EINVAL;
		}
		break;
	case APUPWR_DBG_DEV_SET_OPP:
		if (argc == 3) {
			rpmsg_data.cmd = APUTOP_DEV_SET_OPP;
			rpmsg_data.data0 = args[0]; /* cluster_id */
			rpmsg_data.data1 = args[1]; /* device_id */
			rpmsg_data.data2 = args[2]; /* opp */
			aputop_send_tx_rpmsg(&rpmsg_data, 100);
		} else {
			pr_info("%s invalid param num:%d\n", __func__, argc);
			ret = -EINVAL;
		}
		break;
	case APUPWR_DBG_DVFS_DEBUG:
		if (argc == 1) {
			limit_opp_to_all_devices(args[0]);
		} else {
			pr_info("%s invalid param num:%d\n", __func__, argc);
			ret = -EINVAL;
		}
		break;
	case APUPWR_DBG_DUMP_OPP_TBL:
		if (argc == 1) {
			rpmsg_data.cmd = APUTOP_DUMP_OPP_TBL;
			rpmsg_data.data0 = args[0]; /* pseudo data */
			aputop_send_tx_rpmsg(&rpmsg_data, 100);
		} else {
			pr_info("%s invalid param num:%d\n", __func__, argc);
			ret = -EINVAL;
		}
		break;
	case APUPWR_DBG_CURR_STATUS:
		if (argc == 1) {
			rpmsg_data.cmd = APUTOP_CURR_STATUS;
			rpmsg_data.data0 = args[0]; /* pseudo data */
			aputop_send_tx_rpmsg(&rpmsg_data, 100);
		} else {
			pr_info("%s invalid param num:%d\n", __func__, argc);
			ret = -EINVAL;
		}
		break;
	case APUPWR_DBG_PROFILING:
		if (argc == 2) {
			rpmsg_data.cmd = APUTOP_PWR_PROFILING;
			/* 0:clean, 1:result, 2:allow bit, 3:allow bitmask */
			rpmsg_data.data0 = args[0];
			/* value of allow bit/bitmask */
			rpmsg_data.data1 = args[1]; /* allow bitmask */
			aputop_send_tx_rpmsg(&rpmsg_data, 100);
		} else {
			pr_info("%s invalid param num:%d\n", __func__, argc);
			ret = -EINVAL;
		}
		break;
	case APUPWR_DBG_DPIDLE_SKIP:
		if (argc == 1) {
			rpmsg_data.cmd = APUTOP_DPIDLE_SKIP;
			rpmsg_data.data0 = args[0]; /* 0: disable 1: enable */
			aputop_send_tx_rpmsg(&rpmsg_data, 100);
			pr_info("%s set apu dpidle_ctl: %d\n",
				__func__, args[0]);
		} else {
			pr_info("%s invalid param num:%d\n", __func__, argc);
			ret = -EINVAL;
		}
		break;
	default:
		pr_info("%s unsupport the pwr param:%d\n", __func__, param);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static void plat_dump_boost_mapping(struct seq_file *s,
				    enum dvfs_device_id dvfs_dev_id)
{
	int i, begin, end;
	enum apu_clksrc_id apu_clksrc_id;
	int delta = 0;
	int *step = NULL;
	int opp_min_idx = 0;

	if ((opp_tbl == NULL) || (opp_tbl->tbl_size == 0))
		return;

	switch (dvfs_dev_id) {
	case DVFS_DEV_MVPU:
		apu_clksrc_id = PLL_VPU;
		break;
	case DVFS_DEV_MDLA:
		apu_clksrc_id = PLL_DLA;
		break;
	default:
		return;
	}

	step = kzalloc(sizeof(int)*opp_tbl->tbl_size, GFP_KERNEL);
	if (!step)
		return;

	opp_min_idx = opp_tbl->tbl_size - 1;
	delta = opp_tbl->opp[0].pll_freq[apu_clksrc_id] -
			opp_tbl->opp[opp_min_idx].pll_freq[apu_clksrc_id];
	if (delta <= 0)
		delta = 1;

	for (i = 0; i <= opp_min_idx; i++) {
		step[i] = opp_tbl->opp[i].pll_freq[apu_clksrc_id] -
			opp_tbl->opp[opp_min_idx].pll_freq[apu_clksrc_id];
		if (step[i] > delta)
			step[i] = delta;
	}

	for (i = 0; i <= opp_min_idx; i++)
		step[i] = (step[i] * 100 / delta);

	seq_printf(s, "devfs dev %d\n", dvfs_dev_id);
	for (i = 0, begin = TURBO_BOOST_VAL; i < opp_min_idx + 1; i++) {
		end = step[i];
		if (begin <= end)
			begin = end;
		seq_printf(s, "opp%d : boost %d ~ %d (%d)\n",
					i, begin, end, step[i]);
		begin = step[i] - 1;
	}

	kfree(step);
}

static int aputop_show_opp_tbl(struct seq_file *s, void *unused)
{
	int size, i, j;

	pr_info("%s ++\n", __func__);

	if (opp_tbl == NULL)
		return -EINVAL;

	size = opp_tbl->tbl_size;

	/* first line */
	seq_printf(s, "\n| # | %s |", buck_name[0]);
	for (i = 0 ; i < PLL_NUM ; i++)
		seq_printf(s, " %s |", pll_name[i]);

	seq_puts(s, "\n");
	for (i = 0 ; i < size ; i++) {
		seq_printf(s, "| %d |   %d  |", i, opp_tbl->opp[i].vapu);

		for (j = 0 ; j < PLL_NUM ; j++)
			seq_printf(s, "  %07d |", opp_tbl->opp[i].pll_freq[j]);

		seq_puts(s, "\n");
	}

	seq_puts(s, "\n");
	for (i = DVFS_DEV_MVPU; i < DVFS_DEV_MAX; i++)
		plat_dump_boost_mapping(s, i);
	seq_puts(s, "\n");

	return 0;
}

static int aputop_show_curr_status(struct seq_file *s, void *unused)
{
	struct apu_pwr_curr_info info;
	struct rpc_status_dump cluster_dump[CLUSTER_NUM + 1];
	struct platform_device *pdev = (struct platform_device *)s->private;
	int i;

	pr_info("%s ++\n", __func__);
	memcpy(&info, &curr_info, sizeof(struct apu_pwr_curr_info));

	seq_puts(s, "\n");

	for (i = 0 ; i < PLL_NUM ; i++) {
		seq_printf(s, "%s : opp %d , %d(kHz)\n",
				pll_name[i],
				info.pll_opp[i],
				info.pll_freq[i]);
	}

	for (i = 0 ; i < BUCK_NUM ; i++) {
		seq_printf(s, "%s : opp %d , %d(mV)\n",
				buck_name[i],
				info.buck_opp[i],
				info.buck_volt[i]);
	}

	if (pm_runtime_get_if_in_use(&pdev->dev) > 0) {
		for (i = 0 ; i < CLUSTER_NUM ; i++) {
			mt8188_apu_dump_rpc_status(i, &cluster_dump[i]);
			seq_printf(s,
				"%s : rpc_status 0x%08x , conn_cg 0x%08x\n",
				cluster_name[i],
				cluster_dump[i].rpc_reg_status,
				cluster_dump[i].conn_reg_status);
		}

		pm_runtime_put(&pdev->dev);
	} else {
		for (i = 0 ; i < CLUSTER_NUM ; i++) {
			seq_printf(s, "%s : rpc_status OFF , conn_cg GATED\n",
					cluster_name[i]);
		}
	}

	/* for RCX */
	mt8188_apu_dump_rpc_status(RCX, &cluster_dump[CLUSTER_NUM]);
	seq_printf(s,
		"%s : rpc_status 0x%08x , conn_cg 0x%08x vcore_cg 0x%08x\n",
			cluster_name[CLUSTER_NUM],
			cluster_dump[CLUSTER_NUM].rpc_reg_status,
			cluster_dump[CLUSTER_NUM].conn_reg_status,
			cluster_dump[CLUSTER_NUM].vcore_reg_status);

	seq_puts(s, "\n");

	return 0;
}

static int apu_top_dbg_show(struct seq_file *s, void *unused)
{
	int ret = 0;
	int args[1] = {0};
	enum aputop_rpmsg_cmd cmd = get_curr_tx_rpmsg_cmd();

	pr_info("%s for aputop_rpmsg_cmd : %d\n", __func__, cmd);

	if (cmd == APUTOP_DUMP_OPP_TBL) {
		aputop_dbg_set_parameter(APUPWR_DBG_DUMP_OPP_TBL, 1, args);
		ret = aputop_show_opp_tbl(s, unused);
	} else if (cmd == APUTOP_CURR_STATUS) {
		aputop_dbg_set_parameter(APUPWR_DBG_CURR_STATUS, 1, args);
		ret = aputop_show_curr_status(s, unused);
	} else {
		pr_info("%s not support this cmd (%d)\n",
			__func__, cmd);
	}

	return ret;
}

int mt8188_apu_top_dbg_open(struct inode *inode, struct file *file)
{
	pr_info("%s ++\n", __func__);

	return single_open(file, apu_top_dbg_show, inode->i_private);
}

#define MAX_ARG 4
ssize_t mt8188_apu_top_dbg_write(
		struct file *flip, const char __user *buffer,
		size_t count, loff_t *f_pos)
{
	char *tmp, *token, *cursor;
	int ret, i, param;
	unsigned int args[MAX_ARG];

	pr_info("%s\n", __func__);

	tmp = kzalloc(count + 1, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	ret = copy_from_user(tmp, buffer, count);
	if (ret) {
		pr_info("[%s] copy_from_user failed, ret=%d\n", __func__, ret);
		goto out;
	}

	tmp[count] = '\0';
	cursor = tmp;
	/* parse a command */
	token = strsep(&cursor, " ");
	if (!strcmp(token, "device_ctl"))
		param = APUPWR_DBG_DEV_CTL;
	else if (!strcmp(token, "device_set_opp"))
		param = APUPWR_DBG_DEV_SET_OPP;
	else if (!strcmp(token, "dvfs_debug"))
		param = APUPWR_DBG_DVFS_DEBUG;
	else if (!strcmp(token, "dump_opp_tbl"))
		param = APUPWR_DBG_DUMP_OPP_TBL;
	else if (!strcmp(token, "curr_status"))
		param = APUPWR_DBG_CURR_STATUS;
	else if (!strcmp(token, "pwr_profiling"))
		param = APUPWR_DBG_PROFILING;
	else if (!strcmp(token, "dpidle_skip"))
		param = APUPWR_DBG_DPIDLE_SKIP;
	else {
		ret = -EINVAL;
		pr_info("no power param[%s]!\n", token);
		goto out;
	}

	/* parse arguments */
	for (i = 0; i < MAX_ARG && (token = strsep(&cursor, " ")); i++) {
		ret = kstrtoint(token, 10, &args[i]);
		if (ret) {
			pr_info("fail to parse args[%d](%s)", i, token);
			goto out;
		}
	}

	aputop_dbg_set_parameter(param, i, args);
	ret = count;
out:
	kfree(tmp);
	return ret;
}
#endif

int mt8188_apu_top_tx_rpmsg_cb(int cmd, void *data, int len, void *priv,
				u32 src)
{
	int ret = 0;
	int tbl_size = 0;
	int total_tbl_size = 0;
	struct tiny_dvfs_opp_tbl *tmp_opp_tbl;

	switch ((enum aputop_rpmsg_cmd)cmd) {
	case APUTOP_DEV_CTL:
	case APUTOP_DEV_SET_OPP:
	case APUTOP_PWR_PROFILING:
	case APUTOP_DPIDLE_SKIP:
		/* do nothing */
		break;
	case APUTOP_DUMP_OPP_TBL:
		tbl_size = ((struct tiny_dvfs_opp_tbl *)data)->tbl_size;
		total_tbl_size = sizeof(*opp_tbl) +
				tbl_size * sizeof(struct tiny_dvfs_opp_entry);
		if (len != total_tbl_size) {
			pr_info(
				"[%s][%d] invalid size: len = %d, total_tbl_size = %d\n",
				__func__, __LINE__, len, total_tbl_size);
			ret = -EINVAL;
			break;
		}

		if (opp_tbl == NULL)
			opp_tbl = kzalloc(sizeof(*opp_tbl) + total_tbl_size,
					  GFP_KERNEL);
		else if (opp_tbl->tbl_size < tbl_size) {
			pr_info("[%s][%d] opp_tbl->tbl_size = %d, tbl_size = %d\n",
				__func__, __LINE__, opp_tbl->tbl_size, tbl_size);
			tmp_opp_tbl = kzalloc(total_tbl_size, GFP_KERNEL);
			memcpy(tmp_opp_tbl, opp_tbl, sizeof(*opp_tbl));
			kfree(opp_tbl);
			opp_tbl = tmp_opp_tbl;
		}

		if (IS_ERR_OR_NULL(opp_tbl))
			return -EINVAL;

		memcpy(opp_tbl, data, total_tbl_size);

		break;
	case APUTOP_CURR_STATUS:
		if (len == sizeof(curr_info)) {
			memcpy(&curr_info,
				(struct apu_pwr_curr_info *)data, len);
		} else {
			pr_info("%s invalid size : %d/%ld\n",
					__func__, len, sizeof(curr_info));
			ret = -EINVAL;
		}
		break;
	default:
		pr_info("%s invalid cmd : %d\n", __func__, cmd);
		ret = -EINVAL;
		break;
	}

	return ret;
}

int mt8188_apu_top_rx_rpmsg_cb(int cmd, void *data, int len, void *priv,
				u32 src)
{
	int ret = 0;
	struct aputop_rpmsg_data *reply_data;

	reply_data = kzalloc(sizeof(struct aputop_rpmsg_data), GFP_KERNEL);
	if (!reply_data)
		return -ENOMEM;

	reply_data->cmd = cmd;

	/* handle the cmd received from uP */
	switch ((enum aputop_rpmsg_cmd)cmd) {
	case APUTOP_THERMAL_GET_TEMP:
		/* TODO: get temperature by thermal api */
		reply_data->data0 = -1;

		ret = aputop_send_rx_rpmsg(reply_data);
		break;
	default:
		pr_info("%s invalid cmd : %d\n", __func__, cmd);
		ret = -EINVAL;
		break;
	}

	kfree(reply_data);
	return ret;
}

int mt8188_pwr_flow_remote_sync(uint32_t cfg)
{
	uint32_t reg_data = 0x0;

	reg_data = (cfg & 0x1);

	apu_writel(reg_data, spare_reg_base + PWR_FLOW_SYNC_REG);

	LOG_DBG("%s write 0x%08x, readback:0x%08x\n",
			__func__, reg_data,
			apu_readl(spare_reg_base + PWR_FLOW_SYNC_REG));

	return 0;
}

int mt8188_drv_cfg_remote_sync(struct aputop_func_param *aputop)
{
	struct drv_cfg_data cfg;
	uint32_t reg_data = 0x0;

	cfg.log_level = aputop->param1 & 0xf;
	cfg.dvfs_debounce = aputop->param2 & 0xf;
	cfg.disable_hw_meter = aputop->param3 & 0xf;

	reg_data = cfg.log_level |
		(cfg.dvfs_debounce << 8) |
		(cfg.disable_hw_meter << 16);

	LOG_DBG("%s 0x%08x\n", __func__, reg_data);
	apu_writel(reg_data, spare_reg_base + DRV_CFG_SYNC_REG);

	return 0;
}

int mt8188_chip_data_remote_sync(struct plat_cfg_data *plat_cfg)
{
	uint32_t reg_data = 0x0;

	LOG_DBG("%s %d ++\n", __func__, __LINE__);

	reg_data = (plat_cfg->aging_flag & 0xf)
		| ((plat_cfg->hw_id & 0xf) << 4);

	LOG_DBG("%s 0x%08x\n", __func__, reg_data);
	apu_writel(reg_data, spare_reg_base + PLAT_CFG_SYNC_REG);

	reg_data = (plat_cfg->freq[0] & 0xFFFF)
			| ((plat_cfg->freq[1] & 0xFFFF) << 16);
	apu_writel(reg_data, spare_reg_base + OPP_SYNC_REG0);

	reg_data = (plat_cfg->freq[2] & 0xFFFF)
			| ((plat_cfg->freq[3] & 0xFFFF) << 16);
	apu_writel(reg_data, spare_reg_base + OPP_SYNC_REG1);

	reg_data = (plat_cfg->fix_volt & 0xF);
	apu_writel(reg_data, spare_reg_base + OPP_SYNC_REG2);

	LOG_DBG("%s %d --\n", __func__, __LINE__);

	return 0;
}

int mt8188_init_remote_data_sync(void __iomem *reg_base)
{
	int i;
	uint32_t reg_offset = 0x0;

	LOG_DBG("%s %d ++\n", __func__, __LINE__);

	spare_reg_base = reg_base;

	for (i = 0 ; i < CLUSTER_NUM ; i++) {
		/* 0xffff_ffff means no limit */
		memset(&opp_limit_tbl[i].dev_opp_lmt, -1,
				sizeof(struct device_opp_limit));
		reg_offset = opp_limit_tbl[i].opp_lmt_reg;
#if LOCAL_DBG
		LOG_DBG("%s spare_reg_base:0x%lx, offset:0x%08x\n",
				__func__, (uintptr_t)spare_reg_base,
				reg_offset);
#endif
		apu_writel(0xffffffff, spare_reg_base + reg_offset);
	}

	apu_writel(0x0, spare_reg_base + DRV_CFG_SYNC_REG);
	apu_writel(0x0, spare_reg_base + PLAT_CFG_SYNC_REG);

	apu_writel(0x0, spare_reg_base + OPP_SYNC_REG0);
	apu_writel(0x0, spare_reg_base + OPP_SYNC_REG1);
	apu_writel(0x0, spare_reg_base + OPP_SYNC_REG2);

	LOG_DBG("%s %d --\n", __func__, __LINE__);

	return 0;
}

void mt8188_apu_temperature_sync(const char *tz_name)
{
	struct thermal_zone_device *apu_tz;
	int temp = 0;
	int ret = 0;

	apu_tz = thermal_zone_get_zone_by_name(tz_name);
	if (IS_ERR(apu_tz))
		temp = THERMAL_TEMP_INVALID;

	ret = thermal_zone_get_temp(apu_tz, &temp);
	if (ret)
		temp = THERMAL_TEMP_INVALID;
#if LOCAL_DBG
	LOG_DBG("%s apu current temperature: %d\n", __func__, temp);
#endif
	apu_writel(temp, spare_reg_base + THERMAL_SYNC_TEMP_REG);
}
