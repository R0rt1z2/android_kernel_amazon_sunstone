/*
 * Copyright (C) 2022 Novatek, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/version.h>

#include "nvt_i2c_hid.h"
#include "nvt_i2c_hid_selftest.h"

#define NORMAL_MODE 0x00
#define TEST_MODE_2 0x22
#define MP_MODE_CC 0x41
#define FREQ_HOP_DISABLE 0x66

#define SHORT_TEST_CSV_FILE "/data/local/tmp/ShortTest.csv"
#define OPEN_TEST_CSV_FILE "/data/local/tmp/OpenTest.csv"
#define FW_RAWDATA_CSV_FILE "/data/local/tmp/FWRawdataTest.csv"
#define FW_CC_CSV_FILE "/data/local/tmp/FWCCTest.csv"
#define NOISE_TEST_CSV_FILE "/data/local/tmp/NoiseTest.csv"
#define PEN_FW_RAW_TEST_CSV_FILE "/data/local/tmp/PenFWRawTest.csv"
#define PEN_NOISE_TEST_CSV_FILE "/data/local/tmp/PenNoiseTest.csv"
#if IS_ENABLED(CONFIG_I2C_HID_OF_NVT_TS_SELFTEST)
#define TG_INX_TEST_CONFIG_CSV_PATH "/dfs/input_data/touch/"
#endif

#define nvt_mp_seq_printf(m, fmt, args...)                                     \
	do {                                                                   \
		seq_printf(m, fmt, ##args);                                    \
		if (!nvt_mp_test_result_printed)                               \
			printk(fmt, ##args);                                   \
	} while (0)

static uint8_t *RecordResult_Short = NULL;
static uint8_t *RecordResult_Open = NULL;
static uint8_t *RecordResult_FW_Rawdata = NULL;
static uint8_t *RecordResult_FW_CC = NULL;
static uint8_t *RecordResult_FW_DiffMax = NULL;
static uint8_t *RecordResult_FW_DiffMin = NULL;
static uint8_t *RecordResult_PenTipX_Raw = NULL;
static uint8_t *RecordResult_PenTipY_Raw = NULL;
static uint8_t *RecordResult_PenRingX_Raw = NULL;
static uint8_t *RecordResult_PenRingY_Raw = NULL;
static uint8_t *RecordResult_PenTipX_DiffMax = NULL;
static uint8_t *RecordResult_PenTipX_DiffMin = NULL;
static uint8_t *RecordResult_PenTipY_DiffMax = NULL;
static uint8_t *RecordResult_PenTipY_DiffMin = NULL;
static uint8_t *RecordResult_PenRingX_DiffMax = NULL;
static uint8_t *RecordResult_PenRingX_DiffMin = NULL;
static uint8_t *RecordResult_PenRingY_DiffMax = NULL;
static uint8_t *RecordResult_PenRingY_DiffMin = NULL;

static int32_t TestResult_Short = 0;
static int32_t TestResult_Open = 0;
static int32_t TestResult_FW_Rawdata = 0;
static int32_t TestResult_FW_CC = 0;
static int32_t TestResult_Noise = 0;
static int32_t TestResult_FW_DiffMax = 0;
static int32_t TestResult_FW_DiffMin = 0;
static int32_t TestResult_Pen_FW_Raw = 0;
static int32_t TestResult_PenTipX_Raw = 0;
static int32_t TestResult_PenTipY_Raw = 0;
static int32_t TestResult_PenRingX_Raw = 0;
static int32_t TestResult_PenRingY_Raw = 0;
static int32_t TestResult_Pen_Noise = 0;
static int32_t TestResult_PenTipX_DiffMax = 0;
static int32_t TestResult_PenTipX_DiffMin = 0;
static int32_t TestResult_PenTipY_DiffMax = 0;
static int32_t TestResult_PenTipY_DiffMin = 0;
static int32_t TestResult_PenRingX_DiffMax = 0;
static int32_t TestResult_PenRingX_DiffMin = 0;
static int32_t TestResult_PenRingY_DiffMax = 0;
static int32_t TestResult_PenRingY_DiffMin = 0;

static int32_t *RawData_Short = NULL;
static int32_t *RawData_Open = NULL;
static int32_t *RawData_Diff = NULL;
static int32_t *RawData_Diff_Min = NULL;
static int32_t *RawData_Diff_Max = NULL;
static int32_t *RawData_FW_Rawdata = NULL;
static int32_t *RawData_FW_CC = NULL;
static int32_t *RawData_PenTipX_Raw = NULL;
static int32_t *RawData_PenTipY_Raw = NULL;
static int32_t *RawData_PenRingX_Raw = NULL;
static int32_t *RawData_PenRingY_Raw = NULL;
static int32_t *RawData_PenTipX_DiffMin = NULL;
static int32_t *RawData_PenTipX_DiffMax = NULL;
static int32_t *RawData_PenTipY_DiffMin = NULL;
static int32_t *RawData_PenTipY_DiffMax = NULL;
static int32_t *RawData_PenRingX_DiffMin = NULL;
static int32_t *RawData_PenRingX_DiffMax = NULL;
static int32_t *RawData_PenRingY_DiffMin = NULL;
static int32_t *RawData_PenRingY_DiffMax = NULL;

#if IS_ENABLED(CONFIG_I2C_HID_OF_NVT_TS_SELFTEST)
struct selftest_touch_item {
	char name[50];
	int32_t *rawdata;
};

struct selftest_touch_item selftest_touch_items[] = {
	{"PS_Config_Lmt_Short_Rawdata_P:", PS_Config_Lmt_Short_Rawdata_P},
	{"PS_Config_Lmt_Short_Rawdata_N:", PS_Config_Lmt_Short_Rawdata_N},
	{"PS_Config_Lmt_Open_Rawdata_P:", PS_Config_Lmt_Open_Rawdata_P},
	{"PS_Config_Lmt_Open_Rawdata_N:", PS_Config_Lmt_Open_Rawdata_N},
	{"PS_Config_Lmt_FW_Rawdata_P:", PS_Config_Lmt_FW_Rawdata_P},
	{"PS_Config_Lmt_FW_Rawdata_N:", PS_Config_Lmt_FW_Rawdata_N},
	{"PS_Config_Lmt_FW_CC_P:", PS_Config_Lmt_FW_CC_P},
	{"PS_Config_Lmt_FW_CC_N:", PS_Config_Lmt_FW_CC_N},
	{"PS_Config_Lmt_FW_Diff_P:", PS_Config_Lmt_FW_Diff_P},
	{"PS_Config_Lmt_FW_Diff_N:", PS_Config_Lmt_FW_Diff_N},
};

uint8_t selftest_pen_x_num_x = 0;
uint8_t selftest_pen_x_num_y = 0;
uint8_t selftest_pen_y_num_x = 0;
uint8_t selftest_pen_y_num_y = 0;

struct selftest_pen_item {
	char name[50];
	int32_t *rawdata;
	uint8_t *width;
	uint8_t *height;
};

struct selftest_pen_item selftest_pen_items[] = {
	{"PS_Config_Lmt_PenTipX_FW_Raw_P:", PS_Config_Lmt_PenTipX_FW_Raw_P,
			&selftest_pen_x_num_x, &selftest_pen_x_num_y},
	{"PS_Config_Lmt_PenTipX_FW_Raw_N:", PS_Config_Lmt_PenTipX_FW_Raw_N,
			&selftest_pen_x_num_x, &selftest_pen_x_num_y},
	{"PS_Config_Lmt_PenTipY_FW_Raw_P:", PS_Config_Lmt_PenTipY_FW_Raw_P,
			&selftest_pen_y_num_x, &selftest_pen_y_num_y},
	{"PS_Config_Lmt_PenTipY_FW_Raw_N:", PS_Config_Lmt_PenTipY_FW_Raw_N,
			&selftest_pen_y_num_x, &selftest_pen_y_num_y},
	{"PS_Config_Lmt_PenRingX_FW_Raw_P:", PS_Config_Lmt_PenRingX_FW_Raw_P,
			&selftest_pen_x_num_x, &selftest_pen_x_num_y},
	{"PS_Config_Lmt_PenRingX_FW_Raw_N:", PS_Config_Lmt_PenRingX_FW_Raw_N,
			&selftest_pen_x_num_x, &selftest_pen_x_num_y},
	{"PS_Config_Lmt_PenRingY_FW_Raw_P:", PS_Config_Lmt_PenRingY_FW_Raw_P,
			&selftest_pen_y_num_x, &selftest_pen_y_num_y},
	{"PS_Config_Lmt_PenRingY_FW_Raw_N:", PS_Config_Lmt_PenRingY_FW_Raw_N,
			&selftest_pen_y_num_x, &selftest_pen_y_num_y},
	{"PS_Config_Lmt_PenTipX_FW_Diff_P:", PS_Config_Lmt_PenTipX_FW_Diff_P,
			&selftest_pen_x_num_x, &selftest_pen_x_num_y},
	{"PS_Config_Lmt_PenTipX_FW_Diff_N:", PS_Config_Lmt_PenTipX_FW_Diff_N,
			&selftest_pen_x_num_x, &selftest_pen_x_num_y},
	{"PS_Config_Lmt_PenTipY_FW_Diff_P:", PS_Config_Lmt_PenTipY_FW_Diff_P,
			&selftest_pen_y_num_x, &selftest_pen_y_num_y},
	{"PS_Config_Lmt_PenTipY_FW_Diff_N:", PS_Config_Lmt_PenTipY_FW_Diff_N,
			&selftest_pen_y_num_x, &selftest_pen_y_num_y},
	{"PS_Config_Lmt_PenRingX_FW_Diff_P:", PS_Config_Lmt_PenRingX_FW_Diff_P,
			&selftest_pen_x_num_x, &selftest_pen_x_num_y},
	{"PS_Config_Lmt_PenRingX_FW_Diff_N:", PS_Config_Lmt_PenRingX_FW_Diff_N,
			&selftest_pen_x_num_x, &selftest_pen_x_num_y},
	{"PS_Config_Lmt_PenRingY_FW_Diff_P:", PS_Config_Lmt_PenRingY_FW_Diff_P,
			&selftest_pen_y_num_x, &selftest_pen_y_num_y},
	{"PS_Config_Lmt_PenRingY_FW_Diff_N:", PS_Config_Lmt_PenRingY_FW_Diff_N,
			&selftest_pen_y_num_x, &selftest_pen_y_num_y},
};
#endif

static struct proc_dir_entry *NVT_proc_selftest_entry = NULL;
static int8_t nvt_mp_test_result_printed = 0;
static uint8_t fw_ver = 0;

extern void nvt_change_mode(uint8_t mode);
extern uint8_t nvt_get_fw_pipe(void);
extern void nvt_read_mdata(uint32_t xdata_addr);
extern void nvt_get_mdata(int32_t *buf, uint8_t *m_x_num, uint8_t *m_y_num);
extern void nvt_read_get_num_mdata(uint32_t xdata_addr, int32_t *buffer, uint32_t num);
int32_t nvt_mp_parse_dt(struct device_node *root, const char *node_compatible);

/*******************************************************
Description:
	Novatek touchscreen allocate buffer for mp selftest.

return:
	Executive outcomes. 0---succeed. -12---Out of memory
*******************************************************/
static int nvt_mp_buffer_init(void)
{
	size_t RecordResult_BufSize = X_Y_DIMENSION_MAX;
	size_t RawData_BufSize = X_Y_DIMENSION_MAX * sizeof(int32_t);
	size_t Pen_RecordResult_BufSize = PEN_X_Y_DIMENSION_MAX;
	size_t Pen_RawData_BufSize = PEN_X_Y_DIMENSION_MAX * sizeof(int32_t);

	RecordResult_Short = (uint8_t *)kzalloc(RecordResult_BufSize, GFP_KERNEL);
	if (!RecordResult_Short) {
		NVT_ERR("kzalloc for RecordResult_Short failed!\n");
		return -ENOMEM;
	}

	RecordResult_Open = (uint8_t *)kzalloc(RecordResult_BufSize, GFP_KERNEL);
	if (!RecordResult_Open) {
		NVT_ERR("kzalloc for RecordResult_Open failed!\n");
		return -ENOMEM;
	}

	RecordResult_FW_Rawdata = (uint8_t *)kzalloc(RecordResult_BufSize, GFP_KERNEL);
	if (!RecordResult_FW_Rawdata) {
		NVT_ERR("kzalloc for RecordResult_FW_Rawdata failed!\n");
		return -ENOMEM;
	}

	RecordResult_FW_CC = (uint8_t *)kzalloc(RecordResult_BufSize, GFP_KERNEL);
	if (!RecordResult_FW_CC) {
		NVT_ERR("kzalloc for RecordResult_FW_CC failed!\n");
		return -ENOMEM;
	}

	RecordResult_FW_DiffMax = (uint8_t *)kzalloc(RecordResult_BufSize, GFP_KERNEL);
	if (!RecordResult_FW_DiffMax) {
		NVT_ERR("kzalloc for RecordResult_FW_DiffMax failed!\n");
		return -ENOMEM;
	}

	RecordResult_FW_DiffMin = (uint8_t *)kzalloc(RecordResult_BufSize, GFP_KERNEL);
	if (!RecordResult_FW_DiffMin) {
		NVT_ERR("kzalloc for RecordResult_FW_DiffMin failed!\n");
		return -ENOMEM;
	}

	if (ts->pen_support) {
		RecordResult_PenTipX_Raw = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenTipX_Raw) {
			NVT_ERR("kzalloc for RecordResult_PenTipX_Raw failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenTipY_Raw = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenTipY_Raw) {
			NVT_ERR("kzalloc for RecordResult_PenTipY_Raw failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenRingX_Raw = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenRingX_Raw) {
			NVT_ERR("kzalloc for RecordResult_PenRingX_Raw failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenRingY_Raw = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenRingY_Raw) {
			NVT_ERR("kzalloc for RecordResult_PenRingY_Raw failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenTipX_DiffMax = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenTipX_DiffMax) {
			NVT_ERR("kzalloc for RecordResult_PenTipX_DiffMax failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenTipX_DiffMin = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenTipX_DiffMin) {
			NVT_ERR("kzalloc for RecordResult_PenTipX_DiffMin failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenTipY_DiffMax = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenTipY_DiffMax) {
			NVT_ERR("kzalloc for RecordResult_PenTipY_DiffMax failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenTipY_DiffMin = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenTipY_DiffMin) {
			NVT_ERR("kzalloc for RecordResult_PenTipY_DiffMin failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenRingX_DiffMax = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenRingX_DiffMax) {
			NVT_ERR("kzalloc for RecordResult_PenRingX_DiffMax failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenRingX_DiffMin = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenRingX_DiffMin) {
			NVT_ERR("kzalloc for RecordResult_PenRingX_DiffMin failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenRingY_DiffMax = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenRingY_DiffMax) {
			NVT_ERR("kzalloc for RecordResult_PenRingY_DiffMax failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenRingY_DiffMin = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenRingY_DiffMin) {
			NVT_ERR("kzalloc for RecordResult_PenRingY_DiffMin failed!\n");
			return -ENOMEM;
		}
	} /* if (ts->pen_support) */

	RawData_Short = (int32_t *)kzalloc(RawData_BufSize, GFP_KERNEL);
	if (!RawData_Short) {
		NVT_ERR("kzalloc for RawData_Short failed!\n");
		return -ENOMEM;
	}

	RawData_Open = (int32_t *)kzalloc(RawData_BufSize, GFP_KERNEL);
	if (!RawData_Open) {
		NVT_ERR("kzalloc for RawData_Open failed!\n");
		return -ENOMEM;
	}

	RawData_Diff = (int32_t *)kzalloc(RawData_BufSize, GFP_KERNEL);
	if (!RawData_Diff) {
		NVT_ERR("kzalloc for RawData_Diff failed!\n");
		return -ENOMEM;
	}

	RawData_Diff_Min = (int32_t *)kzalloc(RawData_BufSize, GFP_KERNEL);
	if (!RawData_Diff_Min) {
		NVT_ERR("kzalloc for RawData_Diff_Min failed!\n");
		return -ENOMEM;
	}

	RawData_Diff_Max = (int32_t *)kzalloc(RawData_BufSize, GFP_KERNEL);
	if (!RawData_Diff_Max) {
		NVT_ERR("kzalloc for RawData_Diff_Max failed!\n");
		return -ENOMEM;
	}

	RawData_FW_Rawdata = (int32_t *)kzalloc(RawData_BufSize, GFP_KERNEL);
	if (!RawData_FW_Rawdata) {
		NVT_ERR("kzalloc for RawData_FW_Rawdata failed!\n");
		return -ENOMEM;
	}

	RawData_FW_CC = (int32_t *)kzalloc(RawData_BufSize, GFP_KERNEL);
	if (!RawData_FW_CC) {
		NVT_ERR("kzalloc for RawData_FW_CC failed!\n");
		return -ENOMEM;
	}

	if (ts->pen_support) {
		RawData_PenTipX_Raw = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenTipX_Raw) {
			NVT_ERR("kzalloc for RawData_PenTipX_Raw failed!\n");
			return -ENOMEM;
		}

		RawData_PenTipY_Raw = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenTipY_Raw) {
			NVT_ERR("kzalloc for RawData_PenTipY_Raw failed!\n");
			return -ENOMEM;
		}

		RawData_PenRingX_Raw = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenRingX_Raw) {
			NVT_ERR("kzalloc for RawData_PenRingX_Raw failed!\n");
			return -ENOMEM;
		}

		RawData_PenRingY_Raw = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenRingY_Raw) {
			NVT_ERR("kzalloc for RawData_PenRingY_Raw failed!\n");
			return -ENOMEM;
		}

		RawData_PenTipX_DiffMax = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenTipX_DiffMax) {
			NVT_ERR("kzalloc for RawData_PenTipX_DiffMax failed!\n");
			return -ENOMEM;
		}

		RawData_PenTipX_DiffMin = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenTipX_DiffMin) {
			NVT_ERR("kzalloc for RawData_PenTipX_DiffMin failed!\n");
			return -ENOMEM;
		}

		RawData_PenTipY_DiffMax = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenTipY_DiffMax) {
			NVT_ERR("kzalloc for RawData_PenTipY_DiffMax failed!\n");
			return -ENOMEM;
		}

		RawData_PenTipY_DiffMin = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenTipY_DiffMin) {
			NVT_ERR("kzalloc for RawData_PenTipY_DiffMin failed!\n");
			return -ENOMEM;
		}

		RawData_PenRingX_DiffMax = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenRingX_DiffMax) {
			NVT_ERR("kzalloc for RawData_PenRingX_DiffMax failed!\n");
			return -ENOMEM;
		}

		RawData_PenRingX_DiffMin = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenRingX_DiffMin) {
			NVT_ERR("kzalloc for RawData_PenRingX_DiffMin failed!\n");
			return -ENOMEM;
		}

		RawData_PenRingY_DiffMax = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenRingY_DiffMax) {
			NVT_ERR("kzalloc for RawData_PenRingY_DiffMax failed!\n");
			return -ENOMEM;
		}

		RawData_PenRingY_DiffMin = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenRingY_DiffMin) {
			NVT_ERR("kzalloc for RawData_PenRingY_DiffMin failed!\n");
			return -ENOMEM;
		}
	} /* if (ts->pen_support) */

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen free buffer for mp selftest.

