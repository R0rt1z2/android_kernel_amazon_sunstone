// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

//#define DEBUG

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>

#include "kd_imgsensor_define_v4l2.h"
#include "adaptor.h"
#include "adaptor-hw.h"

#define INST_OPS(__ctx, __field, __idx, __hw_id, __set, __unset) do {\
	if (__ctx->__field[__idx]) { \
		__ctx->hw_ops[__hw_id].set = __set; \
		__ctx->hw_ops[__hw_id].unset = __unset; \
		__ctx->hw_ops[__hw_id].data = (void *)__idx; \
	} \
} while (0)

static const char * const clk_names[] = {
	ADAPTOR_CLK_NAMES
};

static const char * const reg_names[] = {
	ADAPTOR_REGULATOR_NAMES
};

static const char * const state_names[] = {
	ADAPTOR_STATE_NAMES
};

#if MTK_CAM_DUAL_PIPE_DPTZ_SUPPORT
int first_sensor_power_flag;
int second_sensor_power_flag;
#endif

static struct clk *get_clk_by_freq(struct adaptor_ctx *ctx, int freq)
{
	dev_dbg(ctx->dev, "[%s] set camtg freq: %d MHz\n",
				__func__, freq);
	switch (freq) {
	case 6:
		return ctx->clk[CLK_6M];
	case 12:
		return ctx->clk[CLK_12M];
	case 13:
		return ctx->clk[CLK_13M];
	case 24:
		return ctx->clk[CLK_24M];
	case 26:
		return ctx->clk[CLK_26M];
	case 48:
		return ctx->clk[CLK_48M];
	case 52:
		return ctx->clk[CLK_52M];
	}

	return NULL;
}

static int set_mclk(struct adaptor_ctx *ctx, void *data, int val)
{
	int ret;
	struct clk *mclk, *mclk_src;

	mclk = ctx->clk[CLK_MCLK];
	mclk_src = get_clk_by_freq(ctx, val);

	ret = clk_prepare_enable(mclk);
	if (ret) {
		dev_err(ctx->dev, "failed to enable mclk\n");
		return ret;
	}

	ret = clk_prepare_enable(mclk_src);
	if (ret) {
		dev_err(ctx->dev, "failed to enable mclk_src\n");
		return ret;
	}

	ret = clk_set_parent(mclk, mclk_src);
	if (ret) {
		dev_err(ctx->dev, "failed to set mclk's parent\n");
		return ret;
	}

	return 0;
}

static int unset_mclk(struct adaptor_ctx *ctx, void *data, int val)
{
	struct clk *mclk, *mclk_src;

	mclk = ctx->clk[CLK_MCLK];
	mclk_src = get_clk_by_freq(ctx, val);

	clk_disable_unprepare(mclk_src);
	clk_disable_unprepare(mclk);

	return 0;
}

static int set_reg(struct adaptor_ctx *ctx, void *data, int val)
{
	unsigned long long ret, idx;
	struct regulator *reg;

	idx = (unsigned long long)data;
	reg = ctx->regulator[idx];

	ret = regulator_set_voltage(reg, val, val);
	if (ret) {
		dev_dbg(ctx->dev, "failed to set voltage %s %d\n",
				reg_names[idx], val);
	}

	ret = regulator_enable(reg);
	if (ret) {
		dev_dbg(ctx->dev, "failed to enable %s\n",
				reg_names[idx]);
		return ret;
	}

	return 0;
}

static int unset_reg(struct adaptor_ctx *ctx, void *data, int val)
{
	unsigned long long ret, idx;
	struct regulator *reg;

	idx = (unsigned long long)data;
	reg = ctx->regulator[idx];

	ret = regulator_disable(reg);
	if (ret) {
		dev_dbg(ctx->dev, "failed to disable %s\n",
				reg_names[idx]);
		return ret;
	}

	return 0;
}

static int set_state(struct adaptor_ctx *ctx, void *data, int val)
{
	unsigned long long idx, x;
	int ret;

	idx = (unsigned long long)data;
	x = idx + val;

	ret = pinctrl_select_state(ctx->pinctrl, ctx->state[x]);
	if (ret < 0) {
		dev_err(ctx->dev, "fail to select %s\n", state_names[x]);
		return ret;
	}

	return 0;
}

static int unset_state(struct adaptor_ctx *ctx, void *data, int val)
{
	return set_state(ctx, data, 0);
}

static int set_state_div2(struct adaptor_ctx *ctx, void *data, int val)
{
	return set_state(ctx, data, val >> 1);
}

static int set_state_boolean(struct adaptor_ctx *ctx, void *data, int val)
{
	return set_state(ctx, data, !!val);
}

static int set_state_mipi_switch(struct adaptor_ctx *ctx, void *data, int val)
{
	return set_state(ctx, (void *)STATE_MIPI_SWITCH_ON, 0);
}

static int unset_state_mipi_switch(struct adaptor_ctx *ctx, void *data,
	int val)
{
	return set_state(ctx, (void *)STATE_MIPI_SWITCH_OFF, 0);
}

