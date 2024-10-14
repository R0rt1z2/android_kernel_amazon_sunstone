// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/spinlock.h>

#include <lpm_dbg_common_v1.h>
#include <lpm_module.h>
#include <mtk_idle_sysfs.h>
#include <mtk_suspend_sysfs.h>
#include <mtk_spm_sysfs.h>

#include <mt8188_pwr_ctrl.h>
#include <lpm_dbg_fs_common.h>
#include <lpm_spm_comm.h>

/* Determine for node route */
#define MT_LP_RQ_NODE	"/proc/mtk_lpm/spm/spm_resource_req"

#define DEFINE_ATTR_RO(_name)			\
	static struct kobj_attribute _name##_attr = {	\
		.attr	= {				\
			.name = #_name,			\
			.mode = 0444,			\
		},					\
		.show	= _name##_show,			\
	}
#define DEFINE_ATTR_RW(_name)			\
	static struct kobj_attribute _name##_attr = {	\
		.attr	= {				\
			.name = #_name,			\
			.mode = 0644,			\
		},					\
		.show	= _name##_show,			\
		.store	= _name##_store,		\
	}
#define __ATTR_OF(_name)	(&_name##_attr.attr)

#undef mtk_dbg_spm_log
#define mtk_dbg_spm_log(fmt, args...) \
	do { \
		int l = scnprintf(p, sz, fmt, ##args); \
		p += l; \
		sz -= l; \
	} while (0)


static char *mt8188_pwr_ctrl_str[PW_MAX_COUNT] = {
	[PW_PCM_FLAGS] = "pcm_flags",
	[PW_PCM_FLAGS_CUST] = "pcm_flags_cust",
	[PW_PCM_FLAGS_CUST_SET] = "pcm_flags_cust_set",
	[PW_PCM_FLAGS_CUST_CLR] = "pcm_flags_cust_clr",
	[PW_PCM_FLAGS1] = "pcm_flags1",
	[PW_PCM_FLAGS1_CUST] = "pcm_flags1_cust",
	[PW_PCM_FLAGS1_CUST_SET] = "pcm_flags1_cust_set",
	[PW_PCM_FLAGS1_CUST_CLR] = "pcm_flags1_cust_clr",
	[PW_TIMER_VAL] = "timer_val",
	[PW_TIMER_VAL_CUST] = "timer_val_cust",
	[PW_TIMER_VAL_RAMP_EN] = "timer_val_ramp_en",
	[PW_TIMER_VAL_RAMP_EN_SEC] = "timer_val_ramp_en_sec",
	[PW_WAKE_SRC] = "wake_src",
	[PW_WAKE_SRC_CUST] = "wake_src_cust",
	[PW_WDT_DISABLE] = "wdt_disable",

	/* SPM_AP_STANDBY_CON */
	[PW_REG_WFI_OP] = "reg_wfi_op",
	[PW_REG_WFI_TYPE] = "reg_wfi_type",
	[PW_REG_MP0_CPUTOP_IDLE_MASK] = "reg_mp0_cputop_idle_mask",
	[PW_REG_MP1_CPUTOP_IDLE_MASK] = "reg_mp1_cputop_idle_mask",
	[PW_REG_MCUSYS_IDLE_MASK] = "reg_mcusys_idle_mask",
	[PW_REG_MD_APSRC_1_SEL] = "reg_md_apsrc_1_sel",
	[PW_REG_MD_APSRC_0_SEL] = "reg_md_apsrc_0_sel",
	[PW_REG_CONN_APSRC_SEL] = "reg_conn_apsrc_sel",
	/* SPM_SRC_REQ */
	[PW_REG_SPM_APSRC_REQ] = "reg_spm_apsrc_req",
	[PW_REG_SPM_F26M_REQ] = "reg_spm_f26m_req",
	[PW_REG_SPM_INFRA_REQ] = "reg_spm_infra_req",
	[PW_REG_SPM_VRF18_REQ] = "reg_spm_vrf18_req",
	[PW_REG_SPM_DDR_EN_REQ] = "reg_spm_ddr_en_req",
	[PW_REG_SPM_DVFS_REQ] = "reg_spm_dvfs_req",
	[PW_REG_SPM_SW_MAILBOX_REQ] = "reg_spm_sw_mailbox_req",
	[PW_REG_SPM_SSPM_MAILBOX_REQ] = "reg_spm_sspm_mailbox_req",
	[PW_REG_SPM_ADSP_MAILBOX_REQ] = "reg_spm_adsp_mailbox_req",
	[PW_REG_SPM_SCP_MAILBOX_REQ] = "reg_spm_scp_mailbox_req",
	/* SPM_SRC_MASK */
	[PW_REG_SSPM_SRCCLKENA_0_MASK_B] = "reg_sspm_srcclkena_0_mask_b ",
	[PW_REG_SSPM_INFRA_REQ_0_MASK_B] = "reg_sspm_infra_req_0_mask_b ",
	[PW_REG_SSPM_APSRC_REQ_0_MASK_B] = "reg_sspm_apsrc_req_0_mask_b ",
	[PW_REG_SSPM_VRF18_REQ_0_MASK_B] = "reg_sspm_vrf18_req_0_mask_b ",
	[PW_REG_SSPM_DDR_EN_0_MASK_B] = "reg_sspm_ddr_en_0_mask_b ",
	[PW_REG_SCP_SRCCLKENA_MASK_B] = "reg_scp_srcclkena_mask_b ",
	[PW_REG_SCP_INFRA_REQ_MASK_B] = "reg_scp_infra_req_mask_b ",
	[PW_REG_SCP_APSRC_REQ_MASK_B] = "reg_scp_apsrc_req_mask_b ",
	[PW_REG_SCP_VRF18_REQ_MASK_B] = "reg_scp_vrf18_req_mask_b ",
	[PW_REG_SCP_DDR_EN_MASK_B] = "reg_scp_ddr_en_mask_b ",
	[PW_REG_AUDIO_DSP_SRCCLKENA_MASK_B] = "reg_audio_dsp_srcclkena_mask_b",
	[PW_REG_AUDIO_DSP_INFRA_REQ_MASK_B] = "reg_audio_dsp_infra_req_mask_b",
	[PW_REG_AUDIO_DSP_APSRC_REQ_MASK_B] = "reg_audio_dsp_apsrc_req_mask_b",
	[PW_REG_AUDIO_DSP_VRF18_REQ_MASK_B] = "reg_audio_dsp_vrf18_req_mask_b",
	[PW_REG_AUDIO_DSP_DDR_EN_MASK_B] = "reg_audio_dsp_ddr_en_mask_b ",
	[PW_REG_APU_SRCCLKENA_MASK_B] = "reg_apu_srcclkena_mask_b ",
	[PW_REG_APU_INFRA_REQ_MASK_B] = "reg_apu_infra_req_mask_b ",
	[PW_REG_APU_APSRC_REQ_MASK_B] = "reg_apu_apsrc_req_mask_b ",
	[PW_REG_APU_VRF18_REQ_MASK_B] = "reg_apu_vrf18_req_mask_b ",
	[PW_REG_APU_DDR_EN_MASK_B] = "reg_apu_ddr_en_mask_b ",
	[PW_REG_CPUEB_SRCCLKENA_MASK_B] = "reg_cpueb_srcclkena_mask_b ",
	[PW_REG_CPUEB_INFRA_REQ_MASK_B] = "reg_cpueb_infra_req_mask_b ",
	[PW_REG_CPUEB_APSRC_REQ_MASK_B] = "reg_cpueb_apsrc_req_mask_b ",
	[PW_REG_CPUEB_VRF18_REQ_MASK_B] = "reg_cpueb_vrf18_req_mask_b ",
	[PW_REG_CPUEB_DDR_EN_MASK_B] = "reg_cpueb_ddr_en_mask_b ",
	[PW_REG_BAK_PSRI_SRCCLKENA_MASK_B] = "reg_bak_psri_srcclkena_mask_b ",
	[PW_REG_BAK_PSRI_INFRA_REQ_MASK_B] = "reg_bak_psri_infra_req_mask_b ",
	[PW_REG_BAK_PSRI_APSRC_REQ_MASK_B] = "reg_bak_psri_apsrc_req_mask_b ",
	[PW_REG_BAK_PSRI_VRF18_REQ_MASK_B] = "reg_bak_psri_vrf18_req_mask_b ",
	[PW_REG_BAK_PSRI_DDR_EN_MASK_B] = "reg_bak_psri_ddr_en_mask_b ",
	[PW_REG_CAM_DDREN_REQ_MASK_B] = "reg_cam_ddren_req_mask_b ",
	[PW_REG_IMG_DDREN_REQ_MASK_B] = "reg_img_ddren_req_mask_b ",
	/* SPM_SRC2_MASK */
	[PW_REG_MSDC0_SRCCLKENA_MASK_B] = "reg_msdc0_srcclkena_mask_b",
	[PW_REG_MSDC0_INFRA_REQ_MASK_B] = "reg_msdc0_infra_req_mask_b",
	[PW_REG_MSDC0_APSRC_REQ_MASK_B] = "reg_msdc0_apsrc_req_mask_b",
	[PW_REG_MSDC0_VRF18_REQ_MASK_B] = "reg_msdc0_vrf18_req_mask_b",
	[PW_REG_MSDC0_DDR_EN_MASK_B] = "reg_msdc0_ddr_en_mask_b",
	[PW_REG_MSDC1_SRCCLKENA_MASK_B] = "reg_msdc1_srcclkena_mask_b",
	[PW_REG_MSDC1_INFRA_REQ_MASK_B] = "reg_msdc1_infra_req_mask_b",
	[PW_REG_MSDC1_APSRC_REQ_MASK_B] = "reg_msdc1_apsrc_req_mask_b",
	[PW_REG_MSDC1_VRF18_REQ_MASK_B] = "reg_msdc1_vrf18_req_mask_b",
	[PW_REG_MSDC1_DDR_EN_MASK_B] = "reg_msdc1_ddr_en_mask_b",
	[PW_REG_MSDC2_SRCCLKENA_MASK_B] = "reg_msdc2_srcclkena_mask_b",
	[PW_REG_MSDC2_INFRA_REQ_MASK_B] = "reg_msdc2_infra_req_mask_b",
	[PW_REG_MSDC2_APSRC_REQ_MASK_B] = "reg_msdc2_apsrc_req_mask_b",
	[PW_REG_MSDC2_VRF18_REQ_MASK_B] = "reg_msdc2_vrf18_req_mask_b",
	[PW_REG_MSDC2_DDR_EN_MASK_B] = "reg_msdc2_ddr_en_mask_b",
	[PW_REG_UFS_SRCCLKENA_MASK_B] = "reg_ufs_srcclkena_mask_b",
	[PW_REG_UFS_INFRA_REQ_MASK_B] = "reg_ufs_infra_req_mask_b",
	[PW_REG_UFS_APSRC_REQ_MASK_B] = "reg_ufs_apsrc_req_mask_b",
	[PW_REG_UFS_VRF18_REQ_MASK_B] = "reg_ufs_vrf18_req_mask_b",
	[PW_REG_UFS_DDR_EN_MASK_B] = "reg_ufs_ddr_en_mask_b",
	[PW_REG_USB_SRCCLKENA_MASK_B] = "reg_usb_srcclkena_mask_b",
	[PW_REG_USB_INFRA_REQ_MASK_B] = "reg_usb_infra_req_mask_b",
	[PW_REG_USB_APSRC_REQ_MASK_B] = "reg_usb_apsrc_req_mask_b",
	[PW_REG_USB_VRF18_REQ_MASK_B] = "reg_usb_vrf18_req_mask_b",
	[PW_REG_USB_DDR_EN_MASK_B] = "reg_usb_ddr_en_mask_b",
	[PW_REG_PEXTP_P0_SRCCLKENA_MASK_B] = "reg_pextp_p0_srcclkena_mask_b",
	[PW_REG_PEXTP_P0_INFRA_REQ_MASK_B] = "reg_pextp_p0_infra_req_mask_b",
	[PW_REG_PEXTP_P0_APSRC_REQ_MASK_B] = "reg_pextp_p0_apsrc_req_mask_b",
	[PW_REG_PEXTP_P0_VRF18_REQ_MASK_B] = "reg_pextp_p0_vrf18_req_mask_b",
	[PW_REG_PEXTP_P0_DDR_EN_MASK_B] = "reg_pextp_p0_ddr_en_mask_b",
	/* SPM_SRC3_MASK */
	[PW_REG_PEXTP_P1_SRCCLKENA_MASK_B] = "reg_pextp_p1_srcclkena_mask_b",
	[PW_REG_PEXTP_P1_INFRA_REQ_MASK_B] = "reg_pextp_p1_infra_req_mask_b",
	[PW_REG_PEXTP_P1_APSRC_REQ_MASK_B] = "reg_pextp_p1_apsrc_req_mask_b",
	[PW_REG_PEXTP_P1_VRF18_REQ_MASK_B] = "reg_pextp_p1_vrf18_req_mask_b",
	[PW_REG_PEXTP_P1_DDR_EN_MASK_B] = "reg_pextp_p1_ddr_en_mask_b",
	[PW_REG_GCE0_INFRA_REQ_MASK_B] = "reg_gce0_infra_req_mask_b",
	[PW_REG_GCE0_APSRC_REQ_MASK_B] = "reg_gce0_apsrc_req_mask_b",
	[PW_REG_GCE0_VRF18_REQ_MASK_B] = "reg_gce0_vrf18_req_mask_b",
	[PW_REG_GCE0_DDR_EN_MASK_B] = "reg_gce0_ddr_en_mask_b",
	[PW_REG_GCE1_INFRA_REQ_MASK_B] = "reg_gce1_infra_req_mask_b",
	[PW_REG_GCE1_APSRC_REQ_MASK_B] = "reg_gce1_apsrc_req_mask_b",
	[PW_REG_GCE1_VRF18_REQ_MASK_B] = "reg_gce1_vrf18_req_mask_b",
	[PW_REG_GCE1_DDR_EN_MASK_B] = "reg_gce1_ddr_en_mask_b",
	[PW_REG_SPM_SRCCLKENA_RESERVED_MASK_B] = "reg_spm_srcclkena_reserved_mask_b",
	[PW_REG_SPM_INFRA_REQ_RESERVED_MASK_B] = "reg_spm_infra_req_reserved_mask_b",
	[PW_REG_SPM_APSRC_REQ_RESERVED_MASK_B] = "reg_spm_apsrc_req_reserved_mask_b",
	[PW_REG_SPM_VRF18_REQ_RESERVED_MASK_B] = "reg_spm_vrf18_req_reserved_mask_b",
	[PW_REG_SPM_DDR_EN_RESERVED_MASK_B] = "reg_spm_ddr_en_reserved_mask_b",
	[PW_REG_DISP0_APSRC_REQ_MASK_B] = "reg_disp0_apsrc_req_mask_b",
	[PW_REG_DISP0_DDR_EN_MASK_B] = "reg_disp0_ddr_en_mask_b",
	[PW_REG_DISP1_APSRC_REQ_MASK_B] = "reg_disp1_apsrc_req_mask_b",
	[PW_REG_DISP1_DDR_EN_MASK_B] = "reg_disp1_ddr_en_mask_b",
	[PW_REG_DISP2_APSRC_REQ_MASK_B] = "reg_disp2_apsrc_req_mask_b",
	[PW_REG_DISP2_DDR_EN_MASK_B] = "reg_disp2_ddr_en_mask_b",
	[PW_REG_DISP3_APSRC_REQ_MASK_B] = "reg_disp3_apsrc_req_mask_b",
	[PW_REG_DISP3_DDR_EN_MASK_B] = "reg_disp3_ddr_en_mask_b",
	[PW_REG_INFRASYS_APSRC_REQ_MASK_B] = "reg_infrasys_apsrc_req_mask_b",
	[PW_REG_INFRASYS_DDR_EN_MASK_B] = "reg_infrasys_ddr_en_mask_b",
	[PW_REG_CG_CHECK_SRCCLKENA_MASK_B] = "reg_cg_check_srcclkena_mask_b",
	[PW_REG_CG_CHECK_APSRC_REQ_MASK_B] = "reg_cg_check_apsrc_req_mask_b",
	[PW_REG_CG_CHECK_VRF18_REQ_MASK_B] = "reg_cg_check_vrf18_req_mask_b",
	[PW_REG_CG_CHECK_DDR_EN_MASK_B] = "reg_cg_check_ddr_en_mask_b",
	/* SPM_SRC4_MASK */
	[PW_REG_MCUSYS_MERGE_APSRC_REQ_MASK_B] = "reg_mcusys_merge_apsrc_req_mask_b",
	[PW_REG_MCUSYS_MERGE_DDR_EN_MASK_B] = "reg_mcusys_merge_ddr_en_mask_b",
	[PW_REG_DRAMC_MD32_INFRA_REQ_MASK_B] = "reg_dramc_md32_infra_req_mask_b",
	[PW_REG_DRAMC_MD32_VRF18_REQ_MASK_B] = "reg_dramc_md32_vrf18_req_mask_b",
	[PW_REG_DRAMC_MD32_DDR_EN_MASK_B] = "reg_dramc_md32_ddr_en_mask_b",
	[PW_REG_DVFSRC_EVENT_TRIGGER_MASK_B] = "reg_dvfsrc_event_trigger_mask_b",
	/* SPM_WAKEUP_EVENT_MASK2 */
	[PW_REG_SC_SW2SPM_WAKEUP_MASK_B] = "reg_sc_sw2spm_wakeup_mask_b",
	[PW_REG_SC_ADSP2SPM_WAKEUP_MASK_B] = "reg_sc_adsp2spm_wakeup_mask_b",
	[PW_REG_SC_SSPM2SPM_WAKEUP_MASK_B] = "reg_sc_sspm2spm_wakeup_mask_b",
	[PW_REG_SC_SCP2SPM_WAKEUP_MASK_B] = "reg_sc_scp2spm_wakeup_mask_b",
	[PW_REG_CSYSPWRUP_ACK_MASK] = "reg_csyspwrup_ack_mask",
	[PW_REG_CSYSPWRUP_REQ_MASK] = "reg_csyspwrup_req_mask",
	/* SPM_WAKEUP_EVENT_MASK */
	[PW_REG_WAKEUP_EVENT_MASK] = "reg_wakeup_event_mask",
	/* SPM_WAKEUP_EVENT_EXT_MASK */
	[PW_REG_EXT_WAKEUP_EVENT_MASK] = "reg_ext_wakeup_event_mask",
};

/**************************************
 * xxx_ctrl_show Function
 **************************************/
/* code gen by spm_pwr_ctrl_atf.pl, need struct pwr_ctrl */
static ssize_t mt8188_show_pwr_ctrl(int id, char *buf, size_t buf_sz)
{
	char *p = buf;
	size_t mSize = 0;
	int i;

	for (i = 0; i < PW_MAX_COUNT; i++) {
		mSize += scnprintf(p + mSize, buf_sz - mSize,
			"%s = 0x%zx\n",
			mt8188_pwr_ctrl_str[i],
			lpm_smc_spm_dbg(id, MT_LPM_SMC_ACT_GET, i, 0));
	}

	WARN_ON(buf_sz - mSize <= 0);

	return mSize;
}

/**************************************
 * xxx_ctrl_store Function
 **************************************/
/* code gen by spm_pwr_ctrl_atf.pl, need struct pwr_ctrl */
static ssize_t mt8188_store_pwr_ctrl(int id,	const char *buf, size_t count)
{
	u32 val;
	char cmd[64];
	int i;

	if (sscanf(buf, "%63s %x", cmd, &val) != 2)
		return -EPERM;
	pr_info("[SPM] pwr_ctrl: cmd = %s, val = 0x%x\n", cmd, val);

	for (i = 0 ; i < PW_MAX_COUNT; i++) {
		if (!strcmp(cmd, mt8188_pwr_ctrl_str[i])) {
			lpm_smc_spm_dbg(id, MT_LPM_SMC_ACT_SET, i, val);
			break;
		}
	}

	return count;
}

static ssize_t
mt8188_generic_spm_read(char *ToUserBuf, size_t sz, void *priv);

static ssize_t
mt8188_generic_spm_write(char *FromUserBuf, size_t sz, void *priv);

struct mt8188_SPM_ENTERY {
	const char *name;
	int mode;
	struct mtk_lp_sysfs_handle handle;
};

struct mt8188_SPM_NODE {
	const char *name;
	int mode;
	struct mtk_lp_sysfs_handle handle;
	struct mtk_lp_sysfs_op op;
};


struct mt8188_SPM_ENTERY mt8188_spm_root = {
	.name = "power",
	.mode = 0644,
};

struct mt8188_SPM_NODE mt8188_spm_idle = {
	.name = "idle_ctrl",
	.mode = 0644,
	.op = {
		.fs_read = mt8188_generic_spm_read,
		.fs_write = mt8188_generic_spm_write,
		.priv = (void *)&mt8188_spm_idle,
	},
};

struct mt8188_SPM_NODE mt8188_spm_suspend = {
	.name = "suspend_ctrl",
	.mode = 0644,
	.op = {
		.fs_read = mt8188_generic_spm_read,
		.fs_write = mt8188_generic_spm_write,
		.priv = (void *)&mt8188_spm_suspend,
	},
};

static ssize_t
mt8188_generic_spm_read(char *ToUserBuf, size_t sz, void *priv)
{
	int id = MT_SPM_DBG_SMC_UID_SUSPEND_PWR_CTRL;

	if (priv == &mt8188_spm_idle)
		id = MT_SPM_DBG_SMC_UID_IDLE_PWR_CTRL;
	return mt8188_show_pwr_ctrl(id, ToUserBuf, sz);
}

#include <mtk_lpm_sysfs.h>

static ssize_t
mt8188_generic_spm_write(char *FromUserBuf, size_t sz, void *priv)
{
	int id = MT_SPM_DBG_SMC_UID_SUSPEND_PWR_CTRL;

	if (priv == &mt8188_spm_idle)
		id = MT_SPM_DBG_SMC_UID_IDLE_PWR_CTRL;

	return mt8188_store_pwr_ctrl(id, FromUserBuf, sz);
}

static char *mt8188_spm_resource_str[MT_SPM_RES_MAX] = {
	[MT_SPM_RES_XO_FPM] = "XO_FPM",
	[MT_SPM_RES_CK_26M] = "CK_26M",
	[MT_SPM_RES_INFRA] = "INFRA",
	[MT_SPM_RES_SYSPLL] = "SYSPLL",
	[MT_SPM_RES_DRAM_S0] = "DRAM_S0",
	[MT_SPM_RES_DRAM_S1] = "DRAM_S1",
};

static ssize_t mt8188_spm_res_rq_read(char *ToUserBuf, size_t sz, void *priv)
{
	char *p = ToUserBuf;
	int i, s, u;
	unsigned int unum, uvalid, uname_i, uname_t;
	unsigned int rnum, rusage, per_usage;
	char uname[MT_LP_RQ_USER_NAME_LEN+1];

	mtk_dbg_spm_log("resource_num=%d, user_num=%d, user_valid=0x%x\n",
	    rnum = lpm_smc_spm_dbg(MT_SPM_DBG_SMC_UID_RES_NUM,
				       MT_LPM_SMC_ACT_GET, 0, 0),
	    unum = lpm_smc_spm_dbg(MT_SPM_DBG_SMC_UID_RES_USER_NUM,
				       MT_LPM_SMC_ACT_GET, 0, 0),
	    uvalid = lpm_smc_spm_dbg(MT_SPM_DBG_SMC_UID_RES_USER_VALID,
					 MT_LPM_SMC_ACT_GET, 0, 0));
	rusage = lpm_smc_spm_dbg(MT_SPM_DBG_SMC_UID_RES_USAGE,
				     MT_LPM_SMC_ACT_GET,
				     MT_LP_RQ_ID_ALL_USAGE, 0);
	mtk_dbg_spm_log("\n");
	mtk_dbg_spm_log("user [bit][valid]:\n");
	for (i = 0; i < unum; i++) {
		uname_i = lpm_smc_spm_dbg(MT_SPM_DBG_SMC_UID_RES_USER_NAME,
					    MT_LPM_SMC_ACT_GET, i, 0);
		for (s = 0, u = 0; s < MT_LP_RQ_USER_NAME_LEN;
		     s++, u += MT_LP_RQ_USER_CHAR_U) {
			uname_t = ((uname_i >> u) & MT_LP_RQ_USER_CHAR_MASK);
			uname[s] = (uname_t) ? (char)uname_t : ' ';
		}
		uname[s] = '\0';
		mtk_dbg_spm_log("%4s [%3d][%3s]\n", uname, i,
		    ((1<<i) & uvalid) ? "yes" : "no");
	}
	mtk_dbg_spm_log("\n");

	if (rnum != MT_SPM_RES_MAX) {
		mtk_dbg_spm_log("Platform resource amount mismatch\n");
		rnum = (rnum > MT_SPM_RES_MAX) ? MT_SPM_RES_MAX : rnum;
	}

	mtk_dbg_spm_log("resource [bit][user_usage][blocking]:\n");
	for (i = 0; i < rnum; i++) {
		mtk_dbg_spm_log("%8s [%3d][0x%08x][%3s]\n",
			mt8188_spm_resource_str[i], i,
			(per_usage =
			lpm_smc_spm_dbg(MT_SPM_DBG_SMC_UID_RES_USAGE,
					    MT_LPM_SMC_ACT_GET, i, 0)),
			((1<<i) & rusage) ? "yes" : "no"
		   );
	}
	mtk_dbg_spm_log("\n");
	mtk_dbg_spm_log("resource request command help:\n");
	mtk_dbg_spm_log("echo enable ${user_bit} > %s\n", MT_LP_RQ_NODE);
	mtk_dbg_spm_log("echo bypass ${user_bit} > %s\n", MT_LP_RQ_NODE);
	mtk_dbg_spm_log("echo request ${resource_bit} > %s\n", MT_LP_RQ_NODE);
	mtk_dbg_spm_log("echo release > %s\n", MT_LP_RQ_NODE);

	return p - ToUserBuf;
}

static ssize_t mt8188_spm_res_rq_write(char *FromUserBuf, size_t sz, void *priv)
{
	char cmd[128];
	int parm;

	if (sscanf(FromUserBuf, "%127s %x", cmd, &parm) == 2) {
		if (!strcmp(cmd, "bypass"))
			lpm_smc_spm_dbg(MT_SPM_DBG_SMC_UID_RES_USER_VALID,
					    MT_LPM_SMC_ACT_SET,
					    parm, 0);
		else if (!strcmp(cmd, "enable"))
			lpm_smc_spm_dbg(MT_SPM_DBG_SMC_UID_RES_USER_VALID,
					    MT_LPM_SMC_ACT_SET,
					    parm, 1);
		else if (!strcmp(cmd, "request"))
			lpm_smc_spm_dbg(MT_SPM_DBG_SMC_UID_RES_REQ,
					    MT_LPM_SMC_ACT_SET,
					    0, parm);
		return sz;
	} else if (sscanf(FromUserBuf, "%127s", cmd) == 1) {
		if (!strcmp(cmd, "release"))
			lpm_smc_spm_dbg(MT_SPM_DBG_SMC_UID_RES_REQ,
					    MT_LPM_SMC_ACT_CLR,
					    0, 0);
		return sz;
	} else if ((!kstrtoint(FromUserBuf, 10, &parm)) == 1) {
		return sz;
	}

	return -EINVAL;
}

static const struct mtk_lp_sysfs_op mt8188_spm_res_rq_fops = {
	.fs_read = mt8188_spm_res_rq_read,
	.fs_write = mt8188_spm_res_rq_write,
};

int lpm_spm_fs_init(void)
{
	int r;

	mtk_spm_sysfs_root_entry_create();
	mtk_spm_sysfs_entry_node_add("spm_resource_req", 0444
			, &mt8188_spm_res_rq_fops, NULL);

	r = mtk_lp_sysfs_entry_func_create(mt8188_spm_root.name,
					   mt8188_spm_root.mode, NULL,
					   &mt8188_spm_root.handle);
	if (!r) {
		mtk_lp_sysfs_entry_func_node_add(mt8188_spm_suspend.name,
						mt8188_spm_suspend.mode,
						&mt8188_spm_suspend.op,
						&mt8188_spm_root.handle,
						&mt8188_spm_suspend.handle);

		mtk_lp_sysfs_entry_func_node_add(mt8188_spm_idle.name,
						 mt8188_spm_idle.mode,
						 &mt8188_spm_idle.op,
						 &mt8188_spm_root.handle,
						 &mt8188_spm_idle.handle);
	}

	return r;
}

int lpm_spm_fs_deinit(void)
{
	return 0;
}

