// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "mtk_drm_ddp_addon.h"
#include "mtk_drm_drv.h"
#include "mtk_rect.h"
#include "mtk_dump.h"
#include "mtk_drm_ddp_comp.h"
#include "mtk_drm_crtc.h"

static const int disp_rsz_path[] = {
	DDP_COMPONENT_RSZ0,
};

static const int disp_rsz_path_v2[] = {
	DDP_COMPONENT_OVL0_2L_VIRTUAL0,
	DDP_COMPONENT_RSZ0,
};

static const int disp_rsz_path_v3[] = {
	DDP_COMPONENT_OVL1_2L_VIRTUAL0,
	DDP_COMPONENT_RSZ1,
};

static const int dmdp_pq_with_rdma_path[] = {
	DDP_COMPONENT_DMDP_RDMA0,  DDP_COMPONENT_DMDP_HDR0,
	DDP_COMPONENT_DMDP_AAL0,   DDP_COMPONENT_DMDP_RSZ0,
	DDP_COMPONENT_DMDP_TDSHP0,
};

static const int disp_wdma0_path[] = {
	DDP_COMPONENT_WDMA0,
};

static const int disp_wdma1_path[] = {
	DDP_COMPONENT_WDMA1,
};

static const struct mtk_addon_path_data addon_module_path[ADDON_MODULE_NUM] = {
		[DISP_RSZ] = {
				.path = disp_rsz_path,
				.path_len = ARRAY_SIZE(disp_rsz_path),
			},
		[DISP_RSZ_v2] = {
				.path = disp_rsz_path_v2,
				.path_len = ARRAY_SIZE(disp_rsz_path_v2),
			},
		[DISP_RSZ_v3] = {
				.path = disp_rsz_path_v3,
				.path_len = ARRAY_SIZE(disp_rsz_path_v3),
			},
		[DMDP_PQ_WITH_RDMA] = {
				.path = dmdp_pq_with_rdma_path,
				.path_len = ARRAY_SIZE(dmdp_pq_with_rdma_path),
			},
		[DISP_WDMA0] = {
				.path = disp_wdma0_path,
				.path_len = ARRAY_SIZE(disp_wdma0_path),
			},
		[DISP_WDMA1] = {
				.path = disp_wdma1_path,
				.path_len = ARRAY_SIZE(disp_wdma1_path),
			},
};

const struct mtk_addon_path_data *
mtk_addon_module_get_path(enum addon_module module)
{
	return &addon_module_path[module];
}

const struct mtk_addon_scenario_data *
mtk_addon_get_scenario_data(const char *source, struct drm_crtc *crtc,
			    enum addon_scenario scn)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);

	if (scn < NONE || scn >= ADDON_SCN_NR)
		goto err;

	if (mtk_crtc->path_data && mtk_crtc->path_data->addon_data)
		return &mtk_crtc->path_data->addon_data[scn];

err:
	DDPPR_ERR("[%s] crtc%d cannot get addon data scn[%u]\n", source,
		  drm_crtc_index(crtc), scn);
	return NULL;
}
const struct mtk_addon_scenario_data *
mtk_addon_get_scenario_data_dual(const char *source, struct drm_crtc *crtc,
			    enum addon_scenario scn)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);

	if ((scn < NONE) || (scn > TRIPLE_DISP)) {
		DDPPR_ERR("[%s] crtc%d scn is wrong\n", source,
				drm_crtc_index(crtc));
		return NULL;
	}

	if (mtk_crtc->path_data && mtk_crtc->path_data->addon_data_dual)
		return &mtk_crtc->path_data->addon_data_dual[scn];

	DDPPR_ERR("[%s] crtc%d cannot get addon data\n", source,
		  drm_crtc_index(crtc));
	return NULL;
}

bool mtk_addon_scenario_support(struct drm_crtc *crtc, enum addon_scenario scn)
{
	const struct mtk_addon_scenario_data *data =
		mtk_addon_get_scenario_data(__func__, crtc, scn);

	if (data && (data->module_num > 0 || scn == NONE))
		return true;

	return false;
}

