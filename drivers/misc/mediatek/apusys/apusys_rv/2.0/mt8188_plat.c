// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/sched/clock.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/kernel.h>

#include "mt-plat/aee.h"

#include "apusys_power.h"
#include "apusys_secure.h"
#include "../apu.h"
#include "../apu_debug.h"
#include "../apu_config.h"
#include "../apu_excep.h"

/* for IPI IRQ affinity tuning*/
static struct cpumask perf_cpus, normal_cpus;

static uint32_t apusys_rv_smc_call(struct device *dev, uint32_t smc_id,
	uint32_t a2)
{
#if APUSYS_SECURE
	struct arm_smccc_res res;

	dev_info(dev, "%s: smc call %d\n",
			__func__, smc_id);

	arm_smccc_smc(MTK_SIP_APUSYS_CONTROL, smc_id,
				a2, 0, 0, 0, 0, 0, &res);
	if (((int) res.a0) < 0)
		dev_info(dev, "%s: smc call %d return error(%ld)\n",
			__func__,
			smc_id, res.a0);

	return res.a0;
#else
	return 0;
#endif
}

static int mt8188_rproc_init(struct mtk_apu *apu)
{
	return 0;
}

static int mt8188_rproc_exit(struct mtk_apu *apu)
{
	return 0;
}

static void apu_setup_sec_mem(struct mtk_apu *apu)
{
	int32_t ret;
	struct device *dev = apu->dev;

	if (!(apu->platdata->flags & F_SECURE_BOOT))
		return;

	ret = apusys_rv_smc_call(dev,
		MTK_APUSYS_KERNEL_OP_APUSYS_SETUP_SECURE_MEM, 0);
	dev_info(dev, "%s: %d\n", __func__, ret);
}

static void apu_setup_reviser(struct mtk_apu *apu, int boundary, int ns, int domain)
{
	struct device *dev = apu->dev;
	struct mtk_apu_reg_ofs *reg_ofs = &apu->platdata->ofs;
	unsigned long flags;

	if (apu->platdata->flags & F_SECURE_BOOT) {
		apusys_rv_smc_call(dev,
			MTK_APUSYS_KERNEL_OP_APUSYS_RV_SETUP_REVISER, 0);
	} else {
		spin_lock_irqsave(&apu->reg_lock, flags);
		/* setup boundary */
		iowrite32(0x4 | boundary,
			apu->apu_sctrl_reviser + reg_ofs->userfw_ctxt);
		iowrite32(0x4 | boundary,
			apu->apu_sctrl_reviser + reg_ofs->secfw_ctxt);

		/* setup iommu ctrl(mmu_ctrl | mmu_en) */
		if (apu->platdata->flags & F_BYPASS_IOMMU)
			iowrite32(0x2,
				  apu->apu_sctrl_reviser + reg_ofs->iommu_ctrl);
		else
			iowrite32(0x3,
				  apu->apu_sctrl_reviser + reg_ofs->iommu_ctrl);

		/* setup ns/domain */
		iowrite32(ns << 4 | domain,
			apu->apu_sctrl_reviser + reg_ofs->normal_dns);
		iowrite32(ns << 4 | domain,
			apu->apu_sctrl_reviser + reg_ofs->pri_dns);
		spin_unlock_irqrestore(&apu->reg_lock, flags);

		apu_drv_debug("%s: UP_IOMMU_CTRL = 0x%x\n",
			__func__,
			ioread32(apu->apu_sctrl_reviser + reg_ofs->iommu_ctrl));
		apu_drv_debug("%s: UP_NORMAL_DOMAIN_NS = 0x%x\n",
			__func__,
			ioread32(apu->apu_sctrl_reviser + reg_ofs->normal_dns));
		apu_drv_debug("%s: UP_PRI_DOMAIN_NS = 0x%x\n",
			__func__,
			ioread32(apu->apu_sctrl_reviser + reg_ofs->pri_dns));
		apu_drv_debug("%s: USERFW_CTXT = 0x%x\n",
			__func__,
			ioread32(apu->apu_sctrl_reviser + reg_ofs->userfw_ctxt));
		apu_drv_debug("%s: SECUREFW_CTXT = 0x%x\n",
			__func__,
			ioread32(apu->apu_sctrl_reviser + reg_ofs->secfw_ctxt));

		if ((apu->platdata->flags & F_BYPASS_IOMMU) ||
			(apu->platdata->flags & F_PRELOAD_FIRMWARE)) {
			spin_lock_irqsave(&apu->reg_lock, flags);
			/* vld=1, partial_enable=1 */
			iowrite32(0x7,
				apu->apu_sctrl_reviser + reg_ofs->core0_vabase0);
			/* for 34 bit mva */
			iowrite32(0x1 | (u32) (apu->code_da >> 2),
				apu->apu_sctrl_reviser + reg_ofs->core0_mvabase0);

			/* vld=1, partial_enable=1 */
			iowrite32(0x3,
				apu->apu_sctrl_reviser + reg_ofs->core0_vabase1);
			/* for 34 bit mva */
			iowrite32(0x1 | (u32) (apu->code_da >> 2),
				apu->apu_sctrl_reviser +
				reg_ofs->core0_mvabase1);
			spin_unlock_irqrestore(&apu->reg_lock, flags);

			apu_drv_debug("%s: UP_CORE0_VABASE0 = 0x%x\n",
				__func__,
				ioread32(apu->apu_sctrl_reviser +
					 reg_ofs->core0_vabase0));
			apu_drv_debug("%s: UP_CORE0_MVABASE0 = 0x%x\n",
				__func__,
				ioread32(apu->apu_sctrl_reviser +
					 reg_ofs->core0_mvabase0));
			apu_drv_debug("%s: UP_CORE0_VABASE1 = 0x%x\n",
				__func__,
				ioread32(apu->apu_sctrl_reviser +
					 reg_ofs->core0_vabase1));
			apu_drv_debug("%s: UP_CORE0_MVABASE1 = 0x%x\n",
				__func__,
				ioread32(apu->apu_sctrl_reviser +
					 reg_ofs->core0_mvabase1));
		}
	}
}

