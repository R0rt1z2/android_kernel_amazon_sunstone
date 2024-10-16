/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Yong Wu <yong.wu@mediatek.com>
 */
#ifndef __DT_BINDINGS_MEMORY_MTK_MEMORY_PORT_H_
#define __DT_BINDINGS_MEMORY_MTK_MEMORY_PORT_H_

#define MTK_LARB_NR_MAX			64
#define MTK_M4U_DOM_NR_MAX		16

/* dom[19:16] + larb[10:5] + port[4:0] */
#define MTK_M4U_DOM_ID(dom, larb, port)	((dom & 0xf) << 16 |\
					 ((larb & 0x3f) << 5) | (port & 0x1f))
#define MTK_IFRMMU_DOM_ID(dom, port)	((dom & 0xf) << 12 | (port & 0xff))

/* The default dom is 0. */
#define MTK_M4U_ID(larb, port)		(((larb) << 5) | (port))
#define MTK_M4U_TO_LARB(id)		(((id) >> 5) & 0x3f)
#define MTK_M4U_TO_PORT(id)		((id) & 0x1f)
#define MTK_M4U_TO_DOM(id)		(((id) >> 16) & 0xf)

#define MTK_IFRMMU_TO_DEV(id)		((id) & 0xff)

#endif