static void mtk_addon_path_start(struct drm_crtc *crtc,
				 const struct mtk_addon_path_data *path_data,
				 struct cmdq_pkt *cmdq_handle)
{
	int i;
	struct mtk_ddp_comp *add_comp = NULL;
	struct mtk_drm_private *priv = crtc->dev->dev_private;

	for (i = 0; i < path_data->path_len; i++) {
		if (mtk_ddp_comp_get_type(path_data->path[i])
			== MTK_DISP_VIRTUAL)
			continue;

		add_comp = priv->ddp_comp[path_data->path[i]];
		mtk_ddp_comp_start(add_comp, cmdq_handle);
	}
}

static void mtk_addon_path_stop(struct drm_crtc *crtc,
				const struct mtk_addon_path_data *path_data,
				struct cmdq_pkt *cmdq_handle)
{
	int i;
	struct mtk_ddp_comp *add_comp = NULL;
	struct mtk_drm_private *priv = crtc->dev->dev_private;

	for (i = 0; i < path_data->path_len; i++) {
		if (mtk_ddp_comp_get_type(path_data->path[i])
			== MTK_DISP_VIRTUAL)
			continue;

		add_comp = priv->ddp_comp[path_data->path[i]];
		mtk_ddp_comp_stop(add_comp, cmdq_handle);
	}
}

static void
mtk_addon_path_connect_and_add_mutex(struct drm_crtc *crtc,
				     unsigned int ddp_mode,
				     const struct mtk_addon_module_data *module,
				     struct cmdq_pkt *cmdq_handle)
{
	int i;
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	int mutex_id =
		mtk_crtc_get_mutex_id(crtc, ddp_mode, module->attach_comp);
	const struct mtk_addon_path_data *path_data =
		mtk_addon_module_get_path(module->module);

	if (mutex_id < 0) {
		DDPPR_ERR("invalid mutex id:%d\n", mutex_id);
		return;
	}

	for (i = 0; i < path_data->path_len - 1; i++) {
		mtk_ddp_add_comp_to_path_with_cmdq(mtk_crtc, path_data->path[i],
						   path_data->path[i + 1],
						   cmdq_handle);
		mtk_disp_mutex_add_comp_with_cmdq(
			mtk_crtc, path_data->path[i],
			mtk_crtc_is_frame_trigger_mode(crtc), cmdq_handle,
			mutex_id);
	}

	mtk_disp_mutex_add_comp_with_cmdq(
		mtk_crtc, path_data->path[path_data->path_len - 1],
		mtk_crtc_is_frame_trigger_mode(crtc), cmdq_handle, mutex_id);
}

static void mtk_addon_path_disconnect_and_remove_mutex(
	struct drm_crtc *crtc, unsigned int ddp_mode,
	const struct mtk_addon_module_data *module,
	struct cmdq_pkt *cmdq_handle)
{
	int i;
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	int mutex_id =
		mtk_crtc_get_mutex_id(crtc, ddp_mode, module->attach_comp);
	const struct mtk_addon_path_data *path_data =
		mtk_addon_module_get_path(module->module);

	if (mutex_id < 0) {
		DDPPR_ERR("invalid mutex id:%d\n", mutex_id);
		return;
	}

	for (i = 0; i < path_data->path_len - 1; i++) {
		mtk_ddp_remove_comp_from_path_with_cmdq(
			mtk_crtc, path_data->path[i], path_data->path[i + 1],
			cmdq_handle);
		mtk_disp_mutex_remove_comp_with_cmdq(
			mtk_crtc, path_data->path[i], cmdq_handle, mutex_id);
	}

	mtk_disp_mutex_remove_comp_with_cmdq(
		mtk_crtc, path_data->path[path_data->path_len - 1], cmdq_handle,
		mutex_id);
}

void mtk_addon_connect_between(struct drm_crtc *crtc, unsigned int ddp_mode,
			       const struct mtk_addon_module_data *module_data,
			       union mtk_addon_config *addon_config,
			       struct cmdq_pkt *cmdq_handle)