return:
	n.a.
*******************************************************/
static void nvt_mp_buffer_deinit(void)
{
	if (RecordResult_Short) {
		kfree(RecordResult_Short);
		RecordResult_Short = NULL;
	}

	if (RecordResult_Open) {
		kfree(RecordResult_Open);
		RecordResult_Open = NULL;
	}

	if (RecordResult_FW_Rawdata) {
		kfree(RecordResult_FW_Rawdata);
		RecordResult_FW_Rawdata = NULL;
	}

	if (RecordResult_FW_CC) {
		kfree(RecordResult_FW_CC);
		RecordResult_FW_CC = NULL;
	}

	if (RecordResult_FW_DiffMax) {
		kfree(RecordResult_FW_DiffMax);
		RecordResult_FW_DiffMax = NULL;
	}

	if (RecordResult_FW_DiffMin) {
		kfree(RecordResult_FW_DiffMin);
		RecordResult_FW_DiffMin = NULL;
	}

	if (ts->pen_support) {
		if (RecordResult_PenTipX_Raw) {
			kfree(RecordResult_PenTipX_Raw);
			RecordResult_PenTipX_Raw = NULL;
		}

		if (RecordResult_PenTipY_Raw) {
			kfree(RecordResult_PenTipY_Raw);
			RecordResult_PenTipY_Raw = NULL;
		}

		if (RecordResult_PenRingX_Raw) {
			kfree(RecordResult_PenRingX_Raw);
			RecordResult_PenRingX_Raw = NULL;
		}

		if (RecordResult_PenRingY_Raw) {
			kfree(RecordResult_PenRingY_Raw);
			RecordResult_PenRingY_Raw = NULL;
		}

		if (RecordResult_PenTipX_DiffMax) {
			kfree(RecordResult_PenTipX_DiffMax);
			RecordResult_PenTipX_DiffMax = NULL;
		}

		if (RecordResult_PenTipX_DiffMin) {
			kfree(RecordResult_PenTipX_DiffMin);
			RecordResult_PenTipX_DiffMin = NULL;
		}

		if (RecordResult_PenTipY_DiffMax) {
			kfree(RecordResult_PenTipY_DiffMax);
			RecordResult_PenTipY_DiffMax = NULL;
		}

		if (RecordResult_PenTipY_DiffMin) {
			kfree(RecordResult_PenTipY_DiffMin);
			RecordResult_PenTipY_DiffMin = NULL;
		}

		if (RecordResult_PenRingX_DiffMax) {
			kfree(RecordResult_PenRingX_DiffMax);
			RecordResult_PenRingX_DiffMax = NULL;
		}

		if (RecordResult_PenRingX_DiffMin) {
			kfree(RecordResult_PenRingX_DiffMin);
			RecordResult_PenRingX_DiffMin = NULL;
		}

		if (RecordResult_PenRingY_DiffMax) {
			kfree(RecordResult_PenRingY_DiffMax);
			RecordResult_PenRingY_DiffMax = NULL;
		}

		if (RecordResult_PenRingY_DiffMin) {
			kfree(RecordResult_PenRingY_DiffMin);
			RecordResult_PenRingY_DiffMin = NULL;
		}
	} /* if (ts->pen_support) */

	if (RawData_Short) {
		kfree(RawData_Short);
		RawData_Short = NULL;
	}

	if (RawData_Open) {
		kfree(RawData_Open);
		RawData_Open = NULL;
	}

	if (RawData_Diff) {
		kfree(RawData_Diff);
		RawData_Diff = NULL;
	}

	if (RawData_Diff_Min) {
		kfree(RawData_Diff_Min);
		RawData_Diff_Min = NULL;
	}

	if (RawData_Diff_Max) {
		kfree(RawData_Diff_Max);
		RawData_Diff_Max = NULL;
	}

	if (RawData_FW_Rawdata) {
		kfree(RawData_FW_Rawdata);
		RawData_FW_Rawdata = NULL;
	}

	if (RawData_FW_CC) {
		kfree(RawData_FW_CC);
		RawData_FW_CC = NULL;
	}

	if (ts->pen_support) {
		if (RawData_PenTipX_Raw) {
			kfree(RawData_PenTipX_Raw);
			RawData_PenTipX_Raw = NULL;
		}

		if (RawData_PenTipY_Raw) {
			kfree(RawData_PenTipY_Raw);
			RawData_PenTipY_Raw = NULL;
		}

		if (RawData_PenRingX_Raw) {
			kfree(RawData_PenRingX_Raw);
			RawData_PenRingX_Raw = NULL;
		}

		if (RawData_PenRingY_Raw) {
			kfree(RawData_PenRingY_Raw);
			RawData_PenRingY_Raw = NULL;
		}

		if (RawData_PenTipX_DiffMax) {
			kfree(RawData_PenTipX_DiffMax);
			RawData_PenTipX_DiffMax = NULL;
		}

		if (RawData_PenTipX_DiffMin) {
			kfree(RawData_PenTipX_DiffMin);
			RawData_PenTipX_DiffMin = NULL;
		}

		if (RawData_PenTipY_DiffMax) {
			kfree(RawData_PenTipY_DiffMax);
			RawData_PenTipY_DiffMax = NULL;
		}

		if (RawData_PenTipY_DiffMin) {
			kfree(RawData_PenTipY_DiffMin);
			RawData_PenTipY_DiffMin = NULL;
		}

		if (RawData_PenRingX_DiffMax) {
			kfree(RawData_PenRingX_DiffMax);
			RawData_PenRingX_DiffMax = NULL;
		}

		if (RawData_PenRingX_DiffMin) {
			kfree(RawData_PenRingX_DiffMin);
			RawData_PenRingX_DiffMin = NULL;
		}

		if (RawData_PenRingY_DiffMax) {
			kfree(RawData_PenRingY_DiffMax);
			RawData_PenRingY_DiffMax = NULL;
		}

		if (RawData_PenRingY_DiffMin) {
			kfree(RawData_PenRingY_DiffMin);
			RawData_PenRingY_DiffMin = NULL;
		}
	} /* if (ts->pen_support) */
}