static int reinit_pinctrl(struct adaptor_ctx *ctx)
{
	int i;
	struct device *dev = ctx->dev;

	/* pinctrl */
	ctx->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(ctx->pinctrl)) {
		dev_err(dev, "fail to get pinctrl\n");
		return PTR_ERR(ctx->pinctrl);
	}

	/* pinctrl states */
	for (i = 0; i < STATE_MAXCNT; i++) {
		ctx->state[i] = pinctrl_lookup_state(
				ctx->pinctrl, state_names[i]);
		if (IS_ERR(ctx->state[i])) {
			ctx->state[i] = NULL;
			dev_dbg(dev, "no state %s\n", state_names[i]);
		}
	}

	return 0;
}

int adaptor_hw_power_on(struct adaptor_ctx *ctx)
{
	int i;
	const struct subdrv_pw_seq_entry *ent;
	struct adaptor_hw_ops *op;

#ifndef IMGSENSOR_USE_PM_FRAMEWORK

	dev_dbg(ctx->dev, "%s power ref cnt = %d\n", __func__, ctx->power_refcnt);

	if (current->tgid != ctx->camerahalserver_pid) {
		ctx->camerahalserver_pid = current->tgid;
		ctx->power_refcnt = 0;
	}

	ctx->power_refcnt++;
	if (ctx->power_refcnt > 1) {
		dev_dbg(ctx->dev, "%s already powered, cnt = %d\n", __func__, ctx->power_refcnt);
		return 0;
	}
#endif

	/* may be released for mipi switch */
	if (!ctx->pinctrl) {
		dev_info(ctx->dev, "%s, ----call reinit_pinctrl!\n", __func__);
		reinit_pinctrl(ctx);
	}

#if MTK_CAM_DUAL_PIPE_DPTZ_SUPPORT
	if (ctx->sensor_id == DUALSENID_1ST && first_sensor_power_flag == 0)
		first_sensor_power_flag = 1;

	if (ctx->sensor_id == DUALSENID_2ND && second_sensor_power_flag == 0)
		second_sensor_power_flag = 1;
#endif

	op = &ctx->hw_ops[HW_ID_MIPI_SWITCH];
	if (op->set)
		op->set(ctx, op->data, 0);

	for (i = 0; i < ctx->subdrv->pw_seq_cnt; i++) {
		ent = &ctx->subdrv->pw_seq[i];
		op = &ctx->hw_ops[ent->id];
		if (!op->set) {
			dev_dbg(ctx->dev, "cannot set comp %d val %d\n",
				ent->id, ent->val);
			continue;
		}
		op->set(ctx, op->data, ent->val);
		if (ent->delay)
			mdelay(ent->delay);
	}

	dev_dbg(ctx->dev, "%s\n", __func__);

	return 0;
}

int adaptor_hw_power_off(struct adaptor_ctx *ctx)
{
	int i;
	const struct subdrv_pw_seq_entry *ent;
	struct adaptor_hw_ops *op;

#ifndef IMGSENSOR_USE_PM_FRAMEWORK

	if (!ctx->power_refcnt) {
		dev_dbg(ctx->dev, "%s power ref cnt = %d, skip due to not power on yet\n",
			__func__, ctx->power_refcnt);
		return 0;
	}
	dev_dbg(ctx->dev, "%s power ref cnt = %d\n", __func__, ctx->power_refcnt);
	ctx->power_refcnt--;
	if (ctx->power_refcnt > 0) {
		dev_dbg(ctx->dev, "%s skip due to cnt = %d\n", __func__, ctx->power_refcnt);
		return 0;
	}
	ctx->power_refcnt = 0;
	ctx->is_sensor_inited = 0;
	ctx->is_sensor_scenario_inited = 0;
#endif

#if MTK_CAM_DUAL_PIPE_DPTZ_SUPPORT
	if (ctx->sensor_id == DUALSENID_1ST && second_sensor_power_flag == 1) {
		first_sensor_power_flag = 0;
		dev_info(ctx->dev, "%s, ---- call devm_pinctrl_put!\n", __func__);
		devm_pinctrl_put(ctx->pinctrl);
		ctx->pinctrl = NULL;
		return 0;
	}
	if (ctx->sensor_id == DUALSENID_2ND && first_sensor_power_flag == 1) {
		second_sensor_power_flag = 0;
		dev_info(ctx->dev, "%s, ---- call devm_pinctrl_put!\n", __func__);
		devm_pinctrl_put(ctx->pinctrl);
		ctx->pinctrl = NULL;
		return 0;
	}
#endif

	for (i = ctx->subdrv->pw_seq_cnt - 1; i >= 0; i--) {
		ent = &ctx->subdrv->pw_seq[i];
		op = &ctx->hw_ops[ent->id];
		if (!op->unset)
			continue;
		op->unset(ctx, op->data, ent->val);
		//msleep(ent->delay);
	}

	op = &ctx->hw_ops[HW_ID_MIPI_SWITCH];
	if (op->unset)
		op->unset(ctx, op->data, 0);

	/* the pins of mipi switch are shared. free it for another users */
#if MTK_CAM_DUAL_PIPE_DPTZ_SUPPORT
	if (1) {
		dev_info(ctx->dev, "%s, ---- call devm_pinctrl_put!\n", __func__);
		devm_pinctrl_put(ctx->pinctrl);
		ctx->pinctrl = NULL;
	}
#else
	if (ctx->state[STATE_MIPI_SWITCH_ON] ||
		ctx->state[STATE_MIPI_SWITCH_OFF]) {
		devm_pinctrl_put(ctx->pinctrl);
		ctx->pinctrl = NULL;
	}
#endif
	dev_dbg(ctx->dev, "%s\n", __func__);

	return 0;
}