{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_drm_private *priv = crtc->dev->dev_private;
	struct mtk_ddp_comp *comp = NULL;
	enum mtk_ddp_comp_id attach_comp_id, next_attach_comp_id, prev_comp_id,
		cur_comp_id, next_comp_id;
	const struct mtk_addon_path_data *path_data =
		mtk_addon_module_get_path(module_data->module);
	int i, j;
	unsigned int addon_idx;

	attach_comp_id =
		mtk_crtc_find_comp(crtc, ddp_mode, module_data->attach_comp);

	if (module_data->attach_comp < 0) {
		DDPPR_ERR("Invalid attach_comp value\n");
		return;
	}

	if (attach_comp_id == -1 || attach_comp_id >= DDP_COMPONENT_ID_MAX) {
		comp = priv->ddp_comp[module_data->attach_comp];
		DDPPR_ERR("Attach module:%s is not in path mode %d\n",
			  mtk_dump_comp_str(comp), ddp_mode);
		return;
	}

	next_attach_comp_id = mtk_crtc_find_next_comp(crtc, ddp_mode,
						      module_data->attach_comp);

	if (module_data->attach_comp < 0) {
		DDPPR_ERR("Invalid attach_comp value\n");
		return;
	}

	if (next_attach_comp_id == -1) {
		comp = priv->ddp_comp[module_data->attach_comp];
		DDPPR_ERR("Attach module:%s has not a next comp in path mode\n",
			  mtk_dump_comp_str(comp));
		return;
	}

	/* 1. remove original path*/
	mtk_crtc_disconnect_path_between_component(
		crtc, ddp_mode, attach_comp_id, next_attach_comp_id,
		cmdq_handle);

	/* 2. connect subpath and add mutex*/
	mtk_addon_path_connect_and_add_mutex(crtc, ddp_mode, module_data,
					     cmdq_handle);

	/* 3. add subpath to main path */
	mtk_ddp_add_comp_to_path_with_cmdq(mtk_crtc, attach_comp_id,
					   path_data->path[0], cmdq_handle);
	mtk_ddp_add_comp_to_path_with_cmdq(
		mtk_crtc, path_data->path[path_data->path_len - 1],
		next_attach_comp_id, cmdq_handle);

	/* 4. config module */
	/* config attach comp */
	comp = priv->ddp_comp[attach_comp_id];
	prev_comp_id = mtk_crtc_find_prev_comp(crtc, ddp_mode, attach_comp_id);
	cur_comp_id = comp->id;
	next_comp_id = path_data->path[0];
	for (i = 0; i < path_data->path_len; i++)
		if (mtk_ddp_comp_get_type(path_data->path[i]) !=
		    MTK_DISP_VIRTUAL) {
			next_comp_id = path_data->path[i];
			break;
		}

	mtk_ddp_comp_addon_config(comp, prev_comp_id, next_comp_id,
				  addon_config, cmdq_handle);
	/* config subpath comp without last comp*/
	addon_idx = i;
	for (i = addon_idx; i < path_data->path_len; i++) {
		if (mtk_ddp_comp_get_type(path_data->path[i]) ==
		    MTK_DISP_VIRTUAL)
			continue;

		addon_idx = i;
		comp = priv->ddp_comp[path_data->path[i]];
		prev_comp_id = cur_comp_id;
		cur_comp_id = comp->id;
		comp->mtk_crtc = mtk_crtc;

		for (j = i + 1; j < path_data->path_len; j++)
			if (mtk_ddp_comp_get_type(path_data->path[j]) !=
			    MTK_DISP_VIRTUAL) {
				next_comp_id = path_data->path[j];
				mtk_ddp_comp_addon_config(
					comp, prev_comp_id, next_comp_id,
					addon_config, cmdq_handle);
				break;
			}
	}
	/* config subpath last comp */
	comp = priv->ddp_comp[path_data->path[addon_idx]];
	prev_comp_id = cur_comp_id;
	cur_comp_id = comp->id;
	next_comp_id = next_attach_comp_id;
	mtk_ddp_comp_addon_config(comp, prev_comp_id, next_comp_id,
				  addon_config, cmdq_handle);
	/* config next attach comp */
	comp = priv->ddp_comp[next_attach_comp_id];
	prev_comp_id = cur_comp_id;
	cur_comp_id = comp->id;
	next_comp_id =
		mtk_crtc_find_next_comp(crtc, ddp_mode, next_attach_comp_id);
	mtk_ddp_comp_addon_config(comp, prev_comp_id, next_comp_id,
				  addon_config, cmdq_handle);