static void apu_setup_devapc(struct mtk_apu *apu)
{
	int32_t ret;
	struct device *dev = apu->dev;

	ret = (int32_t)apusys_rv_smc_call(dev,
		MTK_APUSYS_KERNEL_OP_DEVAPC_INIT_RCX, 0);

	dev_info(dev, "%s: %d\n", __func__, ret);
}

static void apu_reset_mp(struct mtk_apu *apu)
{
	struct device *dev = apu->dev;
	struct mtk_apu_reg_ofs *reg_ofs = &apu->platdata->ofs;
	unsigned long flags;

	if (apu->platdata->flags & F_SECURE_BOOT) {
		apusys_rv_smc_call(dev,
			MTK_APUSYS_KERNEL_OP_APUSYS_RV_RESET_MP, 0);
	} else {
		spin_lock_irqsave(&apu->reg_lock, flags);
		/* reset uP */
		iowrite32(0, apu->md32_sysctrl + reg_ofs->md32_sys_ctrl);
		spin_unlock_irqrestore(&apu->reg_lock, flags);

		udelay(10);

		spin_lock_irqsave(&apu->reg_lock, flags);
		/* md32_g2b_cg_en | md32_dbg_en | md32_soft_rstn */
		iowrite32(0xc01, apu->md32_sysctrl + reg_ofs->md32_sys_ctrl);
		spin_unlock_irqrestore(&apu->reg_lock, flags);
		apu_drv_debug("%s: MD32_SYS_CTRL = 0x%x\n",
			__func__,
			ioread32(apu->md32_sysctrl + reg_ofs->md32_sys_ctrl));

		spin_lock_irqsave(&apu->reg_lock, flags);
		/* md32 clk enable */
		iowrite32(0x1, apu->md32_sysctrl + reg_ofs->md32_clk_en);
		/* set up_wake_host_mask0 for wdt/mbox irq */
		iowrite32(0x1c0001, apu->md32_sysctrl +
				    reg_ofs->up_wake_host_mask0);
		spin_unlock_irqrestore(&apu->reg_lock, flags);
		apu_drv_debug("%s: MD32_CLK_EN = 0x%x\n",
			__func__,
			ioread32(apu->md32_sysctrl + reg_ofs->md32_clk_en));
		apu_drv_debug("%s: UP_WAKE_HOST_MASK0 = 0x%x\n",
			__func__, ioread32(apu->md32_sysctrl +
					   reg_ofs->up_wake_host_mask0));
	}
}