static void nvt_print_data_log_in_one_line(int32_t *data, int32_t data_num)
{
	char *tmp_log = NULL;
	int32_t i = 0;

	tmp_log = (char *)kzalloc(data_num * 7 + 1, GFP_KERNEL);
	if (!tmp_log) {
		NVT_ERR("kzalloc for tmp_log failed!\n ");
		return;
	}

	for (i = 0; i < data_num; i++)
		sprintf(tmp_log + i * 7, "%5d, ", data[i]);

	tmp_log[data_num * 7] = '\0';
	printk("%s", tmp_log);
	if (tmp_log) {
		kfree(tmp_log);
		tmp_log = NULL;
	}

	return;
}

static void nvt_print_result_log_in_one_line(uint8_t *result, int32_t result_num)
{
	char *tmp_log = NULL;
	int32_t i = 0;

	tmp_log = (char *)kzalloc(result_num * 6 + 1, GFP_KERNEL);
	if (!tmp_log) {
		NVT_ERR("kzalloc for tmp_log failed!\n ");
		return;
	}

	for (i = 0; i < result_num; i++)
		sprintf(tmp_log + i * 6, "0x%02X, ", result[i]);

	tmp_log[result_num * 6] = '\0';
	printk("%s", tmp_log);
	if (tmp_log) {
		kfree(tmp_log);
		tmp_log = NULL;
	}

	return;
}

/*******************************************************
Description:
	Novatek touchscreen self-test criteria print function.

return:
	n.a.
*******************************************************/
static void nvt_print_lmt_array(int32_t *array, int32_t x_ch, int32_t y_ch)
{
	int32_t j = 0;

	for (j = 0; j < y_ch; j++) {
		nvt_print_data_log_in_one_line(array + j * x_ch, x_ch);
		printk("\n");
	}
}

