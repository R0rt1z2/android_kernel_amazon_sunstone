// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#define PFX "CAM_CAL"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__

#include <linux/kernel.h>
#include "cam_cal_list.h"
#include "eeprom_i2c_common_driver.h"
#include "eeprom_i2c_custom_driver.h"
#include "cam_cal_config.h"
#include "kd_imgsensor.h"

#define pr_debug_if(cond, ...)      do { if ((cond)) pr_debug(__VA_ARGS__); } while (0)
#define pr_debug_err(...)    pr_debug("error: " __VA_ARGS__)

static struct STRUCT_CALIBRATION_LAYOUT_STRUCT cal_layout_table = {
	0x00000001, 0x01015858, CAM_CAL_SINGLE_EEPROM_DATA,
	{
		{0x00000000, 0x00000000, 0x00000000, do_module_version},
		{0x00000000, 0x00000001, 0x00000011, do_part_number},
		{0x00000001, 0x00000014, 0x0000074C, do_single_lsc_custom},
		{0x00000001, 0x00000761, 0x00000012, do_2a_gain_rear},
		{0x00000000, 0x00000763, 0x00000800, do_pdaf},
		{0x00000000, 0x00000FAE, 0x00000550, do_stereo_data},
		{0x00000000, 0x00000000, 0x00000786, do_dump_all},
		{0x00000000, 0x00000F80, 0x0000000A, do_lens_id}
	}
};

struct STRUCT_CAM_CAL_CONFIG_STRUCT sc800_rear_eeprom = {
	.name = "sc800_rear_eeprom",
	.check_layout_function = layout_check,
	.read_function = Common_read_region,
	.layout = &cal_layout_table,
	.sensor_id = SC800_REAR_SUNTXD_SENSOR_ID,
	.i2c_write_id = 0xB0,
	.max_size = 0x2000,
	.enable_preload = 1,
	.preload_size = 0x1500,
};