static void apu_setup_boot(struct mtk_apu *apu)
{
	struct device *dev = apu->dev;
	struct mtk_apu_reg_ofs *reg_ofs = &apu->platdata->ofs;
	unsigned long flags;
	int boot_from_tcm;

	if (TCM_OFFSET == 0)
		boot_from_tcm = 1;
	else
		boot_from_tcm = 0;

	if (apu->platdata->flags & F_SECURE_BOOT) {
		apusys_rv_smc_call(dev,
			MTK_APUSYS_KERNEL_OP_APUSYS_RV_SETUP_BOOT, 0);
	} else {
		/* Set uP boot addr to DRAM.
		 * If boot from tcm == 1, boot addr will always map to
		 * 0x1d000000 no matter what value boot_addr is
		 */
		spin_lock_irqsave(&apu->reg_lock, flags);
		if ((apu->platdata->flags & F_BYPASS_IOMMU) ||
			(apu->platdata->flags & F_PRELOAD_FIRMWARE))
			iowrite32((u32)apu->code_da,
				apu->apu_ao_ctl + reg_ofs->md32_boot_ctrl);
		else
			iowrite32((u32)CODE_BUF_DA | boot_from_tcm,
				apu->apu_ao_ctl + reg_ofs->md32_boot_ctrl);
		spin_unlock_irqrestore(&apu->reg_lock, flags);
		apu_drv_debug("%s: MD32_BOOT_CTRL = 0x%x\n",
			__func__,
			ioread32(apu->apu_ao_ctl + reg_ofs->md32_boot_ctrl));

		spin_lock_irqsave(&apu->reg_lock, flags);
		/* set predefined MPU region for cache access */
		iowrite32(0xAB, apu->apu_ao_ctl + reg_ofs->md32_pre_def);
		spin_unlock_irqrestore(&apu->reg_lock, flags);
		apu_drv_debug("%s: MD32_PRE_DEFINE = 0x%x\n",
			__func__,
			ioread32(apu->apu_ao_ctl + reg_ofs->md32_pre_def));
	}
}

static void apu_start_mp(struct mtk_apu *apu)
{
	struct device *dev = apu->dev;
	struct mtk_apu_reg_ofs *reg_ofs = &apu->platdata->ofs;
	int i;
	unsigned long flags;

	if (apu->platdata->flags & F_SECURE_BOOT) {
		apusys_rv_smc_call(dev,
			MTK_APUSYS_KERNEL_OP_APUSYS_RV_START_MP, 0);
	} else {
		spin_lock_irqsave(&apu->reg_lock, flags);
		/* Release runstall */
		iowrite32(0x0, apu->apu_ao_ctl + reg_ofs->md32_runstall);
		spin_unlock_irqrestore(&apu->reg_lock, flags);

		if ((apu->platdata->flags & F_SECURE_BOOT) == 0)
			for (i = 0; i < 20; i++) {
				dev_info(dev, "apu boot: pc=%08x, sp=%08x\n",
					ioread32(apu->md32_sysctrl + 0x838),
					ioread32(apu->md32_sysctrl+0x840));
				usleep_range(0, 20);
			}
	}
}

static int mt8188_rproc_start(struct mtk_apu *apu)
{
	int ns = 1; /* Non Secure */
	int domain = 0;
	int boundary = (u32) upper_32_bits(apu->code_da);

	apu_setup_sec_mem(apu);

	apu_setup_devapc(apu);

	apu_setup_reviser(apu, boundary, ns, domain);

	apu_reset_mp(apu);

	apu_setup_boot(apu);

	apu_start_mp(apu);

	return 0;
}

static int mt8188_rproc_stop(struct mtk_apu *apu)
{
	struct device *dev = apu->dev;

	/* Hold runstall */
	if (apu->platdata->flags & F_SECURE_BOOT)
		apusys_rv_smc_call(dev,
			MTK_APUSYS_KERNEL_OP_APUSYS_RV_STOP_MP, 0);
	else
		iowrite32(0x1, apu->apu_ao_ctl + 8);

	return 0;
}