	/* 5. start addon module */
	mtk_addon_path_start(crtc, path_data, cmdq_handle);
}

void mtk_addon_connect_embed(struct drm_crtc *crtc, unsigned int ddp_mode,
				   const struct mtk_addon_module_data *module_data,
				   union mtk_addon_config *addon_config,
				   struct cmdq_pkt *cmdq_handle)

{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_drm_private *priv = crtc->dev->dev_private;
	struct mtk_ddp_comp *comp = NULL;
	enum mtk_ddp_comp_id attach_comp_id, prev_comp_id,
		cur_comp_id, next_comp_id;
	const struct mtk_addon_path_data *path_data =
		mtk_addon_module_get_path(module_data->module);
	int i, j;
	unsigned int addon_idx;

	attach_comp_id =
		mtk_crtc_find_comp(crtc, ddp_mode, module_data->attach_comp);
	if (attach_comp_id == -1 || attach_comp_id >= DDP_COMPONENT_ID_MAX) {
		if (module_data->attach_comp >= 0)
			comp = priv->ddp_comp[module_data->attach_comp];
		DDPPR_ERR("Attach module:%s is not in path mode %d\n",
			  mtk_dump_comp_str(comp), ddp_mode);
		return;
	}

	/* 1. connect subpath and add mutex*/
	mtk_addon_path_connect_and_add_mutex(crtc, ddp_mode, module_data,
						 cmdq_handle);

	/* 2. embed subpath to main comp */
	mtk_ddp_add_comp_to_path_with_cmdq(
		mtk_crtc, path_data->path[0],
		attach_comp_id, cmdq_handle);
	mtk_ddp_add_comp_to_path_with_cmdq(
		mtk_crtc, path_data->path[path_data->path_len - 1],
		attach_comp_id, cmdq_handle);

	/* 3. config module */
	comp = priv->ddp_comp[attach_comp_id];
	prev_comp_id = mtk_crtc_find_prev_comp(crtc, ddp_mode, attach_comp_id);
	cur_comp_id = comp->id;

	next_comp_id = path_data->path[0];
	for (i = 0; i < path_data->path_len; i++)
		if (mtk_ddp_comp_get_type(path_data->path[i]) !=
			MTK_DISP_VIRTUAL) {
			next_comp_id = path_data->path[i];
			break;
		}

	mtk_ddp_comp_addon_config(comp, prev_comp_id, next_comp_id,
				  addon_config, cmdq_handle);

	addon_idx = i;
	for (; i < path_data->path_len; i++) {
		if (mtk_ddp_comp_get_type(path_data->path[i]) ==
			MTK_DISP_VIRTUAL)
			continue;

		addon_idx = i;
		comp = priv->ddp_comp[path_data->path[i]];
		prev_comp_id = cur_comp_id;
		cur_comp_id = comp->id;

		for (j = i + 1; j < path_data->path_len; j++)
			if (mtk_ddp_comp_get_type(path_data->path[j]) !=
				MTK_DISP_VIRTUAL) {
				next_comp_id = path_data->path[j];
				mtk_ddp_comp_addon_config(
					comp, prev_comp_id, next_comp_id,
					addon_config, cmdq_handle);
				break;
			}
	}

	if (path_data->path[addon_idx] < 0) {
		DDPMSG("invalid path: addon_idx=%d\n", addon_idx);
		return;
	}

	if (path_data->path[addon_idx] >= 0) {
		comp = priv->ddp_comp[path_data->path[addon_idx]];
		prev_comp_id = cur_comp_id;
		cur_comp_id = comp->id;
		next_comp_id = comp->id;
		mtk_ddp_comp_addon_config(comp, prev_comp_id, next_comp_id,
				  addon_config, cmdq_handle);
	}

	/* 4. start addon module */
	mtk_addon_path_start(crtc, path_data, cmdq_handle);
}

