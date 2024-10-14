// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#if IS_ENABLED(CONFIG_PM_SLEEP)
#include <linux/pm_wakeup.h>
#endif
#include <linux/uaccess.h>

#include "apu_top.h"
#include "apu_devfreq_cooling.h"
#include "aputop_rpmsg.h"
#include <apu_top_entry.h>

const struct apupwr_plat_data *pwr_data;

struct platform_device *this_pdev;
#if IS_ENABLED(CONFIG_DEBUG_FS)
static struct apupwr_dbg aputop_dbg;
#endif
static int aputop_func_sel;
static DEFINE_MUTEX(aputop_func_mtx);
#if IS_ENABLED(CONFIG_PM_SLEEP)
struct wakeup_source *ws;
#endif

int fpga_type = 2;
module_param (fpga_type, int, S_IRUGO);
MODULE_PARM_DESC (fpga_type,
"[1]ACX0_mvpu+ACX1_mvpu [2]ACX0_mvpu+ACX1_mdla0 [3]ACX0_mdla0+ACX1_mdla0");

static void apu_pwr_wake_lock(void)
{
#if IS_ENABLED(CONFIG_PM_SLEEP)
	__pm_stay_awake(ws);
#endif
}

static void apu_pwr_wake_unlock(void)
{
#if IS_ENABLED(CONFIG_PM_SLEEP)
	__pm_relax(ws);
#endif
}

static void apu_pwr_wake_init(void)
{
#if IS_ENABLED(CONFIG_PM_SLEEP)
	ws = wakeup_source_register(NULL, pwr_data->plat_name);
#endif
}

static void apu_pwr_wake_exit(void)
{
#if IS_ENABLED(CONFIG_PM_SLEEP)
	wakeup_source_unregister(ws);
#endif
}

static int device_linker(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *sup_np = NULL;
	struct device_node *con_np = NULL;
	struct device *sup_dev = NULL;
	struct device *con_dev = NULL;
	struct platform_device *sup_pdev = NULL;
	struct platform_device *con_pdev = NULL;
	int con_size = 0, idx = 0;

	con_size = of_count_phandle_with_args(dev->of_node, "consumer", NULL);

	if (con_size > 0) {

		sup_np = dev->of_node;

		for (idx = 0; idx < con_size; idx++) {
			con_np = of_parse_phandle(sup_np, "consumer", idx);
			if (con_np == NULL)
				return -EINVAL;

			con_pdev = of_find_device_by_node(con_np);
			con_dev = &con_pdev->dev;
			sup_pdev = of_find_device_by_node(sup_np);
			sup_dev = &sup_pdev->dev;

			if (!(sup_dev &&
				of_node_check_flag(sup_np, OF_POPULATED)
				&& con_dev &&
				of_node_check_flag(con_np, OF_POPULATED))) {

				return -EPROBE_DEFER;
			}

			get_device(sup_dev);
			get_device(con_dev);

			if (!device_link_add(con_dev, sup_dev,
				DL_FLAG_STATELESS | DL_FLAG_PM_RUNTIME)) {
				dev_info(dev, "Not linking %pOFP - %pOFP\n",
						con_np, sup_np);
				put_device(sup_dev);
				put_device(con_dev);

				return -EINVAL;
			}

			put_device(sup_dev);
			put_device(con_dev);

		}
	}

	return 0;
}

static int check_pwr_data(void)
{
	if (!pwr_data) {
		pr_info("%s error : has no platform data\n", __func__);
		return -ENODEV;
	}

	return 0;
}

static int aputop_pwr_on_rpm_cb(struct device *dev)
{
	int ret = 0;

	if (check_pwr_data())
		return -ENODEV;

	if (pwr_data->bypass_pwr_on == 1) {
		dev_info(dev, "%s bypass\n", __func__);
		return 0;
	}

	dev_dbg(dev, "%s %s\n", __func__, pwr_data->plat_name);
	apu_pwr_wake_lock();
	ret =  pwr_data->plat_aputop_on(dev);

	return ret;
}

static int aputop_pwr_off_rpm_cb(struct device *dev)
{
	int ret = 0;

	if (check_pwr_data())
		return -ENODEV;

	if (pwr_data->bypass_pwr_off == 1) {
		dev_info(dev, "%s bypass\n", __func__);
		return 0;
	}

	dev_dbg(dev, "%s %s\n", __func__, pwr_data->plat_name);
	ret = pwr_data->plat_aputop_off(dev);
	apu_pwr_wake_unlock();

	return ret;
}

static int apu_top_probe(struct platform_device *pdev)
{
	int ret = 0;

	dev_info(&pdev->dev, "%s\n", __func__);
	pwr_data = of_device_get_match_data(&pdev->dev);

	if (check_pwr_data())
		return -ENODEV;

	dev_info(&pdev->dev, "%s %s\n", __func__, pwr_data->plat_name);

	apu_pwr_wake_init();

	this_pdev = pdev;
	g_apupw_drv_ver = 3;

	pm_runtime_enable(&pdev->dev);

	ret = device_linker(pdev);
	if (ret)
		return ret;

	return pwr_data->plat_aputop_pb(pdev);
}