static int mt8188_apu_power_init(struct mtk_apu *apu)
{
	struct device *dev = apu->dev;
	struct device_node *np;
	struct platform_device *pdev;

	/* power dev */
	np = of_parse_phandle(dev->of_node, "mediatek,apusys_power", 0);
	if (!np) {
		dev_info(dev, "failed to parse apusys_power node\n");
		return -EINVAL;
	}

	if (!of_device_is_available(np)) {
		dev_info(dev, "unable to find apusys_power node\n");
		of_node_put(np);
		return -ENODEV;
	}

	pdev = of_find_device_by_node(np);
	if (!pdev) {
		dev_info(dev, "apusys_power is not ready yet\n");
		of_node_put(np);
		return -EPROBE_DEFER;
	}

	dev_info(dev, "%s: get power_dev, name=%s\n", __func__, pdev->name);

	apu->power_dev = &pdev->dev;
	of_node_put(np);

	/* apu iommu 0 */
	np = of_parse_phandle(dev->of_node, "apu_iommu0", 0);
	if (!np) {
		dev_info(dev, "failed to parse apu_iommu0 node\n");
		return -EINVAL;
	}
	if (!of_device_is_available(np)) {
		dev_info(dev, "unable to find apu_iommu0 node\n");
		of_node_put(np);
		return -ENODEV;
	}

	pdev = of_find_device_by_node(np);
	if (!pdev) {
		dev_info(dev, "apu_iommu0 is not ready yet\n");
		of_node_put(np);
		return -EPROBE_DEFER;
	}

	dev_info(dev, "%s: get apu_iommu0 device, name=%s\n", __func__, pdev->name);

	apu->apu_iommu0 = &pdev->dev;
	of_node_put(np);

	/* apu iommu 1 */
	np = of_parse_phandle(dev->of_node, "apu_iommu1", 0);
	if (!np) {
		dev_info(dev, "failed to parse apu_iommu1 node\n");
		return -EINVAL;
	}

	if (!of_device_is_available(np)) {
		dev_info(dev, "unable to find apu_iommu1 node\n");
		of_node_put(np);
		return -ENODEV;
	}

	pdev = of_find_device_by_node(np);
	if (!pdev) {
		dev_info(dev, "apu_iommu1 is not ready yet\n");
		of_node_put(np);
		return -EPROBE_DEFER;
	}

	dev_info(dev, "%s: get apu_iommu1 device, name=%s\n", __func__, pdev->name);

	apu->apu_iommu1 = &pdev->dev;
	of_node_put(np);

	return 0;
}

static int mt8188_apu_power_on(struct mtk_apu *apu)
{
	struct device *dev = apu->dev;
	int ret, timeout;

	/* to force apu top power on synchronously */
	ret = pm_runtime_get_sync(apu->power_dev);
	if (ret < 0) {
		dev_info(apu->dev,
			"%s: call to get_sync(power_dev) failed, ret=%d\n",
			__func__, ret);
		/* apusys_rv_aee_warn("APUSYS_RV", "APUSYS_RV_RPM_GET_PWR_ERROR"); */
		return ret;
	}

	/* to notify IOMMU power on */
	ret = pm_runtime_get_sync(apu->apu_iommu0);
	if (ret < 0)
		goto iommu_get_error;

	ret = pm_runtime_get_sync(apu->apu_iommu1);
	if (ret < 0)
		pm_runtime_put_sync(apu->apu_iommu0);

iommu_get_error:
	if (ret < 0) {
		dev_info(apu->dev,
			"%s: call to get_sync(iommu) failed, ret=%d\n",
			__func__, ret);
		apusys_rv_aee_warn("APUSYS_RV", "APUSYS_IOMMU_RPM_GET_ERROR");
		goto error_put_power_dev;
	}

	/* polling IOMMU rpm state till active */
	apu_drv_debug("start polling iommu on\n");
	timeout = 5000;
	while ((!pm_runtime_active(apu->apu_iommu0) ||
			!pm_runtime_active(apu->apu_iommu1)) && timeout-- > 0)
		msleep(20);
	if (timeout <= 0) {
		dev_info(apu->dev, "%s: polling iommu on timeout!!\n",
			__func__);
		WARN_ON(0);
		apusys_rv_aee_warn("APUSYS_RV", "APUSYS_RV_IOMMU_ON_TIMEOUT");
		ret = -ETIMEDOUT;
		goto error_put_iommu_dev;
	}
	apu_drv_debug("polling iommu on done\n");

	ret = pm_runtime_get_sync(apu->dev);
	if (ret < 0) {
		dev_info(apu->dev,
			"%s: call to get_sync(dev) failed, ret=%d\n",
			__func__, ret);
		apusys_rv_aee_warn("APUSYS_RV", "APUSYS_RV_RPM_GET_ERROR");
		goto error_put_iommu_dev;
	}

	return 0;

error_put_iommu_dev:
	pm_runtime_put_sync(apu->apu_iommu1);
	pm_runtime_put_sync(apu->apu_iommu0);

error_put_power_dev:
	pm_runtime_put_sync(apu->power_dev);

	return ret;
}

