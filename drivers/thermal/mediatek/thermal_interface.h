/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __THERMAL_INTERFACE_H__
#define __THERMAL_INTERFACE_H__

#define CPU_TEMP_OFFSET             (0)
#define CPU_HEADROOM_OFFSET         (0x20)
#define CPU_HEADROOM_RATIO_OFFSET   (0x40)
#define CPU_PREDICT_TEMP_OFFSET     (0x60)
#define AP_NTC_HEADROOM_OFFSET      (0x80)
#define TPCB_OFFSET                 (0x84)
#define TARGET_TPCB_OFFSET          (0x88)
#define TTJ_OFFSET                 (0x100)
#define POWER_BUDGET_OFFSET        (0x110)
#define CPU_MIN_OPP_HINT_OFFSET    (0x120)
#define CPU_ACTIVE_BITMASK_OFFSET  (0x130)
#define GPU_TEMP_OFFSET            (0x180)
#define APU_TEMP_OFFSET            (0x190)
#define EMUL_TEMP_OFFSET           (0x1B0)
#define CPU_LIMIT_OPP_OFFSET       (0x200)
#define CPU_CUR_OPP_OFFSET         (0x210)
#define CPU_MAX_TEMP_OFFSET        (0x220)
#define ATC_OFFSET                 (0x280)
#define ATC_NUM                    (9)

#define APU_MBOX_TTJ_OFFSET        (0x700)
#define APU_MBOX_PB_OFFSET         (0x704)
#define APU_MBOX_TEMP_OFFSET       (0x708)
#define APU_MBOX_LIMIT_OPP_OFFSET  (0x70C)
#define APU_MBOX_CUR_OPP_OFFSET    (0x710)
#define APU_MBOX_EMUL_TEMP_OFFSET  (0x714)

struct headroom_info {
    int temp;
    int predict_temp;
    int headroom;
    int ratio;
};
enum headroom_id {
	/* SoC Tj */
	SOC_CPU0,
	SOC_CPU1,
	SOC_CPU2,
	SOC_CPU3,
	SOC_CPU4,
	SOC_CPU5,
	SOC_CPU6,
	SOC_CPU7,
	/* PCB */
	PCB_AP,

	NR_HEADROOM_ID
};

struct fps_cooler_info {
	int activated;
	int target_fps;
	int tpcb;
	int tpcb_slope;
	int ap_headroom;
	int n_sec_to_ttpcb;
};

#define MAX_MD_NAME_LENGTH  (20)

struct md_thermal_sensor_t {
	int id;
	char sensor_name[MAX_MD_NAME_LENGTH];
	int cur_temp;
};

struct md_thermal_actuator_t {
	int id;
	char actuator_name[MAX_MD_NAME_LENGTH];
	int cur_status;
	int max_status;
};

struct md_info {
	int sensor_num;
	struct md_thermal_sensor_t *sensor_info;
	int md_autonomous_ctrl;
	int actuator_num;
	struct md_thermal_actuator_t *actuator_info;
};

extern void update_ap_ntc_headroom(int temp, int polling_interval);
extern int get_thermal_headroom(enum headroom_id id);
extern int set_cpu_min_opp(int gear, int opp);
extern int set_cpu_active_bitmask(int mask);
extern int get_cpu_temp(int cpu_id);
#if IS_ENABLED(CONFIG_MTK_THERMAL_INTERFACE)
extern void __iomem *thermal_csram_base;
extern void __iomem *thermal_apu_mbox_base;
extern struct fps_cooler_info fps_cooler_data;
#else
void __iomem *thermal_csram_base;
void __iomem *thermal_apu_mbox_base;
struct fps_cooler_info fps_cooler_data;
#endif
#endif