void mtk_addon_disconnect_between(
	struct drm_crtc *crtc, unsigned int ddp_mode,
	const struct mtk_addon_module_data *module_data,
	union mtk_addon_config *addon_config, struct cmdq_pkt *cmdq_handle)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_drm_private *priv = crtc->dev->dev_private;
	struct mtk_ddp_comp *comp = NULL;
	enum mtk_ddp_comp_id attach_comp_id, next_attach_comp_id, prev_comp_id,
		next_comp_id;
	const struct mtk_addon_path_data *path_data =
		mtk_addon_module_get_path(module_data->module);

	attach_comp_id =
		mtk_crtc_find_comp(crtc, ddp_mode, module_data->attach_comp);

	if (module_data->attach_comp < 0) {
		DDPPR_ERR("Invalid attach_comp value\n");
		return;
	}

	if (attach_comp_id == -1 || attach_comp_id >= DDP_COMPONENT_ID_MAX) {
		comp = priv->ddp_comp[module_data->attach_comp];
		DDPPR_ERR("Attach module:%s is not in path mode %d\n",
			  mtk_dump_comp_str(comp), ddp_mode);
		return;
	}

	next_attach_comp_id = mtk_crtc_find_next_comp(crtc, ddp_mode,
						      module_data->attach_comp);

	if (module_data->attach_comp < 0) {
		DDPPR_ERR("Invalid attach_comp value\n");
		return;
	}

	if (next_attach_comp_id == -1) {
		comp = priv->ddp_comp[module_data->attach_comp];
		DDPPR_ERR(
			"Attach module:%s has not a next comp in path mode %d\n",
			mtk_dump_comp_str(comp), ddp_mode);
		return;
	}

	/* 1. stop addon module*/
	mtk_addon_path_stop(crtc, path_data, cmdq_handle);

	/* 2. remove subpath from main path */
	mtk_ddp_remove_comp_from_path_with_cmdq(
		mtk_crtc, attach_comp_id, path_data->path[0], cmdq_handle);
	mtk_ddp_remove_comp_from_path_with_cmdq(
		mtk_crtc, path_data->path[path_data->path_len - 1],
		next_attach_comp_id, cmdq_handle);

	/* 3. disconnect subpath and remove mutex */
	mtk_addon_path_disconnect_and_remove_mutex(crtc, ddp_mode, module_data,
						   cmdq_handle);

	/* 4. connect original path*/
	mtk_crtc_connect_path_between_component(crtc, ddp_mode, attach_comp_id,
						next_attach_comp_id,
						cmdq_handle);

	/* 5. config module*/
	/* config attach comp */
	comp = priv->ddp_comp[attach_comp_id];
	prev_comp_id = mtk_crtc_find_prev_comp(crtc, ddp_mode, attach_comp_id);
	next_comp_id = next_attach_comp_id;
	mtk_ddp_comp_addon_config(comp, prev_comp_id, next_comp_id,
				  addon_config, cmdq_handle);

	comp = priv->ddp_comp[next_attach_comp_id];
	prev_comp_id = attach_comp_id;
	next_comp_id =
		mtk_crtc_find_next_comp(crtc, ddp_mode, next_attach_comp_id);
	mtk_ddp_comp_addon_config(comp, prev_comp_id, next_comp_id,
				  addon_config, cmdq_handle);
}

void mtk_addon_disconnect_embed(
	struct drm_crtc *crtc, unsigned int ddp_mode,
	const struct mtk_addon_module_data *module_data,
	union mtk_addon_config *addon_config, struct cmdq_pkt *cmdq_handle)
{
	struct mtk_drm_private *priv = crtc->dev->dev_private;
	struct mtk_ddp_comp *comp = NULL;
	enum mtk_ddp_comp_id attach_comp_id, prev_comp_id, next_comp_id;
	const struct mtk_addon_path_data *path_data;

	if (!module_data ||
	    module_data->attach_comp < 0 ||
	    module_data->attach_comp >= DDP_COMPONENT_ID_MAX) {
		DDPMSG("invalid module_data\n");
		return;
	}

	path_data = mtk_addon_module_get_path(module_data->module);

