/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 * Author: Dennis YC Hsieh <dennis-yc.hsieh@mediatek.com>
 */

#ifndef __MTK_MML_DRM_ADAPTOR_H__
#define __MTK_MML_DRM_ADAPTOR_H__

#include <linux/platform_device.h>
#include <linux/types.h>
#include "mtk-mml.h"
#include "mtk_drm_ddp_comp.h"

struct mml_drm_ctx;

struct mml_drm_param {
	/* helps calculate inline rotate support */
	u32 vblank_interval;

	/* set true if display uses dual pipe */
	bool dual;

	/* set true if display uses vdo mode, false for cmd mode */
	bool vdo_mode;

	/* The height of racing mode for each output tile in pixel. */
	u8 racing_height;
};

/*
 * mml_drm_query_cap - Query current running mode and possible support mode
 * for specific frame info.
 *
 * @ctx:	Context of mml drm adaptor. Get by mml_drm_get_context API.
 * @info:	Frame info which describe frame process by mml.
 *
 * Return:	Capability result of target mode by giving info.
 */
enum mml_mode mml_drm_query_cap(struct mml_drm_ctx *ctx,
				struct mml_frame_info *info);

/*
 * mml_drm_get_context - Get mml drm context to control mml.
 *
 * pdev:	The mml driver platform device pointer. Client driver must call
 *		mml_get_plat_device by giving user client driver platoform
 *		device which contains "mediatek,mml" property link to mml node
 *		in dts.
 * @dual:	set true if display use dual pipe
 *
 * Return:	The drm context pointer to represent mml driver instance.
 *
 */
struct mml_drm_ctx *mml_drm_get_context(struct platform_device *pdev,
	struct mml_drm_param *disp);

/*
 * mml_drm_put_context - Release mml drm context and related cached info
 * inside this context.
 *
 * @ctx:	The drm context instance.
 */
void mml_drm_put_context(struct mml_drm_ctx *ctx);

/*
 * mml_drm_racing_config_sync - append event sync instructions to disp pkt
 *
 * @ctx:	The drm context instance.
 * @pkt:	The pkt to append cmdq instructions, which helps this pkt
 *		and mml pkt execute at same time.
 *
 * return:	0 if success and < 0 error no if fail
 */
s32 mml_drm_racing_config_sync(struct mml_drm_ctx *ctx, struct cmdq_pkt *pkt);

/*
 * mml_drm_submit - submit mml job
 *
 * @ctx:	Context of mml drm adaptor. Get by mml_drm_get_context API.
 * @submit:	Frame info which want mml driver to execute.
 *
 * Return:	Result of submit. In value < 0 case job did not send to mml
 *		driver core.
 */
s32 mml_drm_submit(struct mml_drm_ctx *ctx, struct mml_submit *submit);

/*
 * mml_ddp_comp_register - register ddp component to drm
 *
 * @drm:	Device of drm.
 * @comp:	The component that will be reigster to drm.
 *
 * Return:	Result of component register. In case value < 0 means registeration fail
 */
int mml_ddp_comp_register(struct drm_device *drm, struct mtk_ddp_comp *comp);


/*
 * mml_ddp_comp_unregister - unregister ddp component to drm
 *
 * @drm:	Device of drm.
 * @comp:	The component that will be reigster to drm.
 *
 * Return:	None
 */
void mml_ddp_comp_unregister(struct drm_device *drm, struct mtk_ddp_comp *comp);

#endif	/* __MTK_MML_DRM_ADAPTOR_H__ */