static int mt8188_apu_power_off(struct mtk_apu *apu)
{
	struct device *dev = apu->dev;
	int ret, timeout;

	ret = pm_runtime_put_sync(apu->dev);
	if (ret) {
		dev_info(apu->dev,
			"%s: call to put_sync(dev) failed, ret=%d\n",
			__func__, ret);
		/* apusys_rv_aee_warn("APUSYS_RV", "APUSYS_RV_RPM_PUT_ERROR"); */
		return ret;
	}

	/* to notify IOMMU power off */
	ret = pm_runtime_put_sync(apu->apu_iommu1);
	if (ret < 0)
		goto iommu_put_error;

	ret = pm_runtime_put_sync(apu->apu_iommu0);
	if (ret < 0)
		pm_runtime_get_sync(apu->apu_iommu1);

iommu_put_error:
	if (ret < 0) {
		dev_info(apu->dev,
			"%s: call to put_sync(iommu) failed, ret=%d\n",
			__func__, ret);
		apusys_rv_aee_warn("APUSYS_RV", "APUSYS_IOMMU_RPM_PUT_ERROR");
		goto error_get_rv_dev;
	}

	/* polling IOMMU rpm state till suspended */
	apu_drv_debug("start polling iommu off\n");
	timeout = 5000;
	while ((!pm_runtime_suspended(apu->apu_iommu0) ||
		!pm_runtime_suspended(apu->apu_iommu1)) && timeout-- > 0)
		msleep(20);
	if (timeout <= 0) {
		dev_info(apu->dev, "%s: polling iommu off timeout!!\n",
			__func__);
		apu_ipi_unlock(apu);
		WARN_ON(0);
		apusys_rv_aee_warn("APUSYS_RV", "APUSYS_RV_IOMMU_OFF_TIMEOUT");
		ret = -ETIMEDOUT;
		goto error_get_iommu_dev;
	}

	apu_drv_debug("polling iommu off done\n");

	/* to force apu top power off synchronously */
	ret = pm_runtime_put_sync(apu->power_dev);
	if (ret) {
		dev_info(apu->dev,
			"%s: call to put_sync(power_dev) failed, ret=%d\n",
			__func__, ret);
		apusys_rv_aee_warn("APUSYS_RV", "APUSYS_RV_RPM_PUT_PWR_ERROR");
		goto error_get_iommu_dev;
	}

	/* polling APU TOP rpm state till suspended */
	apu_drv_debug("start polling power off\n");
	timeout = 500;
	while (!pm_runtime_suspended(apu->power_dev) && timeout-- > 0)
		msleep(20);
	if (timeout <= 0) {
		dev_info(apu->dev, "%s: polling power off timeout!!\n",
			__func__);
		apu_ipi_unlock(apu);
		WARN_ON(0);
		apusys_rv_aee_warn("APUSYS_RV", "APUSYS_RV_PWRDN_TIMEOUT");
		ret = -ETIMEDOUT;
		goto error_get_power_dev;
	}

	apu_drv_debug("polling power done\n");

	return 0;

error_get_power_dev:
	pm_runtime_get_sync(apu->power_dev);
error_get_iommu_dev:
	pm_runtime_get_sync(apu->apu_iommu0);
	pm_runtime_get_sync(apu->apu_iommu1);
error_get_rv_dev:
	pm_runtime_get_sync(apu->dev);

	return ret;
}