static void nvt_print_criteria(void)
{
	NVT_LOG("enter\n");

	//---PS_Config_Lmt_Short_Rawdata---
	printk("PS_Config_Lmt_Short_Rawdata_P:\n");
	nvt_print_lmt_array(PS_Config_Lmt_Short_Rawdata_P, ts->x_num, ts->y_num);
	printk("PS_Config_Lmt_Short_Rawdata_N:\n");
	nvt_print_lmt_array(PS_Config_Lmt_Short_Rawdata_N, ts->x_num, ts->y_num);

	//---PS_Config_Lmt_Open_Rawdata---
	printk("PS_Config_Lmt_Open_Rawdata_P:\n");
	nvt_print_lmt_array(PS_Config_Lmt_Open_Rawdata_P, ts->x_num, ts->y_num);
	printk("PS_Config_Lmt_Open_Rawdata_N:\n");
	nvt_print_lmt_array(PS_Config_Lmt_Open_Rawdata_N, ts->x_num, ts->y_num);

	//---PS_Config_Lmt_FW_Rawdata---
	printk("PS_Config_Lmt_FW_Rawdata_P:\n");
	nvt_print_lmt_array(PS_Config_Lmt_FW_Rawdata_P, ts->x_num, ts->y_num);
	printk("PS_Config_Lmt_FW_Rawdata_N:\n");
	nvt_print_lmt_array(PS_Config_Lmt_FW_Rawdata_N, ts->x_num, ts->y_num);

	//---PS_Config_Lmt_FW_CC---
	printk("PS_Config_Lmt_FW_CC_P:\n");
	nvt_print_lmt_array(PS_Config_Lmt_FW_CC_P, ts->x_num, ts->y_num);
	printk("PS_Config_Lmt_FW_CC_N:\n");
	nvt_print_lmt_array(PS_Config_Lmt_FW_CC_N, ts->x_num, ts->y_num);

	//---PS_Config_Lmt_FW_Diff---
	printk("PS_Config_Lmt_FW_Diff_P:\n");
	nvt_print_lmt_array(PS_Config_Lmt_FW_Diff_P, ts->x_num, ts->y_num);
	printk("PS_Config_Lmt_FW_Diff_N:\n");
	nvt_print_lmt_array(PS_Config_Lmt_FW_Diff_N, ts->x_num, ts->y_num);

	if (ts->pen_support) {
		//---PS_Config_Lmt_PenTipX_FW_Raw---
		printk("PS_Config_Lmt_PenTipX_FW_Raw_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenTipX_FW_Raw_P, ts->pen_x_num_x, ts->pen_x_num_y);
		printk("PS_Config_Lmt_PenTipX_FW_Raw_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenTipX_FW_Raw_N, ts->pen_x_num_x, ts->pen_x_num_y);

		//---PS_Config_Lmt_PenTipY_FW_Raw---
		printk("PS_Config_Lmt_PenTipY_FW_Raw_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenTipY_FW_Raw_P, ts->pen_y_num_x, ts->pen_y_num_y);
		printk("PS_Config_Lmt_PenTipY_FW_Raw_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenTipY_FW_Raw_N, ts->pen_y_num_x, ts->pen_y_num_y);

		//---PS_Config_Lmt_PenRingX_FW_Raw---
		printk("PS_Config_Lmt_PenRingX_FW_Raw_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenRingX_FW_Raw_P, ts->pen_x_num_x, ts->pen_x_num_y);
		printk("PS_Config_Lmt_PenRingX_FW_Raw_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenRingX_FW_Raw_N, ts->pen_x_num_x, ts->pen_x_num_y);

		//---PS_Config_Lmt_PenRingY_FW_Raw---
		printk("PS_Config_Lmt_PenRingY_FW_Raw_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenRingY_FW_Raw_P, ts->pen_y_num_x, ts->pen_y_num_y);
		printk("PS_Config_Lmt_PenRingY_FW_Raw_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenRingY_FW_Raw_N, ts->pen_y_num_x, ts->pen_y_num_y);

		//---PS_Config_Lmt_PenTipX_FW_Diff---
		printk("PS_Config_Lmt_PenTipX_FW_Diff_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenTipX_FW_Diff_P, ts->pen_x_num_x, ts->pen_x_num_y);
		printk("PS_Config_Lmt_PenTipX_FW_Diff_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenTipX_FW_Diff_N, ts->pen_x_num_x, ts->pen_x_num_y);

		//---PS_Config_Lmt_PenTipY_FW_Diff---
		printk("PS_Config_Lmt_PenTipY_FW_Diff_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenTipY_FW_Diff_P, ts->pen_y_num_x, ts->pen_y_num_y);
		printk("PS_Config_Lmt_PenTipY_FW_Diff_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenTipY_FW_Diff_N, ts->pen_y_num_x, ts->pen_y_num_y);

		//---PS_Config_Lmt_PenRingX_FW_Diff---
		printk("PS_Config_Lmt_PenRingX_FW_Diff_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenRingX_FW_Diff_P, ts->pen_x_num_x, ts->pen_x_num_y);
		printk("PS_Config_Lmt_PenRingX_FW_Diff_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenRingX_FW_Diff_N, ts->pen_x_num_x, ts->pen_x_num_y);

		//---PS_Config_Lmt_PenRingY_FW_Diff---
		printk("PS_Config_Lmt_PenRingY_FW_Diff_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenRingY_FW_Diff_P, ts->pen_y_num_x, ts->pen_y_num_y);
		printk("PS_Config_Lmt_PenRingY_FW_Diff_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenRingY_FW_Diff_N, ts->pen_y_num_x, ts->pen_y_num_y);
	} /* if (ts->pen_support) */

	NVT_LOG("leave\n");
}

static int32_t nvt_save_rawdata_to_csv(int32_t *rawdata, uint8_t x_ch, uint8_t y_ch,
		const char *file_path, uint32_t offset)
{
	int32_t x = 0;
	int32_t y = 0;
	int32_t iArrayIndex = 0;
	struct file *fp = NULL;
	char *fbufp = NULL;
#ifdef HAVE_SET_FS
	mm_segment_t org_fs;
#endif
	int32_t write_ret = 0;
	uint32_t output_len = 0;
	loff_t pos = 0;

	printk("%s:enter\n", __func__);
	fbufp = (char *)kzalloc(16384, GFP_KERNEL);
	if (!fbufp) {
		NVT_ERR("kzalloc for fbufp failed!\n");
		return -ENOMEM;
	}

	for (y = 0; y < y_ch; y++) {
		for (x = 0; x < x_ch; x++) {
			iArrayIndex = y * x_ch + x;
			sprintf(fbufp + iArrayIndex * 7 + y * 2, "%5d, ", rawdata[iArrayIndex]);
		}
		nvt_print_data_log_in_one_line(rawdata + y * x_ch, x_ch);
		printk("\n");
		sprintf(fbufp + (iArrayIndex + 1) * 7 + y * 2, "\r\n");
	}

#ifdef HAVE_SET_FS
	org_fs = get_fs();
	set_fs(KERNEL_DS);
#endif
	if (offset == 0)
		fp = filp_open(file_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
	else
		fp = filp_open(file_path, O_RDWR | O_CREAT, 0666);
	if (fp == NULL || IS_ERR(fp)) {
		NVT_ERR("open %s failed\n", file_path);
#ifdef HAVE_SET_FS
		set_fs(org_fs);
#endif
		if (fbufp) {
			kfree(fbufp);
			fbufp = NULL;
		}
		return -1;
	}

	output_len = y_ch * x_ch * 7 + y_ch * 2;
	pos = offset;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
	write_ret = kernel_write(fp, fbufp, output_len, &pos);
#else
	write_ret = vfs_write(fp, (char __user *)fbufp, output_len, &pos);
#endif
	if (write_ret <= 0) {
		NVT_ERR("write %s failed\n", file_path);
#ifdef HAVE_SET_FS
		set_fs(org_fs);
#endif
		if (fp) {
			filp_close(fp, NULL);
			fp = NULL;
		}
		if (fbufp) {
			kfree(fbufp);
			fbufp = NULL;
		}
		return -1;
	}

#ifdef HAVE_SET_FS
	set_fs(org_fs);
#endif

	if (fp) {
		filp_close(fp, NULL);
		fp = NULL;
	}
	if (fbufp) {
		kfree(fbufp);
		fbufp = NULL;
	}

	printk("%s:leave\n", __func__);

	return 0;
}

static int32_t nvt_polling_hand_shake_status(void)
{
	uint8_t buf[8] = { 0 };
	int32_t i = 0;
	const int32_t retry = 250;

	usleep_range(20000, 20000);

	for (i = 0; i < retry; i++) {
		//---set xdata index to EVENT BUF ADDR---
		nvt_set_page(I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE);

		//---read fw status---
		buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
		buf[1] = 0x00;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);

		if ((buf[1] == 0xA0) || (buf[1] == 0xA1))
			break;

		usleep_range(10000, 10000);
	}

	if (i >= retry) {
		NVT_ERR("polling hand shake status failed, buf[1]=0x%02X\n", buf[1]);

		// Read back 5 bytes from offset EVENT_MAP_HOST_CMD for debug check
		nvt_set_page(I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);

		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = 0x00;
		buf[2] = 0x00;
		buf[3] = 0x00;
		buf[4] = 0x00;
		buf[5] = 0x00;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 6);
		NVT_ERR("Read back 5 bytes from offset EVENT_MAP_HOST_CMD: 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\n",
				buf[1], buf[2], buf[3], buf[4], buf[5]);

		nvt_read_fw_history(ts->mmap->MMAP_HISTORY_EVENT0);
		nvt_read_fw_history(ts->mmap->MMAP_HISTORY_EVENT1);

		return -1;
	} else {
		return 0;
	}
}

static int8_t nvt_switch_FreqHopEnDis(uint8_t FreqHopEnDis)
{
	uint8_t buf[8] = { 0 };
	uint8_t retry = 0;
	int8_t ret = 0;

	NVT_LOG("enter\n");

	for (retry = 0; retry < 20; retry++) {
		//---set xdata index to EVENT BUF ADDR---
		nvt_set_page(I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);

		//---switch FreqHopEnDis---
		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = FreqHopEnDis;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);

		msleep(35);

		nvt_set_page(I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);
		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = 0xFF;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);

		if (buf[1] == 0x00)
			break;
	}

	if (unlikely(retry == 20)) {
		NVT_ERR("switch FreqHopEnDis 0x%02X failed, buf[1]=0x%02X\n", FreqHopEnDis, buf[1]);
		ret = -1;
	}

	NVT_LOG("leave\n");

	return ret;
}

static int32_t nvt_read_baseline(int32_t *xdata)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	int32_t iArrayIndex = 0;

	NVT_LOG("leave\n");

	nvt_read_mdata(ts->mmap->BASELINE_ADDR);

	nvt_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			iArrayIndex = y * x_num + x;
			xdata[iArrayIndex] = (int16_t)xdata[iArrayIndex];
		}
	}

	printk("%s:\n", __func__);
	// Save Rawdata to CSV file
	if (nvt_save_rawdata_to_csv(xdata, ts->x_num, ts->y_num, FW_RAWDATA_CSV_FILE, 0) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}

	NVT_LOG("leave\n");

	return 0;
}

static int32_t nvt_read_CC(int32_t *xdata)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	int32_t iArrayIndex = 0;

	NVT_LOG("enter\n");

	if (nvt_get_fw_pipe() == 0)
		nvt_read_mdata(ts->mmap->DIFF_PIPE1_ADDR);
	else
		nvt_read_mdata(ts->mmap->DIFF_PIPE0_ADDR);

	nvt_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			iArrayIndex = y * x_num + x;
			xdata[iArrayIndex] = (int16_t)xdata[iArrayIndex];
		}
	}

	printk("%s:\n", __func__);
	// Save Rawdata to CSV file
	if (nvt_save_rawdata_to_csv(xdata, ts->x_num, ts->y_num, FW_CC_CSV_FILE, 0) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}

	NVT_LOG("\n");

	return 0;
}

static int32_t nvt_read_pen_baseline(void)
{
	uint32_t csv_output_offset = 0;

	NVT_LOG("enter\n");

	nvt_read_get_num_mdata(ts->mmap->PEN_2D_BL_TIP_X_ADDR, RawData_PenTipX_Raw,
			ts->pen_x_num_x * ts->pen_x_num_y);
	nvt_read_get_num_mdata(ts->mmap->PEN_2D_BL_TIP_Y_ADDR, RawData_PenTipY_Raw,
			ts->pen_y_num_x * ts->pen_y_num_y);
	nvt_read_get_num_mdata(ts->mmap->PEN_2D_BL_RING_X_ADDR, RawData_PenRingX_Raw,
			ts->pen_x_num_x * ts->pen_x_num_y);
	nvt_read_get_num_mdata(ts->mmap->PEN_2D_BL_RING_Y_ADDR, RawData_PenRingY_Raw,
			ts->pen_y_num_x * ts->pen_y_num_y);

	// Save Rawdata to CSV file
	printk("%s:RawData_PenTipX_Raw\n", __func__);
	if (nvt_save_rawdata_to_csv(RawData_PenTipX_Raw, ts->pen_x_num_x, ts->pen_x_num_y,
			PEN_FW_RAW_TEST_CSV_FILE, csv_output_offset) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}
	csv_output_offset += ts->pen_x_num_y * ts->pen_x_num_x * 7 + ts->pen_x_num_y * 2;
	printk("%s:RawData_PenTipY_Raw\n", __func__);
	if (nvt_save_rawdata_to_csv(RawData_PenTipY_Raw, ts->pen_y_num_x, ts->pen_y_num_y,
			PEN_FW_RAW_TEST_CSV_FILE, csv_output_offset) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}
	csv_output_offset += ts->pen_y_num_y * ts->pen_y_num_x * 7 + ts->pen_y_num_y * 2;
	printk("%s:RawData_PenRingX_Raw\n", __func__);
	if (nvt_save_rawdata_to_csv(RawData_PenRingX_Raw, ts->pen_x_num_x, ts->pen_x_num_y,
			PEN_FW_RAW_TEST_CSV_FILE, csv_output_offset) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}
	csv_output_offset += ts->pen_x_num_y * ts->pen_x_num_x * 7 + ts->pen_x_num_y * 2;
	printk("%s:RawData_PenRingY_Raw\n", __func__);
	if (nvt_save_rawdata_to_csv(RawData_PenRingY_Raw, ts->pen_y_num_x, ts->pen_y_num_y,
			PEN_FW_RAW_TEST_CSV_FILE, csv_output_offset) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}

	NVT_LOG("leave\n");

	return 0;
}

static void nvt_enable_noise_collect(int32_t frame_num)
{
	uint8_t buf[8] = { 0 };

	//---set xdata index to EVENT BUF ADDR---
	nvt_set_page(I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);

	//---enable noise collect---
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = 0x47;
	buf[2] = 0xAA;
	buf[3] = frame_num;
	buf[4] = 0x00;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 5);
}

static int32_t nvt_read_fw_noise(int32_t *xdata)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	int32_t iArrayIndex = 0;
	int32_t frame_num = 0;
	uint32_t rawdata_diff_min_offset = 0;
	uint32_t csv_pen_noise_offset = 0;

	NVT_LOG("enter\n");

	//---Enter Test Mode---
	if (nvt_clear_fw_status())
		return -EAGAIN;

	frame_num = PS_Config_Diff_Test_Frame / 10;
	if (frame_num <= 0)
		frame_num = 1;
	printk("%s: frame_num=%d\n", __func__, frame_num);
	nvt_enable_noise_collect(frame_num);
	// need wait PS_Config_Diff_Test_Frame * 8.3ms
	msleep(frame_num * 83);

	if (nvt_polling_hand_shake_status())
		return -EAGAIN;

	if (nvt_get_fw_pipe() == 0)
		nvt_read_mdata(ts->mmap->DIFF_PIPE0_ADDR);
	else
		nvt_read_mdata(ts->mmap->DIFF_PIPE1_ADDR);

	nvt_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			iArrayIndex = y * x_num + x;
			RawData_Diff_Max[iArrayIndex] = (int8_t)((xdata[iArrayIndex] >> 8) & 0xFF);
			RawData_Diff_Min[iArrayIndex] = (int8_t)(xdata[iArrayIndex] & 0xFF);
		}
	}

	if (ts->pen_support) {
		// get pen noise data
		nvt_read_get_num_mdata(ts->mmap->PEN_2D_DIFF_TIP_X_ADDR, RawData_PenTipX_DiffMax,
				ts->pen_x_num_x * ts->pen_x_num_y);
		nvt_read_get_num_mdata(ts->mmap->PEN_2D_RAW_TIP_X_ADDR, RawData_PenTipX_DiffMin,
				ts->pen_x_num_x * ts->pen_x_num_y);
		nvt_read_get_num_mdata(ts->mmap->PEN_2D_DIFF_TIP_Y_ADDR, RawData_PenTipY_DiffMax,
				ts->pen_y_num_x * ts->pen_y_num_y);
		nvt_read_get_num_mdata(ts->mmap->PEN_2D_RAW_TIP_Y_ADDR, RawData_PenTipY_DiffMin,
				ts->pen_y_num_x * ts->pen_y_num_y);
		nvt_read_get_num_mdata(ts->mmap->PEN_2D_DIFF_RING_X_ADDR, RawData_PenRingX_DiffMax,
				ts->pen_x_num_x * ts->pen_x_num_y);
		nvt_read_get_num_mdata(ts->mmap->PEN_2D_RAW_RING_X_ADDR, RawData_PenRingX_DiffMin,
				ts->pen_x_num_x * ts->pen_x_num_y);
		nvt_read_get_num_mdata(ts->mmap->PEN_2D_DIFF_RING_Y_ADDR, RawData_PenRingY_DiffMax,
				ts->pen_y_num_x * ts->pen_y_num_y);
		nvt_read_get_num_mdata(ts->mmap->PEN_2D_RAW_RING_Y_ADDR, RawData_PenRingY_DiffMin,
				ts->pen_y_num_x * ts->pen_y_num_y);
	} /* if (ts->pen_support) */

	//---Leave Test Mode---
	nvt_change_mode(NORMAL_MODE);

	printk("%s:RawData_Diff_Max:\n", __func__);
	// Save Rawdata to CSV file
	if (nvt_save_rawdata_to_csv(RawData_Diff_Max, ts->x_num, ts->y_num, NOISE_TEST_CSV_FILE, 0) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}

	rawdata_diff_min_offset = ts->y_num * ts->x_num * 7 + ts->y_num * 2;
	printk("%s:RawData_Diff_Min:\n", __func__);
	// Save Rawdata to CSV file
	if (nvt_save_rawdata_to_csv(RawData_Diff_Min, ts->x_num, ts->y_num, NOISE_TEST_CSV_FILE,
			rawdata_diff_min_offset) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}

	if (ts->pen_support) {
		// save pen noise to csv
		printk("%s:RawData_PenTipX_DiffMax:\n", __func__);
		if (nvt_save_rawdata_to_csv(RawData_PenTipX_DiffMax, ts->pen_x_num_x, ts->pen_x_num_y,
				PEN_NOISE_TEST_CSV_FILE, csv_pen_noise_offset) < 0) {
			NVT_ERR("save rawdata to CSV file failed\n");
			return -EAGAIN;
		}
		csv_pen_noise_offset += ts->pen_x_num_y * ts->pen_x_num_x * 7 + ts->pen_x_num_y * 2;
		printk("%s:RawData_PenTipX_DiffMin:\n", __func__);
		if (nvt_save_rawdata_to_csv(RawData_PenTipX_DiffMin, ts->pen_x_num_x, ts->pen_x_num_y,
				PEN_NOISE_TEST_CSV_FILE, csv_pen_noise_offset) < 0) {
			NVT_ERR("save rawdata to CSV file failed\n");
			return -EAGAIN;
		}
		csv_pen_noise_offset += ts->pen_x_num_y * ts->pen_x_num_x * 7 + ts->pen_x_num_y * 2;
		printk("%s:RawData_PenTipY_DiffMax:\n", __func__);
		if (nvt_save_rawdata_to_csv(RawData_PenTipY_DiffMax, ts->pen_y_num_x, ts->pen_y_num_y,
				PEN_NOISE_TEST_CSV_FILE, csv_pen_noise_offset) < 0) {
			NVT_ERR("save rawdata to CSV file failed\n");
			return -EAGAIN;
		}
		csv_pen_noise_offset += ts->pen_y_num_y * ts->pen_y_num_x * 7 + ts->pen_y_num_y * 2;
		printk("%s:RawData_PenTipY_DiffMin:\n", __func__);
		if (nvt_save_rawdata_to_csv(RawData_PenTipY_DiffMin, ts->pen_y_num_x, ts->pen_y_num_y,
				PEN_NOISE_TEST_CSV_FILE, csv_pen_noise_offset) < 0) {
			NVT_ERR("save rawdata to CSV file failed\n");
			return -EAGAIN;
		}
		csv_pen_noise_offset += ts->pen_y_num_y * ts->pen_y_num_x * 7 + ts->pen_y_num_y * 2;
		printk("%s:RawData_PenRingX_DiffMax:\n", __func__);
		if (nvt_save_rawdata_to_csv(RawData_PenRingX_DiffMax, ts->pen_x_num_x, ts->pen_x_num_y,
				PEN_NOISE_TEST_CSV_FILE, csv_pen_noise_offset) < 0) {
			NVT_ERR("save rawdata to CSV file failed\n");
			return -EAGAIN;
		}
		csv_pen_noise_offset += ts->pen_x_num_y * ts->pen_x_num_x * 7 + ts->pen_x_num_y * 2;
		printk("%s:RawData_PenRingX_DiffMin:\n", __func__);
		if (nvt_save_rawdata_to_csv(RawData_PenRingX_DiffMin, ts->pen_x_num_x, ts->pen_x_num_y,
				PEN_NOISE_TEST_CSV_FILE, csv_pen_noise_offset) < 0) {
			NVT_ERR("save rawdata to CSV file failed\n");
			return -EAGAIN;
		}
		csv_pen_noise_offset += ts->pen_x_num_y * ts->pen_x_num_x * 7 + ts->pen_x_num_y * 2;
		printk("%s:RawData_PenRingY_DiffMax:\n", __func__);
		if (nvt_save_rawdata_to_csv(RawData_PenRingY_DiffMax, ts->pen_y_num_x, ts->pen_y_num_y,
				PEN_NOISE_TEST_CSV_FILE, csv_pen_noise_offset) < 0) {
			NVT_ERR("save rawdata to CSV file failed\n");
			return -EAGAIN;
		}
		csv_pen_noise_offset += ts->pen_y_num_y * ts->pen_y_num_x * 7 + ts->pen_y_num_y * 2;
		printk("%s:RawData_PenRingY_DiffMin:\n", __func__);
		if (nvt_save_rawdata_to_csv(RawData_PenRingY_DiffMin, ts->pen_y_num_x, ts->pen_y_num_y,
				PEN_NOISE_TEST_CSV_FILE, csv_pen_noise_offset) < 0) {
			NVT_ERR("save rawdata to CSV file failed\n");
			return -EAGAIN;
		}
	} /* if (ts->pen_support) */

	NVT_LOG("leave\n");

	return 0;
}

static void nvt_enable_open_test(void)
{
	uint8_t buf[8] = { 0 };

	//---set xdata index to EVENT BUF ADDR---
	nvt_set_page(I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);

	//---enable open test---
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = 0x45;
	buf[2] = 0xAA;
	buf[3] = 0x02;
	buf[4] = 0x00;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 5);
}

static void nvt_enable_short_test(void)
{
	uint8_t buf[8] = { 0 };

	//---set xdata index to EVENT BUF ADDR---
	nvt_set_page(I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);

	//---enable short test---
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = 0x43;
	buf[2] = 0xAA;
	buf[3] = 0x02;
	buf[4] = 0x00;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 5);
}

static int32_t nvt_read_fw_open(int32_t *xdata)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	int32_t iArrayIndex = 0;

	NVT_LOG("enter\n");

	//---Enter Test Mode---
	if (nvt_clear_fw_status())
		return -EAGAIN;

	nvt_enable_open_test();

	if (nvt_polling_hand_shake_status())
		return -EAGAIN;

	if (nvt_get_fw_pipe() == 0)
		nvt_read_mdata(ts->mmap->RAW_PIPE0_ADDR);
	else
		nvt_read_mdata(ts->mmap->RAW_PIPE1_ADDR);

	nvt_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			iArrayIndex = y * x_num + x;
			xdata[iArrayIndex] = (int16_t)xdata[iArrayIndex];
		}
	}

	//---Leave Test Mode---
	nvt_change_mode(NORMAL_MODE);

	printk("%s:RawData_Open\n", __func__);
	// Save RawData to CSV file
	if (nvt_save_rawdata_to_csv(xdata, ts->x_num, ts->y_num, OPEN_TEST_CSV_FILE, 0) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}

	NVT_LOG("leave\n");

	return 0;
}

