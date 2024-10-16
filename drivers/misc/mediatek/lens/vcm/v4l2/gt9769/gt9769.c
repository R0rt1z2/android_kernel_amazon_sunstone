// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm_runtime.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#define DRIVER_NAME "gt9769"
#define GT9769_I2C_SLAVE_ADDR 0x18

#define LOG_INF(format, args...)                                               \
	pr_err(DRIVER_NAME " [%s] " format, __func__, ##args)

#define GT9769_NAME				"gt9769"
#define GT9769_MAX_FOCUS_POS			1023

#define GT9769_FOCUS_STEPS			1
#define GT9769_SET_POSITION_ADDR		0x03

#define GT9769_CMD_DELAY			0xff
#define GT9769_CTRL_DELAY_US			5000

#define GT9769_MOVE_STEPS			16
#define GT9769_MOVE_DELAY_US			8400
#define GT9769_STABLE_TIME_US			20000

struct gt9769_device {
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_subdev sd;
	struct v4l2_ctrl *focus;
	struct regulator *vin;
	struct regulator *vdd;
	struct pinctrl *vcamaf_pinctrl;
	struct pinctrl_state *vcamaf_on;
	struct pinctrl_state *vcamaf_off;
};

static inline struct gt9769_device *to_gt9769_vcm(struct v4l2_ctrl *ctrl)
{
	return container_of(ctrl->handler, struct gt9769_device, ctrls);
}

static inline struct gt9769_device *sd_to_gt9769_vcm(struct v4l2_subdev *subdev)
{
	return container_of(subdev, struct gt9769_device, sd);
}

struct regval_list {
	unsigned char reg_num;
	unsigned char value;
};


static int gt9769_set_position(struct gt9769_device *gt9769, u16 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&gt9769->sd);

	client->addr  = GT9769_I2C_SLAVE_ADDR >> 1;
	return i2c_smbus_write_word_data(client, GT9769_SET_POSITION_ADDR,
					 swab16(val));
}

static int gt9769_release(struct gt9769_device *gt9769)
{
	int ret, val;

	for (val = round_down(gt9769->focus->val, GT9769_MOVE_STEPS);
	     val >= 0; val -= GT9769_MOVE_STEPS) {
		ret = gt9769_set_position(gt9769, val);
		if (ret) {
			LOG_INF(" %s I2C failure: %d",
				__func__, ret);
			return ret;
		}
		usleep_range(GT9769_MOVE_DELAY_US,
			     GT9769_MOVE_DELAY_US + 2000);
	}

	/*
	 * Wait for the motor to stabilize after the last movement
	 * to prevent the motor from shaking.
	 */
	usleep_range(GT9769_STABLE_TIME_US - GT9769_MOVE_DELAY_US,
		     GT9769_STABLE_TIME_US - GT9769_MOVE_DELAY_US + 1000);

	return 0;
}

static int gt9769_init(struct gt9769_device *gt9769)
{
	struct i2c_client *client = v4l2_get_subdevdata(&gt9769->sd);
	int ret = 0;

	LOG_INF(" +\n");

	client->addr  = GT9769_I2C_SLAVE_ADDR >> 1;

	ret = i2c_smbus_write_byte_data(client, 0x02, 0x02);//change to advance mode

	ret = i2c_smbus_write_byte_data(client, 0x06, 0x60);//aac4 mode
	ret = i2c_smbus_write_byte_data(client, 0x07, 0x06);

	usleep_range(GT9769_CTRL_DELAY_US - 2000,
			     GT9769_CTRL_DELAY_US + 2000);//delay 3-5ms

	ret = i2c_smbus_write_byte_data(client, 0x03, 0x01);//init actuator pos
	ret = i2c_smbus_write_byte_data(client, 0x04, 0x5e);

	usleep_range(GT9769_MOVE_DELAY_US + 2000,
			     GT9769_MOVE_DELAY_US + 3000);//delay 10ms

	LOG_INF(" Check HW version: %x\n", ret);

	ret = i2c_smbus_read_byte_data(client, 0x00);

	LOG_INF(" -\n");

	return 0;
}

/* Power handling */
static int gt9769_power_off(struct gt9769_device *gt9769)
{
	int ret;

	LOG_INF(" %s\n", __func__);

	ret = gt9769_release(gt9769);
	if (ret)
		LOG_INF(" gt9769 release failed!\n");

	ret = regulator_disable(gt9769->vin);
	if (ret)
		return ret;

	ret = regulator_disable(gt9769->vdd);
	if (ret)
		return ret;

	if (gt9769->vcamaf_pinctrl && gt9769->vcamaf_off)
		ret = pinctrl_select_state(gt9769->vcamaf_pinctrl,
					gt9769->vcamaf_off);

	return ret;
}