	attach_comp_id =
		mtk_crtc_find_comp(crtc, ddp_mode, module_data->attach_comp);
	if (attach_comp_id == -1 || attach_comp_id >= DDP_COMPONENT_ID_MAX) {
		comp = priv->ddp_comp[module_data->attach_comp];
		DDPPR_ERR("Attach module:%s is not in path mode %d\n",
			  mtk_dump_comp_str(comp), ddp_mode);
		return;
	}

	/* 1. stop addon module*/
	mtk_addon_path_stop(crtc, path_data, cmdq_handle);

	/* 2. disconnect subpath and remove mutex */
	mtk_addon_path_disconnect_and_remove_mutex(crtc, ddp_mode, module_data,
						   cmdq_handle);

	/* 5. config module*/
	comp = priv->ddp_comp[attach_comp_id];
	prev_comp_id = mtk_crtc_find_prev_comp(crtc, ddp_mode, attach_comp_id);
	next_comp_id = -1;
	mtk_ddp_comp_addon_config(comp, prev_comp_id, next_comp_id,
				  addon_config, cmdq_handle);
}

void mtk_addon_connect_before(struct drm_crtc *crtc, unsigned int ddp_mode,
			      const struct mtk_addon_module_data *module_data,
			      union mtk_addon_config *addon_config,
			      struct cmdq_pkt *cmdq_handle)

{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_drm_private *priv = crtc->dev->dev_private;
	struct mtk_ddp_comp *comp = NULL;
	enum mtk_ddp_comp_id next_attach_comp_id, prev_comp_id, cur_comp_id,
		next_comp_id;
	const struct mtk_addon_path_data *path_data =
		mtk_addon_module_get_path(module_data->module);
	int i, j;
	unsigned int addon_idx;

	next_attach_comp_id =
		mtk_crtc_find_comp(crtc, ddp_mode, module_data->attach_comp);

	if (module_data->attach_comp < 0) {
		DDPPR_ERR("Invalid attach_comp value\n");
		return;
	}

	if (next_attach_comp_id == -1 || next_attach_comp_id >= DDP_COMPONENT_ID_MAX) {
		comp = priv->ddp_comp[module_data->attach_comp];
		DDPPR_ERR("Attach module:%s is not in path mode %d\n",
			  mtk_dump_comp_str(comp), ddp_mode);
		return;
	}

	/* 1. connect subpath and add mutex*/
	mtk_addon_path_connect_and_add_mutex(crtc, ddp_mode, module_data,
					     cmdq_handle);

	/* 2. add subpath to main path */
	mtk_ddp_add_comp_to_path_with_cmdq(
		mtk_crtc, path_data->path[path_data->path_len - 1],
		next_attach_comp_id, cmdq_handle);

	/* 3. config module */
	/* config subpath comp without last comp*/
	addon_idx = 0;
	cur_comp_id = -1;
	for (i = 0; i < path_data->path_len; i++) {
		if (mtk_ddp_comp_get_type(path_data->path[i]) ==
		    MTK_DISP_VIRTUAL)
			continue;

		addon_idx = i;
		comp = priv->ddp_comp[path_data->path[i]];
		prev_comp_id = cur_comp_id;
		cur_comp_id = comp->id;

		for (j = i + 1; j < path_data->path_len; j++)
			if (mtk_ddp_comp_get_type(path_data->path[j]) !=
			    MTK_DISP_VIRTUAL) {
				next_comp_id = path_data->path[j];

				mtk_ddp_comp_addon_config(
					comp, prev_comp_id, next_comp_id,
					addon_config, cmdq_handle);
				break;
			}
	}
	/* config subpath last comp */
	comp = priv->ddp_comp[path_data->path[addon_idx]];
	prev_comp_id = cur_comp_id;
	cur_comp_id = comp->id;
	next_comp_id = next_attach_comp_id;
	mtk_ddp_comp_addon_config(comp, prev_comp_id, next_comp_id,
				  addon_config, cmdq_handle);
	/* config attach comp */
	comp = priv->ddp_comp[next_attach_comp_id];
	prev_comp_id = cur_comp_id;
	cur_comp_id = comp->id;
	next_comp_id =
		mtk_crtc_find_next_comp(crtc, ddp_mode, next_attach_comp_id);
	mtk_ddp_comp_addon_config(comp, prev_comp_id, next_comp_id,
				  addon_config, cmdq_handle);

