/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_GPUFREQ_INTERNAL_H__
#define __MTK_GPUFREQ_INTERNAL_H__

#if defined(CONFIG_GPU_MT6885)
#include "mt6885/mtk_gpufreq_internal_plat.h"

#elif defined(CONFIG_GPU_MT8195)
#include "mt8195/mtk_gpufreq_internal_plat.h"

#elif defined(CONFIG_GPU_MT8188)
#include "mt8188/mtk_gpufreq_internal_plat.h"

#elif defined(CONFIG_GPU_MT6853)
#include "mt6853/mtk_gpufreq_internal_plat.h"
#endif

#endif /* __MTK_GPUFREQ_INTERNAL_H__ */