static int mt8188_irq_affin_init(struct mtk_apu *apu)
{
	int i;

	/* init perf_cpus mask 0x80, CPU7 only */
	cpumask_clear(&perf_cpus);
	cpumask_set_cpu(7, &perf_cpus);

	/* init normal_cpus mask 0x0f, CPU0~CPU4 */
	cpumask_clear(&normal_cpus);
	for (i = 0; i < 4; i++)
		cpumask_set_cpu(i, &normal_cpus);

	irq_set_affinity_hint(apu->mbox0_irq_number, &normal_cpus);

	return 0;
}

static int mt8188_irq_affin_set(struct mtk_apu *apu)
{
	irq_set_affinity_hint(apu->mbox0_irq_number, &perf_cpus);

	return 0;
}

static int mt8188_irq_affin_unset(struct mtk_apu *apu)
{
	irq_set_affinity_hint(apu->mbox0_irq_number, &normal_cpus);

	return 0;
}

static int mt8188_apu_memmap_init(struct mtk_apu *apu)
{
	struct platform_device *pdev = apu->pdev;
	struct device *dev = apu->dev;
	struct resource *res;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "apu_mbox");
	if (res == NULL) {
		dev_info(dev, "%s: apu_mbox get resource fail\n", __func__);
		return -ENODEV;
	}
	apu->apu_mbox = devm_ioremap_resource(dev, res);
	if (IS_ERR((void const *)apu->apu_mbox)) {
		dev_info(dev, "%s: apu_mbox remap base fail\n", __func__);
		return -ENOMEM;
	}

	res = platform_get_resource_byname(
		pdev, IORESOURCE_MEM, "md32_sysctrl");
	if (res == NULL) {
		dev_info(dev, "%s: md32_sysctrl get resource fail\n", __func__);
		return -ENODEV;
	}
	apu->md32_sysctrl = devm_ioremap_resource(dev, res);
	if (IS_ERR((void const *)apu->md32_sysctrl)) {
		dev_info(dev, "%s: md32_sysctrl remap base fail\n", __func__);
		return -ENOMEM;
	}

	res = platform_get_resource_byname(
		pdev, IORESOURCE_MEM, "md32_debug_apb");
	if (res == NULL) {
		dev_info(dev, "%s: md32_debug_apb get resource fail\n", __func__);
		return -ENODEV;
	}
	apu->md32_debug_apb = devm_ioremap_resource(dev, res);
	if (IS_ERR((void const *)apu->md32_debug_apb)) {
		dev_info(dev, "%s: md32_debug_apb remap base fail\n", __func__);
		return -ENOMEM;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "apu_wdt");
	if (res == NULL) {
		dev_info(dev, "%s: apu_wdt get resource fail\n", __func__);
		return -ENODEV;
	}
	apu->apu_wdt = devm_ioremap_resource(dev, res);
	if (IS_ERR((void const *)apu->apu_wdt)) {
		dev_info(dev, "%s: apu_wdt remap base fail\n", __func__);
		return -ENOMEM;
	}

	res = platform_get_resource_byname(
		pdev, IORESOURCE_MEM, "apu_sctrl_reviser");
	if (res == NULL) {
		dev_info(dev, "%s: apu_sctrl_reviser get resource fail\n", __func__);
		return -ENODEV;
	}
	apu->apu_sctrl_reviser = devm_ioremap_resource(dev, res);
	if (IS_ERR((void const *)apu->apu_sctrl_reviser)) {
		dev_info(dev, "%s: apu_sctrl_reviser remap base fail\n", __func__);
		return -ENOMEM;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "apu_ao_ctl");
	if (res == NULL) {
		dev_info(dev, "%s: apu_ao_ctl get resource fail\n", __func__);
		return -ENODEV;
	}
	apu->apu_ao_ctl = devm_ioremap_resource(dev, res);
	if (IS_ERR((void const *)apu->apu_ao_ctl)) {
		dev_info(dev, "%s: apu_ao_ctl remap base fail\n", __func__);
		return -ENOMEM;
	}

	apu->md32_tcm = NULL;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "md32_cache_dump");
	if (res == NULL) {
		dev_info(dev, "%s: md32_cache_dump get resource fail\n", __func__);
		return -ENODEV;
	}
	apu->md32_cache_dump = devm_ioremap_resource(dev, res);
	if (IS_ERR((void const *)apu->md32_cache_dump)) {
		dev_info(dev, "%s: md32_cache_dump remap base fail\n", __func__);
		return -ENOMEM;
	}

	return 0;
}