static int apu_top_remove(struct platform_device *pdev)
{
	if (check_pwr_data())
		return -ENODEV;

	dev_info(&pdev->dev, "%s %s\n", __func__, pwr_data->plat_name);
	pwr_data->plat_aputop_rm(pdev);
	pm_runtime_disable(&pdev->dev);
	apu_pwr_wake_exit();

	return 0;
}

static const struct of_device_id of_match_apu_top[] = {
	{ .compatible = "mt8188,apu_top_3", .data = &mt8188_plat_data},
	{ .compatible = "mt8195,apu_top_3", .data = &mt8195_plat_data},
	{ /* end of list */},
};

static const struct dev_pm_ops mtk_aputop_pm_ops = {
	SET_RUNTIME_PM_OPS(aputop_pwr_off_rpm_cb, aputop_pwr_on_rpm_cb, NULL)
};

static struct platform_driver apu_top_drv = {
	.probe = apu_top_probe,
	.remove = apu_top_remove,
	.driver = {
		.name = "apu_top_3",
		.pm = &mtk_aputop_pm_ops,
		.of_match_table = of_match_apu_top,
	},
};

static int set_aputop_func_param(const char *buf,
		const struct kernel_param *kp)
{
	struct aputop_func_param aputop;
	int ret = 0, arg_cnt = 0;

	mutex_lock(&aputop_func_mtx);

	if (check_pwr_data())
		return -ENODEV;

	memset(&aputop, 0, sizeof(struct aputop_func_param));

	arg_cnt = sscanf(buf, "%d %d %d %d %d",
				(int *)&aputop.func_id,
				&aputop.param1, &aputop.param2,
				&aputop.param3, &aputop.param4);

	if (arg_cnt > 5) {
		pr_notice("%s invalid input: %s, arg_cnt(%d)\n",
				__func__, buf, arg_cnt);
		return -EINVAL;
	}

	pr_info(
		"%s (func_id, param1, param2, param3, param4): (%d,%d,%d,%d,%d)\n",
		__func__, aputop.func_id,
		aputop.param1, aputop.param2,
		aputop.param3, aputop.param4);

	ret = pwr_data->plat_aputop_func(this_pdev, aputop.func_id, &aputop);

	mutex_unlock(&aputop_func_mtx);
	return ret;
}

static int get_aputop_func_param(char *buf, const struct kernel_param *kp)
{
	if (check_pwr_data())
		return -ENODEV;

	return snprintf(buf, 64, "aputop_func_sel:%d\n", aputop_func_sel);
}

static struct kernel_param_ops aputop_func_ops = {
	.set = set_aputop_func_param,
	.get = get_aputop_func_param,
};

__MODULE_PARM_TYPE(aputop_func_sel, "int");
module_param_cb(aputop_func_sel, &aputop_func_ops, &aputop_func_sel, 0644);
MODULE_PARM_DESC(aputop_func_sel, "trigger apu top func by parameter");

#if IS_ENABLED(CONFIG_DEBUG_FS)
static int aputop_dbg_open(struct inode *inode, struct file *file)
{
	pr_info("%s ++\n", __func__);
	if (pwr_data->plat_aputop_dbg_open)
		return pwr_data->plat_aputop_dbg_open(inode, file);
	else
		return 0;
}

static ssize_t aputop_dbg_write(struct file *flip, const char __user *buffer,
		size_t count, loff_t *f_pos)
{
	pr_info("%s ++\n", __func__);
	if (pwr_data->plat_aputop_dbg_write)
		return pwr_data->plat_aputop_dbg_write(
				flip, buffer, count, f_pos);
	else
		return 0;
}

static const struct file_operations aputop_dbg_fops = {
	.open = aputop_dbg_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = aputop_dbg_write,
};

int aputop_dbg_init(struct apusys_core_info *info)
{
        /* creating power file */
	aputop_dbg.file = debugfs_create_file("power", (0644),
			info->dbg_root, this_pdev, &aputop_dbg_fops);
	if (IS_ERR_OR_NULL(aputop_dbg.file)) {
		pr_info("failed to create \"power\" debug file.\n");
		return -1;
	}

	return 0;
}

void aputop_dbg_exit(void)
{
	debugfs_remove(aputop_dbg.file);
}
#endif

int apu_top_3_init(void)
{
	int ret = 0;

	pr_info("%s register platform driver...\n", __func__);
	ret = platform_driver_register(&apu_top_drv);
	if (ret) {
		pr_info("failed to register aputop platform driver\n");
		return -1;
	}

	pr_info("%s register rpmsg driver...\n", __func__);
	ret = aputop_register_rpmsg();
	if (ret) {
		pr_info("failed to register aputop rpmsg driver\n");
		goto register_rpmsg_fail;
	}

	ret = apu_devfreq_init();
	if (ret) {
		pr_info("failed to register apu devfreq driver\n");
		goto register_apu_devfreq_fail;
	}

	return ret;

register_apu_devfreq_fail:
	aputop_unregister_rpmsg();

register_rpmsg_fail:
	platform_driver_unregister(&apu_top_drv);

	return -1;
}

void apu_top_3_exit(void)
{
	apu_devfreq_exit();
	aputop_unregister_rpmsg();
	platform_driver_unregister(&apu_top_drv);
}
