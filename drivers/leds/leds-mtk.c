// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021-2022 MediaTek Inc.
 *
 */

#include <linux/ctype.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <leds-mtk.h>
#include <linux/proc_fs.h>
#include <linux/amzn_sign_of_life_rtc.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <asm/setup.h>

#ifndef CONTROL_BL_TEMPERATURE
#define CONTROL_BL_TEMPERATURE
#endif

#ifdef CONTROL_BL_TEMPERATURE
#define MAX_BRIGHTNESS 255
static unsigned int thermal_limit = 255;
#endif

/****************************************************************************
 * variables
 ***************************************************************************/
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME " %s(%d) :" fmt, __func__, __LINE__

static int mtk_set_brightness(struct led_classdev *led_cdev,
					 enum led_brightness brightness);

struct mt_leds_desp_info {
	int lens;
	struct led_desp *leds[0];
};

static DEFINE_MUTEX(leds_mutex);
static BLOCKING_NOTIFIER_HEAD(mtk_leds_chain_head);

struct mt_leds_desp_info *leds_info;

int mtk_leds_register_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&mtk_leds_chain_head, nb);
}
EXPORT_SYMBOL_GPL(mtk_leds_register_notifier);

int mtk_leds_unregister_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&mtk_leds_chain_head, nb);
}
EXPORT_SYMBOL_GPL(mtk_leds_unregister_notifier);

int mt_leds_call_notifier(unsigned long action, void *data)
{
	return blocking_notifier_call_chain(&mtk_leds_chain_head, action, data);
}
EXPORT_SYMBOL_GPL(mt_leds_call_notifier);

static int  __maybe_unused call_notifier(int event, struct led_conf_info *led_conf)
{
	int err;

	err = mt_leds_call_notifier(event, led_conf);
	if (err)
		pr_info("notifier_call_chain error\n");
	return err;
}

/****************************************************************************
 * DEBUG MACROS
 ***************************************************************************/

static void led_debug_log(struct mt_led_data *s_led,
		int level, int mappingLevel)
{
	unsigned long cur_time_mod = 0;
	unsigned long long cur_time_display = 0;
	int ret = 0;

	s_led->debug.current_t = sched_clock();
	cur_time_display = s_led->debug.current_t;
	do_div(cur_time_display, 1000000);
	cur_time_mod = do_div(cur_time_display, 1000);

	ret = snprintf(s_led->debug.buffer + strlen(s_led->debug.buffer),
		4095 - strlen(s_led->debug.buffer),
		"T:%lld.%ld,L:%d L:%d map:%d    ",
		cur_time_display, cur_time_mod,
		s_led->conf.cdev.brightness, level, mappingLevel);

	s_led->debug.count++;

	if (ret < 0 || ret >= 4096) {
		pr_info("print log error!");
		s_led->debug.count = 5;
	}

	if (level == 0 || s_led->debug.count >= 5 ||
		(s_led->debug.current_t - s_led->debug.last_t) > 1000000000) {
		pr_info("%s", s_led->debug.buffer);
		s_led->debug.count = 0;
		s_led->debug.buffer[strlen("[Light] Set directly ") +
			strlen(s_led->conf.cdev.name)] = '\0';
	}

	s_led->debug.last_t = sched_clock();
}

static int get_desp_index(char *name)
{
	int i = 0;

	while (i < leds_info->lens) {
		if (!strcmp(name, leds_info->leds[i]->name))
			return i;
		i++;
	}
	return -1;
}

/****************************************************************************
 * driver functions
 ***************************************************************************/

static int brightness_maptolevel(struct led_conf_info *led_conf, int brightness)
{
	return (((led_conf->max_hw_brightness) * brightness +
	       ((led_conf->cdev.max_brightness) / 2)) /
	       (led_conf->cdev.max_brightness));
}

static int mtk_set_hw_brightness(struct mt_led_data *led_dat,
				int brightness)
{

	pr_debug("set hw brightness: %d -> %d", led_dat->hw_brightness, brightness);

	brightness = min(brightness, led_dat->conf.limit_hw_brightness);
	if (brightness == led_dat->hw_brightness)
		return 0;

	led_dat->mtk_hw_brightness_set(led_dat, brightness);
	led_dat->hw_brightness = brightness;

	return 0;
}

int mtk_leds_brightness_set(char *name, int level)
{
	struct mt_led_data *led_dat;
	struct led_conf_info *led_conf;
	int index;

	index = get_desp_index(name);
	if (index < 0) {
		pr_notice("can not find leds by led_desp %s", name);
		return -1;
	}
	led_dat = container_of(leds_info->leds[index],
		struct mt_led_data, desp);
	led_conf = &led_dat->conf;

	mutex_lock(&led_dat->led_access);
	if (!led_dat->conf.aal_enable) {
		led_dat->conf.aal_enable = 1;
		pr_notice("aal not enable, set %s %d return", name, level);
	}

	mtk_set_hw_brightness(led_dat, level);
	led_dat->last_hw_brightness = level;
	mutex_unlock(&led_dat->led_access);

	return 0;
}
EXPORT_SYMBOL(mtk_leds_brightness_set);

