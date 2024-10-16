/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 MediaTek Inc.
 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM scheduler

#if !defined(_TRACE_SCHEDULER_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_SCHEDULER_H
#include <linux/string.h>
#include <linux/types.h>
#include <linux/tracepoint.h>

#define LB_FAIL                   (0x01)
#define LB_SYNC                   (0x02)
#define LB_ZERO_UTIL              (0x04)
#define LB_PREV                   (0x08)
#define LB_LATENCY_SENSITIVE      (0x10)
#define LB_NOT_PREV               (0x20)
#define LB_BEST_ENERGY_CPU        (0x40)

#if IS_ENABLED(CONFIG_MTK_SCHED_BIG_TASK_ROTATE)
/*
 * Tracepoint for big task rotation
 */
TRACE_EVENT(sched_big_task_rotation,

	TP_PROTO(int src_cpu, int dst_cpu, int src_pid, int dst_pid,
		int fin),

	TP_ARGS(src_cpu, dst_cpu, src_pid, dst_pid, fin),

	TP_STRUCT__entry(
		__field(int, src_cpu)
		__field(int, dst_cpu)
		__field(int, src_pid)
		__field(int, dst_pid)
		__field(int, fin)
	),

	TP_fast_assign(
		__entry->src_cpu	= src_cpu;
		__entry->dst_cpu	= dst_cpu;
		__entry->src_pid	= src_pid;
		__entry->dst_pid	= dst_pid;
		__entry->fin		= fin;
	),

	TP_printk("src_cpu=%d dst_cpu=%d src_pid=%d dst_pid=%d fin=%d",
		__entry->src_cpu, __entry->dst_cpu,
		__entry->src_pid, __entry->dst_pid,
		__entry->fin)
);
#endif

TRACE_EVENT(sched_leakage,

	TP_PROTO(int cpu, int opp, unsigned int temp,
		unsigned long cpu_static_pwr, unsigned long static_pwr),

	TP_ARGS(cpu, opp, temp, cpu_static_pwr, static_pwr),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(int, opp)
		__field(unsigned int, temp)
		__field(unsigned long, cpu_static_pwr)
		__field(unsigned long, static_pwr)
		),

	TP_fast_assign(
		__entry->cpu       = cpu;
		__entry->opp        = opp;
		__entry->temp       = temp;
		__entry->cpu_static_pwr = cpu_static_pwr;
		__entry->static_pwr = static_pwr;
		),

	TP_printk("cpu=%d opp=%d temp=%lu lkg=%lu sum_lkg=%lu",
		__entry->cpu,
		__entry->opp,
		__entry->temp,
		__entry->cpu_static_pwr,
		__entry->static_pwr)
);

TRACE_EVENT(sched_em_cpu_energy,

	TP_PROTO(int opp,
		unsigned long freq, unsigned long cost, unsigned long scale_cpu,
		unsigned long dyn_pwr, unsigned long static_pwr),

	TP_ARGS(opp, freq, cost, scale_cpu, dyn_pwr, static_pwr),

	TP_STRUCT__entry(
		__field(int, opp)
		__field(unsigned long, freq)
		__field(unsigned long, cost)
		__field(unsigned long, scale_cpu)
		__field(unsigned long, dyn_pwr)
		__field(unsigned long, static_pwr)
		),

	TP_fast_assign(
		__entry->opp        = opp;
		__entry->freq       = freq;
		__entry->cost       = cost;
		__entry->scale_cpu  = scale_cpu;
		__entry->dyn_pwr    = dyn_pwr;
		__entry->static_pwr = static_pwr;
		),

	TP_printk("opp=%d freq=%lu cost=%lu scale_cpu=%lu dyn_pwr=%lu static_pwr=%lu",
		__entry->opp,
		__entry->freq,
		__entry->cost,
		__entry->scale_cpu,
		__entry->dyn_pwr,
		__entry->static_pwr)
);

TRACE_EVENT(sched_find_busiest_group,

	TP_PROTO(int src_cpu, int dst_cpu,
		int out_balance, int reason),

	TP_ARGS(src_cpu, dst_cpu, out_balance, reason),

	TP_STRUCT__entry(
		__field(int, src_cpu)
		__field(int, dst_cpu)
		__field(int, out_balance)
		__field(int, reason)
		),

	TP_fast_assign(
		__entry->src_cpu	= src_cpu;
		__entry->dst_cpu	= dst_cpu;
		__entry->out_balance	= out_balance;
		__entry->reason		= reason;
		),

	TP_printk("src_cpu=%d dst_cpu=%d out_balance=%d reason=0x%x",
		__entry->src_cpu,
		__entry->dst_cpu,
		__entry->out_balance,
		__entry->reason)
);

TRACE_EVENT(sched_cpu_overutilized,

	TP_PROTO(int cpu, struct cpumask *pd_mask,
		unsigned long sum_util, unsigned long sum_cap,
		int overutilized),

	TP_ARGS(cpu, pd_mask, sum_util, sum_cap,
		overutilized),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(long, cpu_mask)
		__field(unsigned long, sum_util)
		__field(unsigned long, sum_cap)
		__field(int, overutilized)
		),

	TP_fast_assign(
		__entry->cpu		= cpu;
		__entry->cpu_mask	= pd_mask->bits[0];
		__entry->sum_util	= sum_util;
		__entry->sum_cap	= sum_cap;
		__entry->overutilized	= overutilized;
		),

	TP_printk("cpu=%d mask=0x%lx sum_util=%lu sum_cap=%lu overutilized=%d",
		__entry->cpu,
		__entry->cpu_mask,
		__entry->sum_util,
		__entry->sum_cap,
		__entry->overutilized)
);