	/* 4. start addon module */
	mtk_addon_path_start(crtc, path_data, cmdq_handle);
}

void mtk_addon_disconnect_before(
	struct drm_crtc *crtc, unsigned int ddp_mode,
	const struct mtk_addon_module_data *module_data,
	union mtk_addon_config *addon_config, struct cmdq_pkt *cmdq_handle)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_drm_private *priv = crtc->dev->dev_private;
	struct mtk_ddp_comp *comp = NULL;
	enum mtk_ddp_comp_id prev_comp_id, next_comp_id, next_attach_comp_id;
	const struct mtk_addon_path_data *path_data =
		mtk_addon_module_get_path(module_data->module);

	next_attach_comp_id =
		mtk_crtc_find_comp(crtc, ddp_mode, module_data->attach_comp);

	if (module_data->attach_comp < 0) {
		DDPPR_ERR("Invalid attach_comp value\n");
		return;
	}

	if (next_attach_comp_id == -1 || next_attach_comp_id >= DDP_COMPONENT_ID_MAX) {
		comp = priv->ddp_comp[module_data->attach_comp];
		DDPPR_ERR("Attach module:%s is not in path mode %d\n",
			  mtk_dump_comp_str(comp), ddp_mode);
		return;
	}

	/* 1. stop addon module*/
	mtk_addon_path_stop(crtc, path_data, cmdq_handle);

	/* 2. remove subpath from main path */
	mtk_ddp_remove_comp_from_path_with_cmdq(
		mtk_crtc, path_data->path[path_data->path_len - 1],
		next_attach_comp_id, cmdq_handle);

	/* 3. disconnect subpath and remove mutex */
	mtk_addon_path_disconnect_and_remove_mutex(crtc, ddp_mode, module_data,
						   cmdq_handle);
	/* 4. config module */
	/* config attach comp */
	comp = priv->ddp_comp[next_attach_comp_id];
	prev_comp_id =
		mtk_crtc_find_prev_comp(crtc, ddp_mode, next_attach_comp_id);
	next_comp_id =
		mtk_crtc_find_next_comp(crtc, ddp_mode, next_attach_comp_id);
	mtk_ddp_comp_addon_config(comp, prev_comp_id, next_comp_id,
				  addon_config, cmdq_handle);
}