static int gt9769_power_on(struct gt9769_device *gt9769)
{
	int ret;

	LOG_INF(" %s\n", __func__);

	ret = regulator_enable(gt9769->vin);
	if (ret < 0){
		LOG_INF("gt9769->vin ret<0 %s\n", __func__);
		return ret;
	}


	ret = regulator_enable(gt9769->vdd);
	if (ret < 0){
		LOG_INF("gt9769->vdd ret<0 %s\n", __func__);
		return ret;
	}

	if (gt9769->vcamaf_pinctrl && gt9769->vcamaf_on){
		ret = pinctrl_select_state(gt9769->vcamaf_pinctrl, gt9769->vcamaf_on);
		LOG_INF("ret<0 %s pinctrl_select_state\n", __func__);
		return ret;
	}

	if (ret < 0){
		LOG_INF("ret<0 %s vcamaf_pinctrl\n", __func__);
		return ret;
	}
	/*
	 * TODO(b/139784289): Confirm hardware requirements and adjust/remove
	 * the delay.
	 */
	usleep_range(GT9769_CTRL_DELAY_US, GT9769_CTRL_DELAY_US + 100);

	ret = gt9769_init(gt9769);
	if (ret < 0){
		LOG_INF("ret<0 gt9769_init %s\n", __func__);
		goto fail;
	}

	LOG_INF("ret=0 return %s\n", __func__);

	return 0;

fail:
	regulator_disable(gt9769->vin);
	regulator_disable(gt9769->vdd);
	if (gt9769->vcamaf_pinctrl && gt9769->vcamaf_off) {
		pinctrl_select_state(gt9769->vcamaf_pinctrl,
				gt9769->vcamaf_off);
				LOG_INF("pinctrl_select_state %s\n", __func__);
	}

	LOG_INF("pinctrl_select_state %s   ret=%d\n", __func__ ,ret);

	return ret;
}

static int gt9769_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct gt9769_device *gt9769 = to_gt9769_vcm(ctrl);
	int ret = 0;

	LOG_INF("gt9769_set_ctrl\n");

	if (ctrl->id == V4L2_CID_FOCUS_ABSOLUTE) {

	LOG_INF("pos: %d\n",ctrl->val );

	ret = gt9769_set_position(gt9769, ctrl->val);
	LOG_INF("%s gt9769_set_position: %d",  __func__, ret);
	return ret;
	}
	return 0;
}

static const struct v4l2_ctrl_ops gt9769_vcm_ctrl_ops = {
	.s_ctrl = gt9769_set_ctrl,
};

static int gt9769_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	int ret;

	LOG_INF("%s\n", __func__);

	ret = pm_runtime_get_sync(sd->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(sd->dev);
		return ret;
	}

	return 0;
}

static int gt9769_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	LOG_INF("%s\n", __func__);

	pm_runtime_put(sd->dev);

	return 0;
}

static const struct v4l2_subdev_internal_ops gt9769_int_ops = {
	.open = gt9769_open,
	.close = gt9769_close,
};

static const struct v4l2_subdev_ops gt9769_ops = { };

static void gt9769_subdev_cleanup(struct gt9769_device *gt9769)
{
	v4l2_async_unregister_subdev(&gt9769->sd);
	v4l2_ctrl_handler_free(&gt9769->ctrls);
#if IS_ENABLED(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&gt9769->sd.entity);
#endif
}

static int gt9769_init_controls(struct gt9769_device *gt9769)
{
	struct v4l2_ctrl_handler *hdl = &gt9769->ctrls;
	const struct v4l2_ctrl_ops *ops = &gt9769_vcm_ctrl_ops;

	v4l2_ctrl_handler_init(hdl, 1);

	gt9769->focus = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_FOCUS_ABSOLUTE,
			  0, GT9769_MAX_FOCUS_POS, GT9769_FOCUS_STEPS, 0);

	if (hdl->error)
		return hdl->error;

	gt9769->sd.ctrl_handler = hdl;

	return 0;
}