static int mtk_set_brightness(struct led_classdev *led_cdev,
					  enum led_brightness brightness)
{
	int trans_level = 0;

	struct led_conf_info *led_conf =
		container_of(led_cdev, struct led_conf_info, cdev);
	struct mt_led_data *led_dat =
		container_of(led_conf, struct mt_led_data, conf);

	if (led_dat->last_brightness == brightness)
		return 0;

	led_dat->last_brightness = brightness;

	trans_level = brightness_maptolevel(led_conf, brightness);

	led_debug_log(led_dat, brightness, trans_level);

	call_notifier(LED_BRIGHTNESS_CHANGED, led_conf);
	mutex_lock(&led_dat->led_access);
	if (!led_conf->aal_enable) {
		mtk_set_hw_brightness(led_dat, trans_level);
		led_dat->last_hw_brightness = trans_level;
	}
	mutex_unlock(&led_dat->led_access);

	return 0;

}

static enum led_brightness mtk_get_brightness(struct led_classdev *led_cdev)
{
	struct led_conf_info *led_conf =
		container_of(led_cdev, struct led_conf_info, cdev);
	struct mt_led_data *led_dat =
		container_of(led_conf, struct mt_led_data, conf);

	return led_dat->last_brightness;

}

/****************************************************************************
 * add API for temperature control
 ***************************************************************************/

int setMaxBrightness(char *name, int percent, bool enable)
{
	struct mt_led_data *led_dat;
	int max_l = 0, index = -1, limit_l = 0, cur_l = 0;

	index = get_desp_index(name);
	if (index < 0) {
		pr_notice("can not find leds by led_desp %s", name);
		return -1;
	}
	led_dat = container_of(leds_info->leds[index],
		struct mt_led_data, desp);

	max_l = led_dat->conf.max_hw_brightness;
	limit_l = (percent * max_l) / 100;
	pr_info("before: name: %s, percent : %d, limit_l : %d, enable: %d",
		leds_info->leds[index]->name, percent, limit_l, enable);
	if (enable) {
		led_dat->conf.limit_hw_brightness = limit_l;
		cur_l = min(led_dat->last_hw_brightness, limit_l);
	} else if (!enable) {
		led_dat->conf.limit_hw_brightness = max_l;
		cur_l = led_dat->last_hw_brightness;
	}
	thermal_limit = (cur_l * MAX_BRIGHTNESS) / max_l;
	if (led_dat->conf.cdev.brightness != 0)
		mtk_set_hw_brightness(led_dat, cur_l);

	pr_info("after: name: %s, cur_l : %d, max_brightness : %d",
		led_dat->conf.cdev.name, cur_l, led_dat->conf.limit_hw_brightness);
	return 0;

}
EXPORT_SYMBOL(setMaxBrightness);

static bool check_quiescent_mode(void) {
	char keyword[] = "androidboot.quiescent=1";

	if (strstr(saved_command_line, keyword)) {
		return true;
	}
	return false;
}

int set_max_brightness(int max_level, bool enable)
{
	int thousandth_ratio = (max_level * 100) / MAX_BRIGHTNESS;
	return setMaxBrightness("lcd-backlight", thousandth_ratio, enable);
}
EXPORT_SYMBOL(set_max_brightness);

#ifdef CONTROL_BL_TEMPERATURE
static ssize_t thermal_limit_brightness_show(struct device *dev,
					     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", thermal_limit);
}

static DEVICE_ATTR_RO(thermal_limit_brightness);
#endif