void mtk_addon_connect_after(struct drm_crtc *crtc, unsigned int ddp_mode,
			      const struct mtk_addon_module_data *module_data,
			      union mtk_addon_config *addon_config,
			      struct cmdq_pkt *cmdq_handle)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_drm_private *priv = crtc->dev->dev_private;
	struct mtk_ddp_comp *comp = NULL;
	enum mtk_ddp_comp_id prev_attach_comp_id, prev_comp_id, cur_comp_id,
		next_comp_id;
	const struct mtk_addon_path_data *path_data =
		mtk_addon_module_get_path(module_data->module);
	int i, j;
	unsigned int addon_idx;

	prev_attach_comp_id =
		mtk_crtc_find_comp(crtc, ddp_mode, module_data->attach_comp);
	if (prev_attach_comp_id == -1 || prev_attach_comp_id >= DDP_COMPONENT_ID_MAX) {
		comp = priv->ddp_comp[module_data->attach_comp];
		DDPMSG("[ERR]Attach module:%s is not in path mode %d\n",
			  mtk_dump_comp_str(comp), ddp_mode);
		return;
	}

	/* 0. attach subpath to crtc*/
	/* some comp need crtc info in addon_config or irq */
	mtk_crtc_attach_addon_path_comp(crtc, module_data, true);

	/* 1. connect subpath and add mutex*/
	mtk_addon_path_connect_and_add_mutex(crtc, ddp_mode, module_data,
					     cmdq_handle);

	/* 2. add subpath to main path */
	mtk_ddp_add_comp_to_path_with_cmdq(
		mtk_crtc, prev_attach_comp_id, path_data->path[0],
		cmdq_handle);

	/* 3. config module */
	/* config attach comp */
	comp = priv->ddp_comp[prev_attach_comp_id];
	prev_comp_id = mtk_crtc_find_prev_comp(crtc, ddp_mode,
				prev_attach_comp_id);
	cur_comp_id = comp->id;
	next_comp_id = -1;
	for (i = 0; i < path_data->path_len; i++)
		if (mtk_ddp_comp_get_type(path_data->path[i]) !=
		    MTK_DISP_VIRTUAL) {
			next_comp_id = path_data->path[i];
			break;
		}
	mtk_ddp_comp_addon_config(comp, prev_comp_id, next_comp_id,
				  addon_config, cmdq_handle);
	/* config subpath comp without last comp*/
	addon_idx = i;
	for (; i < path_data->path_len; i++) {
		if (mtk_ddp_comp_get_type(path_data->path[i]) ==
		    MTK_DISP_VIRTUAL)
			continue;

		addon_idx = i;
		comp = priv->ddp_comp[path_data->path[i]];

		for (j = i + 1; j < path_data->path_len; j++)
			if (mtk_ddp_comp_get_type(path_data->path[j]) !=
			    MTK_DISP_VIRTUAL) {
				prev_comp_id = cur_comp_id;
				cur_comp_id = comp->id;
				next_comp_id = path_data->path[j];
				mtk_ddp_comp_addon_config(
					comp, prev_comp_id, next_comp_id,
					addon_config, cmdq_handle);
				break;
			}
	}
	/* config subpath last comp */
	comp = priv->ddp_comp[path_data->path[addon_idx]];
	prev_comp_id = cur_comp_id;
	cur_comp_id = comp->id;
	next_comp_id = -1;
	mtk_ddp_comp_addon_config(comp, prev_comp_id, next_comp_id,
				  addon_config, cmdq_handle);

	/* 4. start addon module */
	mtk_addon_path_start(crtc, path_data, cmdq_handle);
}

void mtk_addon_disconnect_after(
	struct drm_crtc *crtc, unsigned int ddp_mode,
	const struct mtk_addon_module_data *module_data,
	union mtk_addon_config *addon_config, struct cmdq_pkt *cmdq_handle)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_drm_private *priv = crtc->dev->dev_private;
	struct mtk_ddp_comp *comp = NULL;
	enum mtk_ddp_comp_id prev_attach_comp_id, prev_comp_id, next_comp_id;
	const struct mtk_addon_path_data *path_data =
		mtk_addon_module_get_path(module_data->module);

	prev_attach_comp_id =
		mtk_crtc_find_comp(crtc, ddp_mode, module_data->attach_comp);
	if (prev_attach_comp_id == -1 ||  prev_attach_comp_id >= DDP_COMPONENT_ID_MAX) {
		comp = priv->ddp_comp[module_data->attach_comp];
		DDPMSG("[ERR]Attach module:%s is not in path mode %d\n",
			  mtk_dump_comp_str(comp), ddp_mode);
		return;
	}

	/* 0. attach subpath to crtc*/
	/* some comp need crtc info in stop*/
	mtk_crtc_attach_addon_path_comp(crtc, module_data, true);

	/* 1. stop addon module*/
	mtk_addon_path_stop(crtc, path_data, cmdq_handle);

	/* 2. remove subpath from main path */
	mtk_ddp_remove_comp_from_path_with_cmdq(
		mtk_crtc, prev_attach_comp_id, path_data->path[0],
		cmdq_handle);

	/* 3. disconnect subpath and remove mutex */
	mtk_addon_path_disconnect_and_remove_mutex(crtc, ddp_mode, module_data,
						   cmdq_handle);

	/* 4. config module */
	/* config attach comp */
	comp = priv->ddp_comp[prev_attach_comp_id];
	prev_comp_id =
		mtk_crtc_find_prev_comp(crtc, ddp_mode, prev_attach_comp_id);
	next_comp_id =
		mtk_crtc_find_next_comp(crtc, ddp_mode, prev_attach_comp_id);
	mtk_ddp_comp_addon_config(comp, prev_comp_id, next_comp_id,
				  addon_config, cmdq_handle);
}