static int gt9769_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct gt9769_device *gt9769;
	int ret;

	LOG_INF("%s\n", __func__);

	gt9769 = devm_kzalloc(dev, sizeof(*gt9769), GFP_KERNEL);
	if (!gt9769)
		return -ENOMEM;

	gt9769->vin = devm_regulator_get(dev, "vin");
	if (IS_ERR(gt9769->vin)) {
		ret = PTR_ERR(gt9769->vin);
		if (ret != -EPROBE_DEFER)
			LOG_INF("cannot get vin regulator\n");
		return ret;
	}

	gt9769->vdd = devm_regulator_get(dev, "vdd");
	if (IS_ERR(gt9769->vdd)) {
		ret = PTR_ERR(gt9769->vdd);
		if (ret != -EPROBE_DEFER)
			LOG_INF("cannot get vdd regulator\n");
		return ret;
	}

	gt9769->vcamaf_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(gt9769->vcamaf_pinctrl)) {
		ret = PTR_ERR(gt9769->vcamaf_pinctrl);
		gt9769->vcamaf_pinctrl = NULL;
		LOG_INF("cannot get pinctrl\n");
	} else {
		gt9769->vcamaf_on = pinctrl_lookup_state(
			gt9769->vcamaf_pinctrl, "vcamaf_on");

		if (IS_ERR(gt9769->vcamaf_on)) {
			ret = PTR_ERR(gt9769->vcamaf_on);
			gt9769->vcamaf_on = NULL;
			LOG_INF("cannot get vcamaf_on pinctrl\n");
		}

		gt9769->vcamaf_off = pinctrl_lookup_state(
			gt9769->vcamaf_pinctrl, "vcamaf_off");

		if (IS_ERR(gt9769->vcamaf_off)) {
			ret = PTR_ERR(gt9769->vcamaf_off);
			gt9769->vcamaf_off = NULL;
			LOG_INF("cannot get vcamaf_off pinctrl\n");
		}
	}

	v4l2_i2c_subdev_init(&gt9769->sd, client, &gt9769_ops);
	gt9769->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	gt9769->sd.internal_ops = &gt9769_int_ops;

	ret = gt9769_init_controls(gt9769);
	if (ret)
		goto err_cleanup;

#if IS_ENABLED(CONFIG_MEDIA_CONTROLLER)
	ret = media_entity_pads_init(&gt9769->sd.entity, 0, NULL);
	if (ret < 0)
		goto err_cleanup;

	gt9769->sd.entity.function = MEDIA_ENT_F_LENS;
#endif

	ret = v4l2_async_register_subdev(&gt9769->sd);
	if (ret < 0)
		goto err_cleanup;

	pm_runtime_enable(dev);

	return 0;

err_cleanup:
	gt9769_subdev_cleanup(gt9769);
	return ret;
}

static int gt9769_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct gt9769_device *gt9769 = sd_to_gt9769_vcm(sd);

	LOG_INF("%s\n", __func__);

	gt9769_subdev_cleanup(gt9769);
	pm_runtime_disable(&client->dev);
	if (!pm_runtime_status_suspended(&client->dev))
		gt9769_power_off(gt9769);
	pm_runtime_set_suspended(&client->dev);

	return 0;
}

static int __maybe_unused gt9769_vcm_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct gt9769_device *gt9769 = sd_to_gt9769_vcm(sd);

	return gt9769_power_off(gt9769);
}

static int __maybe_unused gt9769_vcm_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct gt9769_device *gt9769 = sd_to_gt9769_vcm(sd);

	return gt9769_power_on(gt9769);
}

static const struct i2c_device_id gt9769_id_table[] = {
	{ GT9769_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, gt9769_id_table);

static const struct of_device_id gt9769_of_table[] = {
	{ .compatible = "mediatek,gt9769" },
	{ },
};
MODULE_DEVICE_TABLE(of, gt9769_of_table);

static const struct dev_pm_ops gt9769_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(gt9769_vcm_suspend, gt9769_vcm_resume, NULL)
};

static struct i2c_driver gt9769_i2c_driver = {
	.driver = {
		.name = GT9769_NAME,
		.pm = &gt9769_pm_ops,
		.of_match_table = gt9769_of_table,
	},
	.probe_new  = gt9769_probe,
	.remove = gt9769_remove,
	.id_table = gt9769_id_table,
};

module_i2c_driver(gt9769_i2c_driver);

MODULE_AUTHOR("Po-Hao Huang <Po-Hao.Huang@mediatek.com>");
MODULE_DESCRIPTION("GT9769 VCM driver");
MODULE_LICENSE("GPL v2");