static int32_t nvt_read_fw_short(int32_t *xdata)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	int32_t iArrayIndex = 0;

	NVT_LOG("enter\n");

	//---Enter Test Mode---
	if (nvt_clear_fw_status())
		return -EAGAIN;

	nvt_enable_short_test();

	if (nvt_polling_hand_shake_status())
		return -EAGAIN;

	if (nvt_get_fw_pipe() == 0)
		nvt_read_mdata(ts->mmap->RAW_PIPE0_ADDR);
	else
		nvt_read_mdata(ts->mmap->RAW_PIPE1_ADDR);

	nvt_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			iArrayIndex = y * x_num + x;
			xdata[iArrayIndex] = (int16_t)xdata[iArrayIndex];
		}
	}

	//---Leave Test Mode---
	nvt_change_mode(NORMAL_MODE);

	printk("%s:RawData_Short\n", __func__);
	// Save Rawdata to CSV file
	if (nvt_save_rawdata_to_csv(xdata, ts->x_num, ts->y_num, SHORT_TEST_CSV_FILE, 0) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}

	NVT_LOG("leave\n");

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen raw data test for each single point function.

return:
	Executive outcomes. 0---passed. negative---failed.
*******************************************************/
static int32_t RawDataTest_SinglePoint_Sub(int32_t rawdata[], uint8_t RecordResult[], uint8_t x_ch,
		uint8_t y_ch, int32_t Rawdata_Limit_Postive[], int32_t Rawdata_Limit_Negative[])
{
	int32_t i = 0;
	int32_t j = 0;
	int32_t iArrayIndex = 0;
	bool isPass = true;

	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			iArrayIndex = j * x_ch + i;

			RecordResult[iArrayIndex] = 0x00; // default value for PASS

			if (rawdata[iArrayIndex] > Rawdata_Limit_Postive[iArrayIndex])
				RecordResult[iArrayIndex] |= 0x01;

			if (rawdata[iArrayIndex] < Rawdata_Limit_Negative[iArrayIndex])
				RecordResult[iArrayIndex] |= 0x02;
		}
	}

	//---Check RecordResult---
	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			if (RecordResult[j * x_ch + i] != 0) {
				isPass = false;
				break;
			}
		}
	}

	if (isPass == false) {
		return -1; // FAIL
	} else {
		return 0; // PASS
	}
}

/*******************************************************
Description:
	Novatek touchscreen print self-test result function.

return:
	n.a.
*******************************************************/
void print_selftest_result(struct seq_file *m, int32_t TestResult,
			   uint8_t RecordResult[], int32_t rawdata[],
			   uint8_t x_len, uint8_t y_len)
{
	int32_t i = 0;
	int32_t j = 0;
	int32_t iArrayIndex = 0;

	switch (TestResult) {
	case 0:
		nvt_mp_seq_printf(m, " PASS!\n");
		break;
	case 1:
		nvt_mp_seq_printf(m, " ERROR! Read Data FAIL!\n");
		break;
	case -1:
		nvt_mp_seq_printf(m, " FAIL!\n");
		nvt_mp_seq_printf(m, "RecordResult:\n");
		for (i = 0; i < y_len; i++) {
			for (j = 0; j < x_len; j++) {
				iArrayIndex = i * x_len + j;
				seq_printf(m, "0x%02X, ",
					   RecordResult[iArrayIndex]);
			}
			if (!nvt_mp_test_result_printed)
				nvt_print_result_log_in_one_line(
					RecordResult + i * x_len, x_len);
			nvt_mp_seq_printf(m, "\n");
		}
		nvt_mp_seq_printf(m, "ReadData:\n");
		for (i = 0; i < y_len; i++) {
			for (j = 0; j < x_len; j++) {
				iArrayIndex = i * x_len + j;
				seq_printf(m, "%5d, ", rawdata[iArrayIndex]);
			}
			if (!nvt_mp_test_result_printed)
				nvt_print_data_log_in_one_line(
					rawdata + i * x_len, x_len);
			nvt_mp_seq_printf(m, "\n");
		}
		break;
	default:
		break;
	}
	nvt_mp_seq_printf(m, "\n");
}

/*******************************************************
Description:
	Novatek touchscreen self-test sequence print show
	function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t c_show_selftest(struct seq_file *m, void *v)
{
	NVT_LOG("enter\n");

	nvt_mp_seq_printf(m, "FW Version: %d\n", fw_ver);
	nvt_mp_seq_printf(m, "\n");

	nvt_mp_seq_printf(m, "Short Test");
	print_selftest_result(m, TestResult_Short, RecordResult_Short, RawData_Short,
			ts->x_num, ts->y_num);

	nvt_mp_seq_printf(m, "Open Test");
	print_selftest_result(m, TestResult_Open, RecordResult_Open, RawData_Open,
			ts->x_num, ts->y_num);

	nvt_mp_seq_printf(m, "FW Rawdata Test");
	print_selftest_result(m, TestResult_FW_Rawdata, RecordResult_FW_Rawdata,
			RawData_FW_Rawdata, ts->x_num, ts->y_num);

	nvt_mp_seq_printf(m, "FW CC Test");
	print_selftest_result(m, TestResult_FW_CC, RecordResult_FW_CC,
			RawData_FW_CC, ts->x_num, ts->y_num);

	nvt_mp_seq_printf(m, "Noise Test");
	if ((TestResult_Noise == 0) || (TestResult_Noise == 1)) {
		print_selftest_result(m, TestResult_FW_DiffMax, RecordResult_FW_DiffMax, RawData_Diff_Max,
				ts->x_num, ts->y_num);
	} else { // TestResult_Noise is -1
		nvt_mp_seq_printf(m, " FAIL!\n");
		if (TestResult_FW_DiffMax == -1) {
			nvt_mp_seq_printf(m, "FW Diff Max");
			print_selftest_result(m, TestResult_FW_DiffMax, RecordResult_FW_DiffMax,
					RawData_Diff_Max, ts->x_num, ts->y_num);
		}
		if (TestResult_FW_DiffMin == -1) {
			nvt_mp_seq_printf(m, "FW Diff Min");
			print_selftest_result(m, TestResult_FW_DiffMin, RecordResult_FW_DiffMin,
					RawData_Diff_Min, ts->x_num, ts->y_num);
		}
	}

	if (ts->pen_support) {
		nvt_mp_seq_printf(m, "Pen FW Rawdata Test");
		if ((TestResult_Pen_FW_Raw == 0) ||
		    (TestResult_Pen_FW_Raw == 1)) {
			print_selftest_result(m, TestResult_Pen_FW_Raw, RecordResult_PenTipX_Raw,
					RawData_PenTipX_Raw, ts->pen_x_num_x, ts->pen_x_num_y);
		} else { // TestResult_Pen_FW_Raw is -1
			nvt_mp_seq_printf(m, " FAIL!\n");
			if (TestResult_PenTipX_Raw == -1) {
				nvt_mp_seq_printf(m, "Pen Tip X Raw");
				print_selftest_result(m, TestResult_PenTipX_Raw, RecordResult_PenTipX_Raw,
						RawData_PenTipX_Raw, ts->pen_x_num_x, ts->pen_x_num_y);
			}
			if (TestResult_PenTipY_Raw == -1) {
				nvt_mp_seq_printf(m, "Pen Tip Y Raw");
				print_selftest_result(m, TestResult_PenTipY_Raw, RecordResult_PenTipY_Raw,
						RawData_PenTipY_Raw, ts->pen_y_num_x, ts->pen_y_num_y);
			}
			if (TestResult_PenRingX_Raw == -1) {
				nvt_mp_seq_printf(m, "Pen Ring X Raw");
				print_selftest_result(m, TestResult_PenRingX_Raw, RecordResult_PenRingX_Raw,
						RawData_PenRingX_Raw, ts->pen_x_num_x, ts->pen_x_num_y);
			}
			if (TestResult_PenRingY_Raw == -1) {
				nvt_mp_seq_printf(m, "Pen Ring Y Raw");
				print_selftest_result(m, TestResult_PenRingY_Raw, RecordResult_PenRingY_Raw,
						RawData_PenRingY_Raw, ts->pen_y_num_x, ts->pen_y_num_y);
			}
		}

		nvt_mp_seq_printf(m, "Pen Noise Test");
		if ((TestResult_Pen_Noise == 0) ||
		    (TestResult_Pen_Noise == 1)) {
			print_selftest_result(m, TestResult_Pen_Noise, RecordResult_PenTipX_DiffMax,
					RawData_PenTipX_DiffMax, ts->pen_x_num_x, ts->pen_x_num_y);
		} else { // TestResult_Pen_Noise is -1
			nvt_mp_seq_printf(m, " FAIL!\n");
			if (TestResult_PenTipX_DiffMax == -1) {
				nvt_mp_seq_printf(m, "Pen Tip X Diff Max");
				print_selftest_result(m, TestResult_PenTipX_DiffMax, RecordResult_PenTipX_DiffMax,
						RawData_PenTipX_DiffMax, ts->pen_x_num_x, ts->pen_x_num_y);
			}
			if (TestResult_PenTipX_DiffMin == -1) {
				nvt_mp_seq_printf(m, "Pen Tip X Diff Min");
				print_selftest_result(m, TestResult_PenTipX_DiffMin, RecordResult_PenTipX_DiffMin,
						RawData_PenTipX_DiffMin, ts->pen_x_num_x, ts->pen_x_num_y);
			}
			if (TestResult_PenTipY_DiffMax == -1) {
				nvt_mp_seq_printf(m, "Pen Tip Y Diff Max");
				print_selftest_result(m, TestResult_PenTipY_DiffMax, RecordResult_PenTipY_DiffMax,
						RawData_PenTipY_DiffMax, ts->pen_y_num_x, ts->pen_y_num_y);
			}
			if (TestResult_PenTipY_DiffMin == -1) {
				nvt_mp_seq_printf(m, "Pen Tip Y Diff Min");
				print_selftest_result(m, TestResult_PenTipY_DiffMin, RecordResult_PenTipY_DiffMin,
					RawData_PenTipY_DiffMin, ts->pen_y_num_x, ts->pen_y_num_y);
			}
			if (TestResult_PenRingX_DiffMax == -1) {
				nvt_mp_seq_printf(m, "Pen Ring X Diff Max");
				print_selftest_result(m, TestResult_PenRingX_DiffMax, RecordResult_PenRingX_DiffMax,
						RawData_PenRingX_DiffMax, ts->pen_x_num_x, ts->pen_x_num_y);
			}
			if (TestResult_PenRingX_DiffMin == -1) {
				nvt_mp_seq_printf(m, "Pen Ring X Diff Min");
				print_selftest_result(m, TestResult_PenRingX_DiffMin, RecordResult_PenRingX_DiffMin,
						RawData_PenRingX_DiffMin, ts->pen_x_num_x, ts->pen_x_num_y);
			}
			if (TestResult_PenRingY_DiffMax == -1) {
				nvt_mp_seq_printf(m, "Pen Ring Y Diff Max");
				print_selftest_result(m, TestResult_PenRingY_DiffMax, RecordResult_PenRingY_DiffMax,
						RawData_PenRingY_DiffMax, ts->pen_y_num_x, ts->pen_y_num_y);
			}
			if (TestResult_PenRingY_DiffMin == -1) {
				nvt_mp_seq_printf(m, "Pen Ring Y Diff Min");
				print_selftest_result(m, TestResult_PenRingY_DiffMin, RecordResult_PenRingY_DiffMin,
						RawData_PenRingY_DiffMin, ts->pen_y_num_x, ts->pen_y_num_y);
			}
		}
	} /* if (ts->pen_support) */

	nvt_mp_test_result_printed = 1;

	NVT_LOG("leave\n");

