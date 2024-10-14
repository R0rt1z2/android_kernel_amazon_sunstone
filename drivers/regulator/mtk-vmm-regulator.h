/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _ISP_DVFS_H
#define _ISP_DVFS_H
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include "internal.h"

#define CMD_UPDATE_DVFS_INFO 1
#define MAX_MUX_NUM (10)
#define MAX_OPP_STEP 7
#define DEFAULT_VOLTAGE 600000
#define VMM_MAX_VOLTAGE 725000


struct dvfs_clk_data {
	const char *mux_name;
	struct clk *mux;
	struct clk *clk_src[MAX_OPP_STEP];
};

struct dvfs_table {
	unsigned long frequency[MAX_OPP_STEP];
	int voltage[MAX_OPP_STEP];
	int opp_num;
};

struct dvfs_info {
	struct mutex voltage_mutex;
	int voltage_target;
	u32 opp_level;
};

struct dvfs_driver_data {
	struct device *dev;
	u32 num_muxes;
	struct dvfs_clk_data muxes[MAX_MUX_NUM];
	struct dvfs_table opp_table;
	struct dvfs_info current_dvfs;
	u32 support_aging;
};

struct dvfs_ipc_info {
	int voltage;
};

int isp_apmcu_set_voltage(struct regulator *reg, int min_uV, int max_uV);

#endif
