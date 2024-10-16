/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef MTK_DRM_DRV_H
#define MTK_DRM_DRV_H

#include <drm/drm_fb_helper.h>
#include <drm/mediatek_drm.h>
#include <linux/types.h>
#include <linux/io.h>
#include <drm/drm_atomic.h>
#include "mtk_drm_ddp_comp.h"
#include "mtk_drm_plane.h"
#include "mtk_drm_crtc.h"
#include "mtk_drm_ddp.h"
#include "mtk_drm_session.h"
#include "mtk_drm_helper.h"

#define MAX_CONNECTOR 3

#ifdef CONFIG_MTK_DRM_BRINGUP_STAGE
#define MTK_DRM_BRINGUP_STAGE
#define CONFIG_MTK_DISP_NO_LK
#endif

#ifndef MTK_DRM_BRINGUP_STAGE
#define MTK_DRM_ESD_SUPPORT
#if IS_ENABLED(CONFIG_MTK_MMDVFS)
#define MTK_DISP_MMQOS_SUPPORT
#define MTK_DISP_MMDVFS_SUPPORT
#endif
#endif
#define MTK_DRM_FENCE_SUPPORT
#if IS_ENABLED(CONFIG_MTK_CMDQ_MBOX_EXT)
#define MTK_DRM_CMDQ_ASYNC
#define CONFIG_MTK_DISPLAY_CMDQ
#define MTK_FILL_MIPI_IMPEDANCE
#endif

#ifndef MTK_DRM_BRINGUP_STAGE
#if IS_ENABLED(CONFIG_MTK_IOMMU)
#define CONFIG_MTK_DISPLAY_M4U
#endif
#endif

struct device;
struct device_node;
struct drm_crtc;
struct drm_device;
struct drm_property;
struct regmap;

struct mtk_atomic_state {
	struct drm_atomic_state base;
	struct list_head list;
	struct kref kref;
};

struct mtk_fake_eng_reg {
	unsigned int CG_idx;
	unsigned int CG_bit;
	bool share_port;
};

struct mtk_fake_eng_data {
	int fake_eng_num;
	const struct mtk_fake_eng_reg *fake_eng_reg;
};

struct mtk_mmsys_driver_data {
	const struct mtk_crtc_path_data *main_path_data;
	const struct mtk_crtc_path_data *ext_path_data;
	const struct mtk_crtc_path_data *third_path_data;
	enum mtk_mmsys_id mmsys_id;
	bool shadow_register;
	const struct mtk_session_mode_tb *mode_tb;
	void (*sodi_config)(struct drm_device *drm, enum mtk_ddp_comp_id id,
			struct cmdq_pkt *handle, void *data);
	const struct mtk_fake_eng_data *fake_eng_data;
	bool bypass_infra_ddr_control;
	bool has_smi_limitation;
	bool doze_ctrl_pmic;
	bool can_compress_rgb565;
};

struct mtk_drm_lyeblob_ids {
	uint32_t lye_idx;
	uint32_t frame_weight;
	uint32_t hrt_num;
	int32_t ddp_blob_id;
	int32_t ref_cnt;
	int32_t ref_cnt_mask;
	int32_t free_cnt_mask;
	int32_t lye_plane_blob_id[MAX_CRTC][OVL_LAYER_NR];
	struct list_head list;
};

struct mtk_drm_private {
	struct drm_device *drm;
	struct device *dma_dev;
	struct device *mmsys_dev;

	struct drm_crtc *crtc[MAX_CRTC];
	unsigned int num_pipes;

	unsigned int session_id[MAX_SESSION_COUNT];
	unsigned int num_sessions;
	enum MTK_DRM_SESSION_MODE session_mode;
	atomic_t crtc_present[MAX_CRTC];
	atomic_t crtc_sf_present[MAX_CRTC];

	struct device_node *mutex_node;
	struct device *mutex_dev;
	unsigned int dispsys_num;
	void __iomem *config_regs;
	resource_size_t config_regs_pa;
	void __iomem *side_config_regs;
	resource_size_t side_config_regs_pa;
	void __iomem *infra_regs;
	resource_size_t infra_regs_pa;
	const struct mtk_mmsys_reg_data *reg_data;
	struct device_node *comp_node[DDP_COMPONENT_ID_MAX];
	struct mtk_ddp_comp *ddp_comp[DDP_COMPONENT_ID_MAX];
	const struct mtk_mmsys_driver_data *data;

	struct {
		struct drm_atomic_state *state;
		struct work_struct work;
		struct mutex lock;
	} commit;

	struct drm_atomic_state *suspend_state;

	struct {
		struct work_struct work;
		struct list_head list;
		spinlock_t lock;
	} unreference;

	/* property */
	struct drm_property *crtc_property[MAX_CRTC][CRTC_PROP_MAX];

	struct mtk_ddp_fb_info fb_info;

	struct list_head lyeblob_head;
	struct mutex lyeblob_list_mutex;
	struct task_struct *fence_release_thread[MAX_CRTC-1];

	/* variable for repaint */
	struct {
		wait_queue_head_t wq;
		struct list_head job_queue;
		struct list_head job_pool;
	} repaint_data;

	struct mtk_drm_helper *helper_opt;

	atomic_t idle_need_repaint;
	atomic_t rollback_all;

	unsigned int top_clk_num;
	struct clk **top_clk;
	int *top_clk_related_sub_sys;
	bool power_state;
	struct device_node *power_node[MAX_CRTC];
	struct device *power_dev[MAX_CRTC];

	/* for rpo caps info */
	unsigned int rsz_in_max[2];

	struct icc_path *hrt_bw_request;