#if NO_PEN_SELFTEST
		ts->pen_support = !ts->pen_support;
#endif

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen self-test sequence print start
	function.

return:
	Executive outcomes. 1---call next function.
	NULL---not call next function and sequence loop
	stop.
*******************************************************/
static void *c_start(struct seq_file *m, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

/*******************************************************
Description:
	Novatek touchscreen self-test sequence print next
	function.

return:
	Executive outcomes. NULL---no next and call sequence
	stop function.
*******************************************************/
static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

/*******************************************************
Description:
	Novatek touchscreen self-test sequence print stop
	function.

return:
	n.a.
*******************************************************/
static void c_stop(struct seq_file *m, void *v)
{
	return;
}

const struct seq_operations nvt_selftest_seq_ops = {
	.start = c_start,
	.next = c_next,
	.stop = c_stop,
	.show = c_show_selftest
};

#if IS_ENABLED(CONFIG_I2C_HID_OF_NVT_TS_SELFTEST)
static void goto_next_line(char **ptr)
{
	if (**ptr == '\n') {
		goto step_to_next_line;
	} else {
		do {
			*ptr = *ptr + 1;
		} while (**ptr != '\n');
	}

step_to_next_line:
	*ptr = *ptr + 1;
	return;
}

static void copy_this_line(char *dest, char *src)
{
	char *copy_from = NULL;
	char *copy_to = NULL;

	copy_from = src;
	copy_to = dest;
	do {
		*copy_to = *copy_from;
		copy_from++;
		copy_to++;
	} while ((*copy_from != '\n') && (*copy_from != '\r'));
	*copy_to = '\0';

	return;
}

static int32_t parse_mp_setting_criteria_item(char **ptr, const char *item_string,
		int32_t *item_value)
{
	char *tmp = NULL;

	NVT_LOG("%s ++\n", __func__);
	tmp = strstr(*ptr, item_string);
	if (tmp == NULL) {
		NVT_ERR("%s not found\n", item_string);
		return -1;
	}
	*ptr = tmp;
	goto_next_line(ptr);
	sscanf(*ptr, "%d,", item_value);
	NVT_LOG("%s %d\n", item_string, *item_value);

	NVT_LOG("%s --\n", __func__);
	return 0;
}

static int32_t parse_mp_criteria_item_array(char **ptr, const char *item_string,
		int32_t *item_array, uint32_t x_num, uint32_t y_num)
{
	char *tmp = NULL;
	int32_t y_index = 0;
	int32_t x_index = 0;
	char *token = NULL;
	char *tok_ptr = NULL;
	size_t offset = 0;
	char tmp_buf[512] = {0};

	NVT_LOG("%s ++\n", __func__);
	tmp = strstr(*ptr, item_string);
	if (tmp == NULL) {
		NVT_ERR("%s not found\n", item_string);
		return -1;
	}
	*ptr = tmp;

	NVT_LOG("%s\n", item_string);
	for (y_index = 0; y_index < y_num; y_index++) {
		goto_next_line(ptr);
		memset(tmp_buf, 0, sizeof(tmp_buf));
		copy_this_line(tmp_buf, *ptr);
		offset = strlen(tmp_buf);
		tok_ptr = tmp_buf;
		x_index = 0;
		while ((token = strsep(&tok_ptr, ", \t\r\0"))) {
			if (strlen(token) == 0)
				continue;
			item_array[y_index * x_num + x_index] =
					(int32_t)simple_strtol(token, NULL, 10);
			x_index++;
		}
		/* check if x_index equals to x_num */
		if (x_index != x_num) {
			NVT_ERR("%s :x_index not equal x_num!, x_index=%d, x_num=%d\n",
					item_string, x_index, x_num);
			return -1;
		}
		*ptr = *ptr + offset;
	}

	NVT_LOG("%s --\n", __func__);
	return 0;
}

static int32_t nvt_load_mp_setting_criteria_from_csv(const char *filename)
{
	int32_t retval = 0;
	char *fbufp = NULL;
	char *ptr = NULL;
	int fsize = 0;
	struct file *f = NULL;
	loff_t pos = 0;
	int32_t index = 0;

	NVT_LOG("%s ++\n", __func__);

	if (filename == NULL) {
		NVT_ERR("filename is null\n");
		return -EINVAL;
	}

	f = filp_open(filename, O_RDONLY, 644);
	if ((IS_ERR(f) || f == NULL)) {
		NVT_ERR("Failed to open %s at %ld\n", filename, PTR_ERR(f));
		return -EINVAL;
	}

	fsize = f->f_inode->i_size;
	NVT_LOG("CSV file size = %d\n", fsize);
	if (fsize <= 0) {
		NVT_ERR("The size of file is invalid\n");
		retval = -1;
		goto exit_free;
	}

	fbufp = (char *)vmalloc(fsize + 2);
	if ((IS_ERR(fbufp) || fbufp == NULL)) {
		NVT_ERR("Failed to allocate tmp memory, %ld\n", PTR_ERR(fbufp));
		retval = -1;
		goto exit_free;
	}

	kernel_read(f, fbufp, fsize, &pos);

	fbufp[fsize] = '\0';
	fbufp[fsize + 1] = '\n';

	for (index = 0; index < ARRAY_SIZE(selftest_touch_items); index++) {
		ptr = fbufp;
		if (parse_mp_criteria_item_array(&ptr, selftest_touch_items[index].name,
					selftest_touch_items[index].rawdata, ts->x_num,
					ts->y_num) < 0) {
			NVT_ERR("Cannot get %s array value!\n", selftest_touch_items[index].name);
			retval = -1;
			goto exit_free;
		}
	}

	if (ts->pen_support) {
		selftest_pen_x_num_x = ts->pen_x_num_x;
		selftest_pen_x_num_y = ts->pen_x_num_y;
		selftest_pen_y_num_x = ts->pen_y_num_x;
		selftest_pen_y_num_y = ts->pen_y_num_y;
		for (index = 0; index < ARRAY_SIZE(selftest_pen_items); index++) {
			ptr = fbufp;
			if (parse_mp_criteria_item_array(&ptr, selftest_pen_items[index].name,
						selftest_pen_items[index].rawdata,
						*selftest_pen_items[index].width,
						*selftest_pen_items[index].height) < 0) {
				NVT_ERR("Cannot get %s array value!\n",
						selftest_pen_items[index].name);
				retval = -1;
				goto exit_free;
			}
		}
	}

	ptr = fbufp;
	if (parse_mp_setting_criteria_item(&ptr, "PS_Config_Diff_Test_Frame:",
				&PS_Config_Diff_Test_Frame) < 0) {
		NVT_ERR("Cannot get PS_Config_Diff_Test_Frame array value!\n");
		retval = -1;
		goto exit_free;
	}

	NVT_LOG("Load MP setting and criteria from CSV file finished.\n");
	retval = 0;

exit_free:
	if (f != NULL)
		filp_close(f, NULL);

	if (fbufp) {
		vfree(fbufp);
		fbufp = NULL;
	}

	NVT_LOG("--, retval=%d\n", retval);
	return retval;
}
#endif

/*******************************************************
Description:
	Novatek touchscreen /proc/nvt_selftest open function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_selftest_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct device_node *np = ts->client->dev.of_node;
	unsigned char mpcriteria[32] = { 0 }; //novatek-mp-criteria-default

#if IS_ENABLED(CONFIG_I2C_HID_OF_NVT_TS_SELFTEST)
	char mp_setting_criteria_csv_filename[64] = { 0 };
#endif

#if NO_PEN_SELFTEST
	ts->pen_support = !ts->pen_support;
#endif

	TestResult_Short = 0;
	TestResult_Open = 0;
	TestResult_FW_Rawdata = 0;
	TestResult_FW_CC = 0;
	TestResult_Noise = 0;
	TestResult_FW_DiffMax = 0;
	TestResult_FW_DiffMin = 0;
	if (ts->pen_support) {
		TestResult_Pen_FW_Raw = 0;
		TestResult_PenTipX_Raw = 0;
		TestResult_PenTipY_Raw = 0;
		TestResult_PenRingX_Raw = 0;
		TestResult_PenRingY_Raw = 0;
		TestResult_Pen_Noise = 0;
		TestResult_PenTipX_DiffMax = 0;
		TestResult_PenTipX_DiffMin = 0;
		TestResult_PenTipY_DiffMax = 0;
		TestResult_PenTipY_DiffMin = 0;
		TestResult_PenRingX_DiffMax = 0;
		TestResult_PenRingX_DiffMin = 0;
		TestResult_PenRingY_DiffMax = 0;
		TestResult_PenRingY_DiffMin = 0;
	} /* if (ts->pen_support) */

	NVT_LOG("enter\n");

	if (mutex_lock_interruptible(&ts->lock)) {
#if NO_PEN_SELFTEST
		ts->pen_support = !ts->pen_support;
#endif
		return -ERESTARTSYS;
	}

	nvt_bootloader_reset();
	if (nvt_check_fw_reset_state(RESET_STATE_REK)) {
		mutex_unlock(&ts->lock);
		NVT_ERR("check fw reset state failed!\n");
#if NO_PEN_SELFTEST
		ts->pen_support = !ts->pen_support;
#endif
		return -EAGAIN;
	}

	if (nvt_get_fw_info()) {
		mutex_unlock(&ts->lock);
		NVT_ERR("get fw info failed!\n");
 #if NO_PEN_SELFTEST
		ts->pen_support = !ts->pen_support;
 #endif
		return -EAGAIN;
	}

	fw_ver = ts->fw_ver;

#if IS_ENABLED(CONFIG_I2C_HID_OF_NVT_TS_SELFTEST)
	/* ---Check if MP Setting Criteria CSV file exist and load--- */
	ret = snprintf(mp_setting_criteria_csv_filename, sizeof(mp_setting_criteria_csv_filename),
			"%sNT36xxx_MP_Setting_Criteria_%04X.csv",
			TG_INX_TEST_CONFIG_CSV_PATH, ts->nvt_pid);
	if (ret < 0) {
		NVT_ERR("snprintf copy string failed. (%d)\n", ret);
		return -EINVAL;
	}

	NVT_LOG("MP setting criteria csv filename: %s\n", mp_setting_criteria_csv_filename);
	if (nvt_load_mp_setting_criteria_from_csv(mp_setting_criteria_csv_filename) < 0) {
		NVT_ERR("SelfTest MP setting criteria CSV file not exist or load failed\n");
#endif
		/* Parsing criteria from dts */
		if (of_property_read_bool(np, "novatek,mp-support-dt")) {
			/*
			* Parsing Criteria by Novatek PID
			* The string rule is "novatek-mp-criteria-<nvt_pid>"
			* nvt_pid is 2 bytes (show hex).
			*
			* Ex. nvt_pid = 500A
			*     mpcriteria = "novatek-mp-criteria-500A"
			*/
			ret = snprintf(mpcriteria, sizeof(mpcriteria), "novatek-mp-criteria-%04X",
					ts->nvt_pid);
			if (ret < 0) {
				NVT_ERR("snprintf copy string failed. (%d)\n", ret);
				return -EINVAL;
			}

			if (nvt_mp_parse_dt(np, mpcriteria)) {
				mutex_unlock(&ts->lock);
				NVT_ERR("mp parse device tree failed!\n");
#if NO_PEN_SELFTEST
				ts->pen_support = !ts->pen_support;
#endif
				return -EINVAL;
			}
		} else {
			NVT_LOG("Not found novatek,mp-support-dt, use default setting\n");
			//---Print Test Criteria---
			nvt_print_criteria();
		}
#if IS_ENABLED(CONFIG_I2C_HID_OF_NVT_TS_SELFTEST)
	} else {
		NVT_LOG("SelfTest MP setting criteria loaded from CSV file\n");
	}
#endif

	nvt_bootloader_reset();
	if (nvt_check_fw_reset_state(RESET_STATE_REK)) {
		mutex_unlock(&ts->lock);
		NVT_ERR("check fw reset state failed!\n");
#if NO_PEN_SELFTEST
		ts->pen_support = !ts->pen_support;
#endif
		return -EAGAIN;
	}

	if (nvt_switch_FreqHopEnDis(FREQ_HOP_DISABLE)) {
		mutex_unlock(&ts->lock);
		NVT_ERR("switch frequency hopping disable failed!\n");
#if NO_PEN_SELFTEST
		ts->pen_support = !ts->pen_support;
#endif
		return -EAGAIN;
	}

	if (nvt_check_fw_reset_state(RESET_STATE_NORMAL_RUN)) {
		mutex_unlock(&ts->lock);
		NVT_ERR("check fw reset state failed!\n");
#if NO_PEN_SELFTEST
		ts->pen_support = !ts->pen_support;
#endif
		return -EAGAIN;
	}

	msleep(100);

	//---Enter Test Mode---
	if (nvt_clear_fw_status()) {
		mutex_unlock(&ts->lock);
		NVT_ERR("clear fw status failed!\n");
#if NO_PEN_SELFTEST
		ts->pen_support = !ts->pen_support;
#endif
		return -EAGAIN;
	}

	nvt_change_mode(MP_MODE_CC);

	if (nvt_check_fw_status()) {
		mutex_unlock(&ts->lock);
		NVT_ERR("check fw status failed!\n");
#if NO_PEN_SELFTEST
		ts->pen_support = !ts->pen_support;
#endif
		return -EAGAIN;
	}

	//---FW Rawdata Test---
	if (nvt_read_baseline(RawData_FW_Rawdata) != 0) {
		TestResult_FW_Rawdata = 1;
	} else {
		TestResult_FW_Rawdata = RawDataTest_SinglePoint_Sub(RawData_FW_Rawdata, RecordResult_FW_Rawdata,
				ts->x_num, ts->y_num, PS_Config_Lmt_FW_Rawdata_P, PS_Config_Lmt_FW_Rawdata_N);
	}

	if (nvt_read_CC(RawData_FW_CC) != 0) {
		TestResult_FW_CC = 1;
	} else {
		TestResult_FW_CC = RawDataTest_SinglePoint_Sub(RawData_FW_CC, RecordResult_FW_CC, ts->x_num, ts->y_num,
				PS_Config_Lmt_FW_CC_P, PS_Config_Lmt_FW_CC_N);
	}

	if (ts->pen_support) {
		//---Pen FW Rawdata Test---
		if (nvt_read_pen_baseline() != 0) {
			TestResult_Pen_FW_Raw = 1;
		} else {
			TestResult_PenTipX_Raw = RawDataTest_SinglePoint_Sub(RawData_PenTipX_Raw, RecordResult_PenTipX_Raw,
					ts->pen_x_num_x, ts->pen_x_num_y, PS_Config_Lmt_PenTipX_FW_Raw_P, PS_Config_Lmt_PenTipX_FW_Raw_N);
			TestResult_PenTipY_Raw = RawDataTest_SinglePoint_Sub(RawData_PenTipY_Raw, RecordResult_PenTipY_Raw,
					ts->pen_y_num_x, ts->pen_y_num_y, PS_Config_Lmt_PenTipY_FW_Raw_P, PS_Config_Lmt_PenTipY_FW_Raw_N);
			TestResult_PenRingX_Raw = RawDataTest_SinglePoint_Sub(RawData_PenRingX_Raw, RecordResult_PenRingX_Raw,
					ts->pen_x_num_x, ts->pen_x_num_y, PS_Config_Lmt_PenRingX_FW_Raw_P, PS_Config_Lmt_PenRingX_FW_Raw_N);
			TestResult_PenRingY_Raw = RawDataTest_SinglePoint_Sub(RawData_PenRingY_Raw, RecordResult_PenRingY_Raw,
					ts->pen_y_num_x, ts->pen_y_num_y, PS_Config_Lmt_PenRingY_FW_Raw_P, PS_Config_Lmt_PenRingY_FW_Raw_N);

			if ((TestResult_PenTipX_Raw == -1) ||
			    (TestResult_PenTipY_Raw == -1) ||
			    (TestResult_PenRingX_Raw == -1) ||
			    (TestResult_PenRingY_Raw == -1))
				TestResult_Pen_FW_Raw = -1;
			else
				TestResult_Pen_FW_Raw = 0;
		}
	} /* if (ts->pen_support) */

	//---Leave Test Mode---
	nvt_change_mode(NORMAL_MODE);

	//---Noise Test---
	if (nvt_read_fw_noise(RawData_Diff) != 0) {
		TestResult_Noise = 1; // 1: ERROR
		TestResult_FW_DiffMax = 1;
		TestResult_FW_DiffMin = 1;
		if (ts->pen_support) {
			TestResult_Pen_Noise = 1;
			TestResult_PenTipX_DiffMax = 1;
			TestResult_PenTipX_DiffMin = 1;
			TestResult_PenTipY_DiffMax = 1;
			TestResult_PenTipY_DiffMin = 1;
			TestResult_PenRingX_DiffMax = 1;
			TestResult_PenRingX_DiffMin = 1;
			TestResult_PenRingY_DiffMax = 1;
			TestResult_PenRingY_DiffMin = 1;
		} /* if (ts->pen_support) */
	} else {
		TestResult_FW_DiffMax = RawDataTest_SinglePoint_Sub(RawData_Diff_Max, RecordResult_FW_DiffMax, ts->x_num,
				ts->y_num, PS_Config_Lmt_FW_Diff_P, PS_Config_Lmt_FW_Diff_N);

		TestResult_FW_DiffMin = RawDataTest_SinglePoint_Sub(RawData_Diff_Min, RecordResult_FW_DiffMin, ts->x_num,
				ts->y_num, PS_Config_Lmt_FW_Diff_P, PS_Config_Lmt_FW_Diff_N);

		if ((TestResult_FW_DiffMax == -1) || (TestResult_FW_DiffMin == -1))
			TestResult_Noise = -1;
		else
			TestResult_Noise = 0;

		if (ts->pen_support) {
			TestResult_PenTipX_DiffMax = RawDataTest_SinglePoint_Sub(RawData_PenTipX_DiffMax, RecordResult_PenTipX_DiffMax,
					ts->pen_x_num_x, ts->pen_x_num_y, PS_Config_Lmt_PenTipX_FW_Diff_P, PS_Config_Lmt_PenTipX_FW_Diff_N);

			TestResult_PenTipX_DiffMin = RawDataTest_SinglePoint_Sub(RawData_PenTipX_DiffMin, RecordResult_PenTipX_DiffMin,
						ts->pen_x_num_x, ts->pen_x_num_y, PS_Config_Lmt_PenTipX_FW_Diff_P, PS_Config_Lmt_PenTipX_FW_Diff_N);

			TestResult_PenTipY_DiffMax = RawDataTest_SinglePoint_Sub(RawData_PenTipY_DiffMax, RecordResult_PenTipY_DiffMax,
						ts->pen_y_num_x, ts->pen_y_num_y, PS_Config_Lmt_PenTipY_FW_Diff_P, PS_Config_Lmt_PenTipY_FW_Diff_N);

			TestResult_PenTipY_DiffMin = RawDataTest_SinglePoint_Sub(RawData_PenTipY_DiffMin, RecordResult_PenTipY_DiffMin,
						ts->pen_y_num_x, ts->pen_y_num_y, PS_Config_Lmt_PenTipY_FW_Diff_P, PS_Config_Lmt_PenTipY_FW_Diff_N);

			TestResult_PenRingX_DiffMax = RawDataTest_SinglePoint_Sub(RawData_PenRingX_DiffMax, RecordResult_PenRingX_DiffMax,
						ts->pen_x_num_x, ts->pen_x_num_y, PS_Config_Lmt_PenRingX_FW_Diff_P, PS_Config_Lmt_PenRingX_FW_Diff_N);

			TestResult_PenRingX_DiffMin = RawDataTest_SinglePoint_Sub(RawData_PenRingX_DiffMin, RecordResult_PenRingX_DiffMin,
						ts->pen_x_num_x, ts->pen_x_num_y, PS_Config_Lmt_PenRingX_FW_Diff_P, PS_Config_Lmt_PenRingX_FW_Diff_N);

			TestResult_PenRingY_DiffMax = RawDataTest_SinglePoint_Sub(RawData_PenRingY_DiffMax, RecordResult_PenRingY_DiffMax,
						ts->pen_y_num_x, ts->pen_y_num_y, PS_Config_Lmt_PenRingY_FW_Diff_P, PS_Config_Lmt_PenRingY_FW_Diff_N);

			TestResult_PenRingY_DiffMin = RawDataTest_SinglePoint_Sub(RawData_PenRingY_DiffMin, RecordResult_PenRingY_DiffMin,
						ts->pen_y_num_x, ts->pen_y_num_y, PS_Config_Lmt_PenRingY_FW_Diff_P, PS_Config_Lmt_PenRingY_FW_Diff_N);

			if ((TestResult_PenTipX_DiffMax == -1) ||
			    (TestResult_PenTipX_DiffMin == -1) ||
			    (TestResult_PenTipY_DiffMax == -1) ||
			    (TestResult_PenTipY_DiffMin == -1) ||
			    (TestResult_PenRingX_DiffMax == -1) ||
			    (TestResult_PenRingX_DiffMin == -1) ||
			    (TestResult_PenRingY_DiffMax == -1) ||
			    (TestResult_PenRingY_DiffMin == -1))
				TestResult_Pen_Noise = -1;
			else
				TestResult_Pen_Noise = 0;
		} /* if (ts->pen_support) */
	}

	//--Short Test---
	if (nvt_read_fw_short(RawData_Short) != 0) {
		TestResult_Short = 1; // 1:ERROR
	} else {
		//---Self Test Check --- // 0:PASS, -1:FAIL
		TestResult_Short = RawDataTest_SinglePoint_Sub(RawData_Short, RecordResult_Short, ts->x_num, ts->y_num,
				PS_Config_Lmt_Short_Rawdata_P, PS_Config_Lmt_Short_Rawdata_N);
	}

	//---Open Test---
	if (nvt_read_fw_open(RawData_Open) != 0) {
		TestResult_Open = 1; // 1:ERROR
	} else {
		//---Self Test Check --- // 0:PASS, -1:FAIL
		TestResult_Open = RawDataTest_SinglePoint_Sub(RawData_Open, RecordResult_Open, ts->x_num, ts->y_num,
				PS_Config_Lmt_Open_Rawdata_P, PS_Config_Lmt_Open_Rawdata_N);
	}

	//---Reset IC---
	nvt_bootloader_reset();

	mutex_unlock(&ts->lock);

	NVT_LOG("leave\n");

	nvt_mp_test_result_printed = 0;

	return seq_open(file, &nvt_selftest_seq_ops);
}