static void mt8188_apu_memmap_remove(struct mtk_apu *apu)
{
}

static void mt8188_rv_cg_gating(struct mtk_apu *apu)
{
	struct mtk_apu_reg_ofs *reg_ofs = &apu->platdata->ofs;

	iowrite32(0x0, apu->md32_sysctrl + reg_ofs->md32_clk_en);
}

static void mt8188_rv_cg_ungating(struct mtk_apu *apu)
{
	struct mtk_apu_reg_ofs *reg_ofs = &apu->platdata->ofs;

	iowrite32(0x1, apu->md32_sysctrl + reg_ofs->md32_clk_en);
}

static void mt8188_rv_cachedump(struct mtk_apu *apu)
{
	int offset;
	unsigned long flags;
	struct mtk_apu_reg_ofs *reg_ofs = &apu->platdata->ofs;

	struct apu_coredump *coredump =
		(struct apu_coredump *) apu->coredump_buf;

	spin_lock_irqsave(&apu->reg_lock, flags);
	/* set APU_UP_SYS_DBG_EN for cache dump enable through normal APB */
	iowrite32(ioread32(apu->md32_sysctrl + reg_ofs->dbg_bus_sel) |
		(1UL << 16), apu->md32_sysctrl + reg_ofs->dbg_bus_sel);
	spin_unlock_irqrestore(&apu->reg_lock, flags);

	for (offset = 0; offset < CACHE_DUMP_SIZE/sizeof(uint32_t); offset++)
		coredump->cachedump[offset] =
			ioread32(apu->md32_cache_dump + offset*sizeof(uint32_t));

	spin_lock_irqsave(&apu->reg_lock, flags);
	/* clear APU_UP_SYS_DBG_EN */
	iowrite32(ioread32(apu->md32_sysctrl + reg_ofs->dbg_bus_sel) &
		~(1UL << 16), apu->md32_sysctrl + reg_ofs->dbg_bus_sel);
	spin_unlock_irqrestore(&apu->reg_lock, flags);
}

const struct mtk_apu_platdata mt8188_platdata = {
	.flags		= F_PRELOAD_FIRMWARE | F_AUTO_BOOT | F_REQUIRE_VPU_INIT
			| F_SECURE_BOOT | F_SECURE_COREDUMP
			| F_APUSYS_SECURE,
	.sec_iova       = 0x200000UL,
	.ops		= {
		.init	= mt8188_rproc_init,
		.exit	= mt8188_rproc_exit,
		.start	= mt8188_rproc_start,
		.stop	= mt8188_rproc_stop,
		.apu_memmap_init = mt8188_apu_memmap_init,
		.apu_memmap_remove = mt8188_apu_memmap_remove,
		.cg_gating = mt8188_rv_cg_gating,
		.cg_ungating = mt8188_rv_cg_ungating,
		.rv_cachedump = mt8188_rv_cachedump,
		.power_init = mt8188_apu_power_init,
		.power_on = mt8188_apu_power_on,
		.power_off = mt8188_apu_power_off,
		.irq_affin_init = mt8188_irq_affin_init,
		.irq_affin_set = mt8188_irq_affin_set,
		.irq_affin_unset = mt8188_irq_affin_unset,
	},
	.ofs		= {
		.normal_dns		= 0x0000,
		.pri_dns		= 0x0004,
		.iommu_ctrl		= 0x0008,
		.core0_vabase0		= 0x000c,
		.core0_mvabase0		= 0x0010,
		.core0_vabase1		= 0x0014,
		.core0_mvabase1		= 0x0018,
		.userfw_ctxt		= 0x1000,
		.secfw_ctxt		= 0x1004,
		.md32_sys_ctrl		= 0x0000,
		.dbg_bus_sel		= 0x0098,
		.md32_clk_en		= 0x00b8,
		.up_wake_host_mask0	= 0x00bc,
		.md32_pre_def		= 0x0000,
		.md32_boot_ctrl		= 0x0004,
		.md32_runstall		= 0x0008,
		.mbox_host_cfg		= 0x0048,
	},
};
