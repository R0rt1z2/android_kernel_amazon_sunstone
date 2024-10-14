/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _EAS_PLUS_H
#define _EAS_PLUS_H

#define MIGR_IDLE_BALANCE               1
#define MIGR_IDLE_PULL_MISFIT_RUNNING   2
#define MIGR_TICK_PULL_MISFIT_RUNNING   3

#ifdef CONFIG_SMP
/*
 * The margin used when comparing utilization with CPU capacity.
 *
 * (default: ~20%)
 */
#define fits_capacity(cap, max) ((cap) * 1280 < (max) * 1024)
unsigned long capacity_of(int cpu);
#endif

extern unsigned long cpu_util(int cpu);
extern int mtk_static_power_init(void);
extern int task_fits_capacity(struct task_struct *p, long capacity);

#if IS_ENABLED(CONFIG_MTK_EAS)
extern void mtk_find_busiest_group(void *data, struct sched_group *busiest,
		struct rq *dst_rq, int *out_balance);
extern void mtk_find_energy_efficient_cpu(void *data, struct task_struct *p,
		int prev_cpu, int sync, int *new_cpu);
extern void mtk_cpu_overutilized(void *data, int cpu, int *overutilized);
extern void mtk_em_cpu_energy(void *data, struct em_perf_domain *pd,
		unsigned long max_util, unsigned long sum_util, unsigned long *energy);
extern unsigned int mtk_get_leakage(unsigned int cpu, unsigned int opp, unsigned int temperature);
#if IS_ENABLED(CONFIG_MTK_THERMAL_AWARE_SCHEDULING)
extern int sort_thermal_headroom(struct cpumask *cpus, int *cpu_order);
#endif

extern int init_sram_info(void);
extern void mtk_tick_entry(void *data, struct rq *rq);
extern void mtk_set_wake_flags(void *data, int *wake_flags, unsigned int *mode);

#if IS_ENABLED(CONFIG_UCLAMP_TASK_GROUP)
extern void mtk_uclamp_eff_get(void *data, struct task_struct *p, enum uclamp_id clamp_id,
		struct uclamp_se *uc_max, struct uclamp_se *uc_eff, int *ret);
#endif
#if IS_ENABLED(CONFIG_MTK_NEWIDLE_BALANCE)
extern void mtk_sched_newidle_balance(void *data, struct rq *this_rq,
		struct rq_flags *rf, int *pulled_task, int *done);
#endif
#endif

extern int migrate_running_task(int this_cpu, struct task_struct *p, struct rq *target,
		int reason);
extern void hook_scheduler_tick(void *data, struct rq *rq);
#if IS_ENABLED(CONFIG_MTK_SCHED_BIG_TASK_ROTATE)
extern void task_check_for_rotation(struct rq *src_rq);
extern void rotat_after_enqueue_task(void *data, struct rq *rq, struct task_struct *p);
extern void rotat_task_stats(void *data, struct task_struct *p);
extern void rotat_task_newtask(void __always_unused *data, struct task_struct *p,
				unsigned long clone_flags);
#endif
#endif