int mt_leds_parse_dt(struct mt_led_data *mdev, struct fwnode_handle *fwnode)
{
	int ret = 0, default_brightness = 0;
	const char *state;
	struct mt_leds_desp_info *nleds_info;

	ret = fwnode_property_read_string(fwnode, "label", &(mdev->conf.cdev.name));
	if (ret)
		return -EINVAL;

	ret = fwnode_property_read_string(fwnode, "default-trigger",
		&(mdev->conf.cdev.default_trigger));
	if (ret) {
		pr_info("Fail to read default-trigger property");
		mdev->conf.cdev.default_trigger = NULL;
	}

	ret = fwnode_property_read_u32(fwnode,
		"max-brightness", &(mdev->conf.cdev.max_brightness));
	if (ret) {
		pr_info("No max-brightness, use default value 255");
			mdev->conf.cdev.max_brightness = 255;
	}
	ret = fwnode_property_read_u32(fwnode,
		"max-hw-brightness", &(mdev->conf.max_hw_brightness));
	if (ret) {
		pr_info("No max-hw-brightness, use default value 1023");
		mdev->conf.max_hw_brightness = 1023;
	}
	mdev->conf.limit_hw_brightness = mdev->conf.max_hw_brightness;
	ret = fwnode_property_read_string(fwnode, "default-state", &state);
	if (!ret) {
		if (!strncmp(state, "half", strlen("half")))
			mdev->conf.cdev.brightness = mdev->conf.cdev.max_brightness / 2;
		else if (!strncmp(state, "on", strlen("on"))) {
			ret = fwnode_property_read_u32(fwnode,
				"default-state-brightness", &default_brightness);
			if (ret)
				mdev->conf.cdev.brightness = mdev->conf.cdev.max_brightness;
			else
				mdev->conf.cdev.brightness = default_brightness;
		}
		else
			mdev->conf.cdev.brightness = 0;
	} else {
		mdev->conf.cdev.brightness = mdev->conf.cdev.max_brightness * 40 / 100;
	}

	strlcpy(mdev->desp.name, mdev->conf.cdev.name,
		sizeof(mdev->desp.name));
	mdev->desp.index = leds_info->lens;

	nleds_info = krealloc(leds_info, sizeof(struct mt_leds_desp_info) +
		sizeof(struct led_desp *) * (leds_info->lens + 1),
		GFP_KERNEL);

	if (!nleds_info) {
		return -ENOMEM;
	}
	leds_info = nleds_info;
	leds_info->leds[leds_info->lens] = &mdev->desp;
	leds_info->lens++;
	mdev->conf.aal_enable = 0;
	mutex_init(&mdev->led_access);
	if (check_quiescent_mode()) {
		mdev->conf.cdev.brightness = 0;
	}

	pr_info("parse led: %s, num: %d, max: %d, max_hw: %d, brightness: %d",
		mdev->conf.cdev.name,
		leds_info->lens,
		mdev->conf.cdev.max_brightness,
		mdev->conf.max_hw_brightness,
		mdev->conf.cdev.brightness);

	return 0;
}
EXPORT_SYMBOL_GPL(mt_leds_parse_dt);

int mt_leds_classdev_register(struct device *parent,
				     struct mt_led_data *led_dat)
{
	int ret = 0;

	led_dat->conf.cdev.flags = LED_CORE_SUSPENDRESUME;
	led_dat->conf.cdev.brightness_set_blocking = mtk_set_brightness;
	led_dat->conf.cdev.brightness_get = mtk_get_brightness;

	ret = devm_led_classdev_register(parent, &(led_dat->conf.cdev));
	if (ret < 0) {
		pr_notice("led class register fail!");
		return ret;
	}
	pr_info("%s devm_led_classdev_register ok! ", led_dat->conf.cdev.name);

#ifdef CONTROL_BL_TEMPERATURE
	if (strcmp(led_dat->conf.cdev.name, "lcd-backlight") == 0) {
		ret = device_create_file(led_dat->conf.cdev.dev,
			&dev_attr_thermal_limit_brightness);
		if (ret)
			pr_notice("device_create_file thermal_limit_brightness fail\n");
	}
#endif

	ret = snprintf(led_dat->debug.buffer + strlen(led_dat->debug.buffer),
		4095 - strlen(led_dat->debug.buffer),
		"[Light] Set %s directly ", led_dat->conf.cdev.name);

	if (ret < 0 || ret >= 4096)
		pr_info("print log init error!");

	led_dat->last_brightness = led_dat->conf.cdev.brightness;

	led_dat->last_hw_brightness = brightness_maptolevel(&led_dat->conf,
							    led_dat->last_brightness);
	mtk_set_hw_brightness(led_dat,
		brightness_maptolevel(&led_dat->conf, led_dat->last_brightness));

	pr_info("%s devm_led_classdev_register end!", led_dat->conf.cdev.name);

	return ret;
}
EXPORT_SYMBOL_GPL(mt_leds_classdev_register);

void mt_leds_classdev_unregister(struct device *parent,
				     struct mt_led_data *led_dat)
{
	devm_led_classdev_unregister(parent, &(led_dat->conf.cdev));

	pr_info("%s devm_led_classdev_unregister ok! ", led_dat->conf.cdev.name);

}
EXPORT_SYMBOL_GPL(mt_leds_classdev_unregister);

static int __init mtk_leds_init(void)
{
	int ret = 0;

	pr_info("init begain +++");

	leds_info = kzalloc(sizeof(struct mt_leds_desp_info), GFP_KERNEL);

	if (!leds_info) {
		ret = -ENOMEM;
		goto err;
	}

	pr_info("init end ---");
	return 0;
 err:
	pr_notice("Failed to probe!\n");
	ret = -ENOMEM;
	return ret;

}

static void __exit mtk_leds_exit(void)
{
	kfree(leds_info);
	leds_info = NULL;
}

/* delay leds init, for (1)display has delayed to use clock upstream.
 * (2)to fix repeat switch battary and power supply caused BL KE issue,
 * battary calling bl .shutdown whitch need to call display
 * function and they not yet probe.
 */
module_init(mtk_leds_init);
module_exit(mtk_leds_exit);
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);

MODULE_AUTHOR("Mediatek Corporation");
MODULE_DESCRIPTION("MTK Disp Backlight Driver");
MODULE_LICENSE("GPL");