	struct pinctrl *pctrl;

#ifdef DRM_MMPATH
	int HWC_gpid; // for mmpath auto gen
#endif

	int need_vds_path_switch;
	int vds_path_switch_dirty;
	int vds_path_switch_done;
	int vds_path_enable;

	bool need_cwb_path_disconnect;
	bool cwb_is_preempted;

	bool dma_parms_allocated;

	bool already_first_config;
};

struct mtk_drm_property {
	int flags;
	char *name;
	unsigned long min;
	unsigned long max;
	unsigned long val;
};

struct repaint_job_t {
	struct list_head link;
	unsigned int type;
};

#define LCM_FPS_ARRAY_SIZE  32
struct lcm_fps_ctx_t {
	atomic_t is_inited;
	spinlock_t lock;
	atomic_t skip_update;
	unsigned int dsi_mode;
	unsigned int head_idx;
	unsigned int num;
	unsigned int fps;
	unsigned long long last_ns;
	unsigned long long array[LCM_FPS_ARRAY_SIZE];
};

#define DISP_LARB_COUNT 1
struct disp_iommu_device {
	struct platform_device *larb_pdev[DISP_LARB_COUNT];
	struct platform_device *iommu_pdev;
	unsigned int inited;
};

struct tag_videolfb {
	u64 fb_base;
	u32 islcmfound;
	u32 fps;
	u32 vram;
	char lcmname[1]; /* this is the minimum size */
};

struct disp_iommu_device *disp_get_iommu_dev(void);

extern struct platform_driver mtk_ddp_driver;
extern struct platform_driver mtk_disp_tdshp_driver;
extern struct platform_driver mtk_disp_color_driver;
extern struct platform_driver mtk_disp_ccorr_driver;
extern struct platform_driver mtk_disp_c3d_driver;
extern struct platform_driver mtk_disp_gamma_driver;
extern struct platform_driver mtk_disp_aal_driver;
extern struct platform_driver mtk_dmdp_aal_driver;
extern struct platform_driver mtk_disp_postmask_driver;
extern struct platform_driver mtk_disp_dither_driver;
extern struct platform_driver mtk_disp_chist_driver;
extern struct platform_driver mtk_disp_ovl_driver;
extern struct platform_driver mtk_disp_rdma_driver;
extern struct platform_driver mtk_disp_wdma_driver;
extern struct platform_driver mtk_disp_rsz_driver;
extern struct platform_driver mtk_dsi_driver;
extern struct platform_driver ext_mipi_phy_driver;
extern struct platform_driver mtk_mipi_tx_driver;
extern struct platform_driver mtk_lvds_driver;
extern struct platform_driver mtk_lvds_tx_driver;
extern struct platform_driver mtk_disp_dsc_driver;
extern struct platform_driver mtk_disp_cm_driver;
extern struct platform_driver mtk_disp_spr_driver;
extern struct lcm_fps_ctx_t lcm_fps_ctx[MAX_CRTC];
extern struct platform_driver mtk_disp_merge_driver;

extern struct platform_driver mtk_disp_power_dev_driver;
extern struct platform_driver mtk_dpi_driver;
extern struct platform_driver mtk_ethdr_driver;
extern struct platform_driver mtk_disp_pseudo_ovl_driver;
extern struct platform_driver mtk_pseudo_ovl_larb_driver;


extern struct platform_driver mtk_dpintf_driver;
#ifdef CONFIG_MTK_DPTX_SUPPORT
extern struct platform_driver mtk_dp_driver;
#endif
#ifdef CONFIG_MTK_EDPTX_SUPPORT
extern struct platform_driver mtk_edp_driver;
#endif

void mtk_atomic_state_put_queue(struct drm_atomic_state *state);
void mtk_drm_fence_update(unsigned int fence_idx, unsigned int index);
void drm_trigger_repaint(enum DRM_REPAINT_TYPE type,
			 struct drm_device *drm_dev);
int mtk_drm_suspend_release_fence(struct device *dev);
void mtk_drm_suspend_release_present_fence(struct device *dev,
					   unsigned int index);
void mtk_drm_suspend_release_sf_present_fence(struct device *dev,
					      unsigned int index);
void mtk_drm_top_clk_prepare_enable(struct drm_device *drm, int crtc_id);
void mtk_drm_top_clk_disable_unprepare(struct drm_device *drm, int crtc_id);
struct mtk_panel_params *mtk_drm_get_lcm_ext_params(struct drm_crtc *crtc);
struct mtk_panel_funcs *mtk_drm_get_lcm_ext_funcs(struct drm_crtc *crtc);
bool mtk_drm_session_mode_is_decouple_mode(struct drm_device *dev);
bool mtk_drm_session_mode_is_mirror_mode(struct drm_device *dev);
bool mtk_drm_top_clk_isr_get(char *master);
void mtk_drm_top_clk_isr_put(char *master);
int lcm_fps_ctx_init(struct drm_crtc *crtc);
int lcm_fps_ctx_reset(struct drm_crtc *crtc);
int lcm_fps_ctx_update(unsigned long long cur_ns,
		unsigned int crtc_id, unsigned int mode);
int mtk_mipi_clk_change(struct drm_crtc *crtc, unsigned int data_rate);
bool mtk_drm_lcm_is_connect(void);
int _parse_tag_videolfb(unsigned int *vramsize, phys_addr_t *fb_base,
	unsigned int *fps);
void mtk_drm_suspend(struct device *dev);
void mtk_drm_resume(struct device *dev);

extern void mtk_mmdvfs_enable(void);

#endif /* MTK_DRM_DRV_H */