int adaptor_hw_init(struct adaptor_ctx *ctx)
{
	int i;
	struct device *dev = ctx->dev;

	ctx->camerahalserver_pid = current->tgid;

	/* clocks */
	for (i = 0; i < CLK_MAXCNT; i++) {
		ctx->clk[i] = devm_clk_get(dev, clk_names[i]);
		if (IS_ERR(ctx->clk[i])) {
			ctx->clk[i] = NULL;
			dev_dbg(dev, "no clk %s\n", clk_names[i]);
		}
	}

	/* supplies */
	for (i = 0; i < REGULATOR_MAXCNT; i++) {
		ctx->regulator[i] = devm_regulator_get_optional(
				dev, reg_names[i]);
		if (IS_ERR(ctx->regulator[i])) {
			ctx->regulator[i] = NULL;
			dev_dbg(dev, "no reg %s\n", reg_names[i]);
		}
	}

	/* pinctrl */
	ctx->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(ctx->pinctrl)) {
		dev_err(dev, "fail to get pinctrl\n");
		return PTR_ERR(ctx->pinctrl);
	}

	/* pinctrl states */
	for (i = 0; i < STATE_MAXCNT; i++) {
		ctx->state[i] = pinctrl_lookup_state(
				ctx->pinctrl, state_names[i]);
		if (IS_ERR(ctx->state[i])) {
			ctx->state[i] = NULL;
			dev_dbg(dev, "no state %s\n", state_names[i]);
		}
	}

	/* install operations */

	INST_OPS(ctx, clk, CLK_MCLK, HW_ID_MCLK, set_mclk, unset_mclk);

	INST_OPS(ctx, regulator, REGULATOR_AVDD, HW_ID_AVDD,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_DVDD, HW_ID_DVDD,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_DOVDD, HW_ID_DOVDD,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_AFVDD, HW_ID_AFVDD,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_AVDD1, HW_ID_AVDD1,
			set_reg, unset_reg);

	if (ctx->state[STATE_MIPI_SWITCH_ON])
		ctx->hw_ops[HW_ID_MIPI_SWITCH].set = set_state_mipi_switch;

	if (ctx->state[STATE_MIPI_SWITCH_OFF])
		ctx->hw_ops[HW_ID_MIPI_SWITCH].unset = unset_state_mipi_switch;

	INST_OPS(ctx, state, STATE_MCLK_OFF, HW_ID_MCLK_DRIVING_CURRENT,
			set_state_div2, unset_state);

	INST_OPS(ctx, state, STATE_RST_LOW, HW_ID_RST,
			set_state, unset_state);

	INST_OPS(ctx, state, STATE_PDN_LOW, HW_ID_PDN,
			set_state, unset_state);

	INST_OPS(ctx, state, STATE_AVDD_OFF, HW_ID_AVDD,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_DVDD_OFF, HW_ID_DVDD,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_DOVDD_OFF, HW_ID_DOVDD,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_AFVDD_OFF, HW_ID_AFVDD,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_AVDD1_OFF, HW_ID_AVDD1,
			set_state_boolean, unset_state);

	/* the pins of mipi switch are shared. free it for another users */
#if MTKCAM_VSDOF_SINGLE_CAMERA_SUPPORT
	dev_info(ctx->dev, "%s, ---- MTKCAM_VSDOF_SINGLE_CAMERA_SUPPORT!\n", __func__);
	if (1) {
		dev_info(ctx->dev, "%s, ---- call devm_pinctrl_put!\n", __func__);
		devm_pinctrl_put(ctx->pinctrl);
		ctx->pinctrl = NULL;
	}

#else
	if (ctx->state[STATE_MIPI_SWITCH_ON] ||
		ctx->state[STATE_MIPI_SWITCH_OFF]) {
		devm_pinctrl_put(ctx->pinctrl);
		ctx->pinctrl = NULL;
	}
#endif

#if MTK_CAM_DUAL_PIPE_DPTZ_SUPPORT
	first_sensor_power_flag = 0;
	second_sensor_power_flag = 0;
#endif

	return 0;
}