/*
 * Tracepoint for task force migrations.
 */
TRACE_EVENT(sched_frequency_limits,

	TP_PROTO(int cpu_id, int freq_thermal, int freq_max, int freq_ceiling),

	TP_ARGS(cpu_id, freq_thermal, freq_max, freq_ceiling),

	TP_STRUCT__entry(
		__field(int,  cpu_id)
		__field(int,  freq_thermal)
		__field(int,  freq_max)
		__field(int,  freq_ceiling)
		),

	TP_fast_assign(
		__entry->cpu_id = cpu_id;
		__entry->freq_thermal = freq_thermal;
		__entry->freq_max = freq_max;
		__entry->freq_ceiling = freq_ceiling;
		),

	TP_printk("cpu=%d thermal=%lu cpu_freq_max=%lu sched_ceiling=%lu",
		__entry->cpu_id, __entry->freq_thermal, __entry->freq_max, __entry->freq_ceiling)
);

TRACE_EVENT(sched_queue_task,
	TP_PROTO(int cpu, int pid, int enqueue,
		unsigned long cfs_util,
		unsigned int min, unsigned int max,
		unsigned int task_min, unsigned int task_max),
	TP_ARGS(cpu, pid, enqueue, cfs_util, min, max, task_min, task_max),
	TP_STRUCT__entry(
		__field(int, cpu)
		__field(int, pid)
		__field(int, enqueue)
		__field(unsigned long, cfs_util)
		__field(unsigned int, min)
		__field(unsigned int, max)
		__field(unsigned int, task_min)
		__field(unsigned int, task_max)
	),
	TP_fast_assign(
		__entry->cpu = cpu;
		__entry->pid = pid;
		__entry->enqueue = enqueue;
		__entry->cfs_util = cfs_util;
		__entry->min = min;
		__entry->max = max;
		__entry->task_min = task_min;
		__entry->task_max = task_max;
	),
	TP_printk(
		"cpu=%d pid=%d enqueue=%d cfs_util=%lu min=%u max=%u task_min=%u task_max=%u",
		__entry->cpu,
		__entry->pid,
		__entry->enqueue,
		__entry->cfs_util,
		__entry->min,
		__entry->max,
		__entry->task_min,
		__entry->task_max)
);

TRACE_EVENT(sched_task_util,
	TP_PROTO(int pid, char *comm,
		unsigned long util,
		unsigned int util_enqueued, unsigned int util_ewma),
	TP_ARGS(pid, comm, util, util_enqueued, util_ewma),
	TP_STRUCT__entry(
		__field(int, pid)
		__field(char *, comm)
		__field(unsigned long, util)
		__field(unsigned int, util_enqueued)
		__field(unsigned int, util_ewma)
	),
	TP_fast_assign(
		__entry->pid = pid;
		__entry->comm = comm;
		__entry->util = util;
		__entry->util_enqueued = util_enqueued;
		__entry->util_ewma = util_ewma;
	),
	TP_printk(
		"pid=%d comm=%s util=%lu util_enqueued=%u util_ewma=%u",
		__entry->pid,
		__entry->comm,
		__entry->util,
		__entry->util_enqueued,
		__entry->util_ewma)
);

TRACE_EVENT(sched_task_uclamp,
	TP_PROTO(int pid, unsigned long util,
		unsigned int active,
		unsigned int min, unsigned int max,
		unsigned int min_ud, unsigned int min_req,
		unsigned int max_ud, unsigned int max_req),
	TP_ARGS(pid, util, active,
		min, max,
		min_ud, min_req,
		max_ud, max_req),
	TP_STRUCT__entry(
		__field(int, pid)
		__field(unsigned long, util)
		__field(unsigned int, active)
		__field(unsigned int, min)
		__field(unsigned int, max)
		__field(unsigned int, min_ud)
		__field(unsigned int, min_req)
		__field(unsigned int, max_ud)
		__field(unsigned int, max_req)
	),
	TP_fast_assign(
		__entry->pid = pid;
		__entry->util = util;
		__entry->active = active;
		__entry->min = min;
		__entry->max = max;
		__entry->min_ud = min_ud;
		__entry->min_req = min_req;
		__entry->max_ud = max_ud;
		__entry->max_req = max_req;
	),
	TP_printk(
		"pid=%d util=%lu active=%u min=%u max=%u min_ud=%u min_req=%u max_ud=%u max_req=%u",
		__entry->pid,
		__entry->util,
		__entry->active,
		__entry->min,
		__entry->max,
		__entry->min_ud,
		__entry->min_req,
		__entry->max_ud,
		__entry->max_req)
);

#endif /* _TRACE_SCHEDULER_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH eas
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE eas_trace
/* This part must be outside protection */
#include <trace/define_trace.h>