#ifdef HAVE_PROC_OPS
static const struct proc_ops nvt_selftest_fops = {
	.proc_open = nvt_selftest_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = seq_release,
};
#else
static const struct file_operations nvt_selftest_fops = {
	.owner = THIS_MODULE,
	.open = nvt_selftest_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};
#endif

#ifdef CONFIG_OF
/*******************************************************
Description:
	Novatek touchscreen parse AIN setting for array type.

return:
	n.a.
*******************************************************/
int32_t nvt_mp_parse_ain(struct device_node *np, const char *name, uint8_t *array, int32_t size)
{
	struct property *data = NULL;
	int32_t len = 0, ret = 0;
	int32_t tmp[50] = {0};
	int32_t i = 0;

	data = of_find_property(np, name, &len);
	len /= sizeof(u32);
	if ((!data) || (!len) || (len != size)) {
		NVT_ERR("error find %s. len=%d\n", name, len);
		return -1;
	} else {
		NVT_LOG("%s. len=%d\n", name, len);
		ret = of_property_read_u32_array(np, name, tmp, len);
		if (ret) {
			NVT_ERR("error reading %s. ret=%d\n", name, ret);
			return -1;
		}

		for (i = 0; i < len; i++)
			array[i] = tmp[i];

#if NVT_DEBUG
		printk("[NVT-ts] %s = ", name);
		nvt_print_result_log_in_one_line(array, len);
		printk("\n");
#endif
	}

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen parse criterion for u32 type.

return:
	n.a.
*******************************************************/
int32_t nvt_mp_parse_u32(struct device_node *np, const char *name, int32_t *para)
{
	int32_t ret = 0;

	ret = of_property_read_u32(np, name, para);
	if (ret) {
		NVT_ERR("error reading %s. ret=%d\n", name, ret);
		return -1;
	} else {
#if NVT_DEBUG
		NVT_LOG("%s=%d\n", name, *para);
#endif
	}

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen parse criterion for array type.

return:
	n.a.
*******************************************************/
int32_t nvt_mp_parse_array(struct device_node *np, const char *name, int32_t *array, int32_t x_ch,
		int32_t y_ch)
{
	struct property *data = NULL;
	int32_t len = 0, ret = 0, size = 0;

	size = x_ch * y_ch;
	data = of_find_property(np, name, &len);
	len /= sizeof(u32);
	if ((!data) || (!len) || (len < size)) {
		NVT_ERR("error find %s. len=%d\n", name, len);
		return -1;
	} else {
		NVT_LOG("%s. len=%d\n", name, len);
		ret = of_property_read_u32_array(np, name, array, len);
		if (ret) {
			NVT_ERR("error reading %s. ret=%d\n", name, ret);
			return -1;
		}

#if NVT_DEBUG
		NVT_LOG("%s =\n", name);
		nvt_print_lmt_array(array, x_ch, y_ch);
#endif
	}

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen parse criterion for pen array type.

return:
	n.a.
*******************************************************/
int32_t nvt_mp_parse_pen_array(struct device_node *np, const char *name, int32_t *array,
		uint32_t x_num, uint32_t y_num)
{
	struct property *data = NULL;
	int32_t len = 0, ret = 0;
#if NVT_DEBUG
	int32_t j = 0;
#endif
	uint32_t size = 0;

	size = x_num * y_num;
	data = of_find_property(np, name, &len);
	len /= sizeof(u32);
	if ((!data) || (!len) || (len < size)) {
		NVT_ERR("error find %s. len=%d\n", name, len);
		return -1;
	} else {
		NVT_LOG("%s. len=%d\n", name, len);
		ret = of_property_read_u32_array(np, name, array, len);
		if (ret) {
			NVT_ERR("error reading %s. ret=%d\n", name, ret);
			return -1;
		}

#if NVT_DEBUG
		NVT_LOG("%s =\n", name);
		for (j = 0; j < y_num; j++) {
			nvt_print_data_log_in_one_line(array + j * x_num,
						       x_num);
			printk("\n");
		}
#endif
	}

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen parse device tree mp function.

return:
	n.a.
*******************************************************/
int32_t nvt_mp_parse_dt(struct device_node *root, const char *node_compatible)
{
	struct device_node *np = root;
	struct device_node *child = NULL;

	NVT_LOG("Parse mp criteria for node %s\n", node_compatible);

	/* find each MP sub-nodes */
	for_each_child_of_node (root, child) {
		/* find the specified node */
		if (of_device_is_compatible(child, node_compatible)) {
			NVT_LOG("found child node %s\n", node_compatible);
			np = child;
			break;
		}
	}
	if (child == NULL) {
		NVT_ERR("Not found compatible node %s!\n", node_compatible);
		return -1;
	}

	/* MP Criteria */
	if (nvt_mp_parse_array(np, "PS_Config_Lmt_Short_Rawdata_P", PS_Config_Lmt_Short_Rawdata_P,
			ts->x_num, ts->y_num))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_Short_Rawdata_N", PS_Config_Lmt_Short_Rawdata_N,
			ts->x_num, ts->y_num))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_Open_Rawdata_P", PS_Config_Lmt_Open_Rawdata_P,
			ts->x_num, ts->y_num))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_Open_Rawdata_N", PS_Config_Lmt_Open_Rawdata_N,
			ts->x_num, ts->y_num))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_FW_Rawdata_P", PS_Config_Lmt_FW_Rawdata_P,
			ts->x_num, ts->y_num))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_FW_Rawdata_N", PS_Config_Lmt_FW_Rawdata_N,
			ts->x_num, ts->y_num))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_FW_CC_P", PS_Config_Lmt_FW_CC_P,
			ts->x_num, ts->y_num))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_FW_CC_N", PS_Config_Lmt_FW_CC_N,
			ts->x_num, ts->y_num))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_FW_Diff_P", PS_Config_Lmt_FW_Diff_P,
			ts->x_num, ts->y_num))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_FW_Diff_N", PS_Config_Lmt_FW_Diff_N,
			ts->x_num, ts->y_num))
		return -1;

	if (ts->pen_support) {
		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenTipX_FW_Raw_P", PS_Config_Lmt_PenTipX_FW_Raw_P,
				ts->pen_x_num_x, ts->pen_x_num_y))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenTipX_FW_Raw_N", PS_Config_Lmt_PenTipX_FW_Raw_N,
				ts->pen_x_num_x, ts->pen_x_num_y))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenTipY_FW_Raw_P", PS_Config_Lmt_PenTipY_FW_Raw_P,
				ts->pen_y_num_x, ts->pen_y_num_y))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenTipY_FW_Raw_N", PS_Config_Lmt_PenTipY_FW_Raw_N,
				ts->pen_y_num_x, ts->pen_y_num_y))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenRingX_FW_Raw_P", PS_Config_Lmt_PenRingX_FW_Raw_P,
				ts->pen_x_num_x, ts->pen_x_num_y))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenRingX_FW_Raw_N", PS_Config_Lmt_PenRingX_FW_Raw_N,
				ts->pen_x_num_x, ts->pen_x_num_y))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenRingY_FW_Raw_P", PS_Config_Lmt_PenRingY_FW_Raw_P,
				ts->pen_y_num_x, ts->pen_y_num_y))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenRingY_FW_Raw_N", PS_Config_Lmt_PenRingY_FW_Raw_N,
				ts->pen_y_num_x, ts->pen_y_num_y))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenTipX_FW_Diff_P", PS_Config_Lmt_PenTipX_FW_Diff_P,
				ts->pen_x_num_x, ts->pen_x_num_y))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenTipX_FW_Diff_N", PS_Config_Lmt_PenTipX_FW_Diff_N,
			   ts->pen_x_num_x, ts->pen_x_num_y))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenTipY_FW_Diff_P", PS_Config_Lmt_PenTipY_FW_Diff_P,
				ts->pen_y_num_x, ts->pen_y_num_y))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenTipY_FW_Diff_N", PS_Config_Lmt_PenTipY_FW_Diff_N,
				ts->pen_y_num_x, ts->pen_y_num_y))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenRingX_FW_Diff_P", PS_Config_Lmt_PenRingX_FW_Diff_P,
				ts->pen_x_num_x, ts->pen_x_num_y))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenRingX_FW_Diff_N", PS_Config_Lmt_PenRingX_FW_Diff_N,
				ts->pen_x_num_x, ts->pen_x_num_y))
			return -1;

		if (nvt_mp_parse_pen_array(np,
					   "PS_Config_Lmt_PenRingY_FW_Diff_P",
					   PS_Config_Lmt_PenRingY_FW_Diff_P,
					   ts->pen_y_num_x, ts->pen_y_num_y))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenRingY_FW_Diff_N", PS_Config_Lmt_PenRingY_FW_Diff_N,
				ts->pen_y_num_x, ts->pen_y_num_y))
			return -1;
	} /* if (ts->pen_support) */

	if (nvt_mp_parse_u32(np, "PS_Config_Diff_Test_Frame", &PS_Config_Diff_Test_Frame))
		return -1;

	NVT_LOG("Parse mp criteria done!\n");

	return 0;
}
#endif /* #ifdef CONFIG_OF */

