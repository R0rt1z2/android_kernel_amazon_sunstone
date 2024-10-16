// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include "mtk_qos_ipi.h"
#include "mtk_qos_bound.h"
#ifdef QOS_PREFETCH_SUPPORT
#include "mtk_qos_prefetch.h"
#endif /* QOS_PREFETCH_SUPPORT */
#include "mtk_qos_sram.h"
#include "mtk_qos_sysfs.h"

#ifdef QOS_SHARE_SUPPORT
#include "mtk_qos_share.h"
#endif /* QOS_SHARE_SUPPORT */

static int mtk_qos_probe(struct platform_device *pdev)
{
	struct resource *res;
	void __iomem *regs;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(regs))
		return PTR_ERR(regs);
	qos_sram_init(regs);

	qos_add_interface(&pdev->dev);
	qos_ipi_init();
	qos_bound_init();
#ifdef QOS_PREFETCH_SUPPORT
	qos_prefetch_init();
#endif /* QOS_PREFETCH_SUPPORT */
	qos_ipi_recv_init();
#ifdef QOS_SHARE_SUPPORT
	qos_init_rec_share();
#endif /* QOS_SHARE_SUPPORT */
	return 0;
}

static int mtk_qos_remove(struct platform_device *pdev)
{
	qos_remove_interface(&pdev->dev);
	return 0;
}

static const struct of_device_id mtk_qos_of_match[] = {
	{ .compatible = "mediatek,qos" },
	{ .compatible = "mediatek,qos-2.0" },
	{ },
};

static struct platform_driver mtk_qos_platdrv = {
	.probe	= mtk_qos_probe,
	.remove	= mtk_qos_remove,
	.driver	= {
		.name	= "mtk-qos",
		.of_match_table = mtk_qos_of_match,
	},
};

static int __init mtk_qos_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&mtk_qos_platdrv);

	return ret;
}

late_initcall(mtk_qos_init)

static void __exit mtk_qos_exit(void)
{
	platform_driver_unregister(&mtk_qos_platdrv);
}
module_exit(mtk_qos_exit)

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek QoS driver");