/*******************************************************
Description:
	Novatek touchscreen MP function proc. file node
	initial function.

return:
	Executive outcomes. 0---succeed. -1---failed.
*******************************************************/
int32_t nvt_mp_proc_init(void)
{
	if (!NVT_proc_selftest_entry) {
		NVT_proc_selftest_entry = proc_create("nvt_selftest", 0444, NULL, &nvt_selftest_fops);
		if (NVT_proc_selftest_entry == NULL) {
			NVT_ERR("create /proc/nvt_selftest Failed!\n");
			return -1;
		} else {
			if (nvt_mp_buffer_init()) {
				NVT_ERR("Allocate mp memory failed\n");
				return -1;
			} else {
				NVT_LOG("create /proc/nvt_selftest Succeeded!\n");
			}
		}
	} else {
		NVT_LOG("/proc/nvt_selftest is already created\n");
	}
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen MP function proc. file node
	deinitial function.

return:
	n.a.
*******************************************************/
void nvt_mp_proc_deinit(void)
{
	nvt_mp_buffer_deinit();

	if (NVT_proc_selftest_entry != NULL) {
		remove_proc_entry("nvt_selftest", NULL);
		NVT_proc_selftest_entry = NULL;
		NVT_LOG("Removed /proc/%s\n", "nvt_selftest");
	}
}
