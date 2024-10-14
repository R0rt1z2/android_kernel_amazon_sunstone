/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __MTK_VDO1_COMP_H__
#define __MTK_VDO1_COMP_H__

#include "mtk_drm_crtc.h"
#include "mtk_drm_ddp.h"
#include "mtk_drm_ddp_comp.h"

#define VDO1_MUTEX_SOF  0x02C
#define VDO1_MUTEX_MOD0 0x030
#define VDO1_MUTEX_MOD1 0x034

#define VDO1_MUTEX_MOD0_MDP_RDMA0 BIT(0)
#define VDO1_MUTEX_MOD0_MDP_RDMA1 BIT(1)
#define VDO1_MUTEX_MOD0_MDP_RDMA2 BIT(2)
#define VDO1_MUTEX_MOD0_MDP_RDMA3 BIT(3)
#define VDO1_MUTEX_MOD0_MDP_RDMA4 BIT(4)
#define VDO1_MUTEX_MOD0_MDP_RDMA5 BIT(5)
#define VDO1_MUTEX_MOD0_MDP_RDMA6 BIT(6)
#define VDO1_MUTEX_MOD0_MDP_RDMA7 BIT(7)

#define VDO1_MUTEX_MOD0_DISP_PADDING0 BIT(8)
#define VDO1_MUTEX_MOD0_DISP_PADDING1 BIT(9)
#define VDO1_MUTEX_MOD0_DISP_PADDING2 BIT(10)
#define VDO1_MUTEX_MOD0_DISP_PADDING3 BIT(11)
#define VDO1_MUTEX_MOD0_DISP_PADDING4 BIT(12)
#define VDO1_MUTEX_MOD0_DISP_PADDING5 BIT(13)
#define VDO1_MUTEX_MOD0_DISP_PADDING6 BIT(14)
#define VDO1_MUTEX_MOD0_DISP_PADDING7 BIT(15)

#define VDO1_MUTEX_MOD0_DISP_RSZ0 BIT(16)
#define VDO1_MUTEX_MOD0_DISP_RSZ1 BIT(17)
#define VDO1_MUTEX_MOD0_DISP_RSZ2 BIT(18)
#define VDO1_MUTEX_MOD0_DISP_RSZ3 BIT(19)

#define VDO1_MUTEX_MOD0_DISP_MERGE0 BIT(20)
#define VDO1_MUTEX_MOD0_DISP_MERGE1 BIT(21)
#define VDO1_MUTEX_MOD0_DISP_MERGE2 BIT(22)
#define VDO1_MUTEX_MOD0_DISP_MERGE3 BIT(23)
#define VDO1_MUTEX_MOD0_DISP_MERGE4 BIT(24)

//#define VDO1_MUTEX_MOD0_VPP2_DL_RELAY BIT(25)
//#define VDO1_MUTEX_MOD0_VPP3_DL_RELAY BIT(26)

//#define VDO1_MUTEX_MOD0_VDO0_DSC_DL_ASYNC    BIT(27)
#define VDO1_MUTEX_MOD0_VDO0_MERGE_DL_ASYNC  BIT(28)
#define VDO1_MUTEX_MOD0_VDO1_OUT_DL_RELAY    BIT(29)

#define VDO1_MUTEX_MOD0_DISP_MIXER           BIT(30)
#define VDO1_MUTEX_MOD0_HDR_VDO_FE0          BIT(31)
#define VOD1_MUTEX_MOD1_HDR_VDO_FE1         (BIT(0)  << 32)
#define VOD1_MUTEX_MOD1_HDR_GFX_FE0         (BIT(1)  << 32)
#define VOD1_MUTEX_MOD1_HDR_GFX_FE1         (BIT(2)  << 32)
#define VOD1_MUTEX_MOD1_HDR_VDO_BE0         (BIT(3)  << 32)
#define VOD1_MUTEX_MOD1_HDR_MLOAD           (BIT(4)  << 32)

#define VOD1_MUTEX_MOD1_DPI0              (BIT(5)  << 32)
#define VOD1_MUTEX_MOD1_DPI1              (BIT(6)  << 32)
#define VOD1_MUTEX_MOD1_DP_INTF           (BIT(7)  << 32)

#define VDO1_MUTEX_MOD1_HDR_VDO_FE0_ASYNC (BIT(8)  << 32)
#define VDO1_MUTEX_MOD1_HDR_VDO_FE1_ASYNC (BIT(9)  << 32)
#define VDO1_MUTEX_MOD1_HDR_GFX_FE0_ASYNC (BIT(10) << 32)
#define VDO1_MUTEX_MOD1_HDR_GFX_FE1_ASYNC (BIT(11) << 32)
#define VDO1_MUTEX_MOD1_HDR_VDO_BE0_ASYNC (BIT(12) << 32)

#define VDO1_MUTEX_SOF_SINGLE_MODE 0
#define VDO1_MUTEX_SOF_DSI0        1
#define VDO1_MUTEX_SOF_DSI1        2
#define VDO1_MUTEX_SOF_EDP_INTF0   3
#define VDO1_MUTEX_SOF_DP_INTF0    4
#define VDO1_MUTEX_SOF_DPI1        5
#define VDO1_MUTEX_SOF_DPI0        6
#define VDO1_MUTEX_SOF_SPLIT0      7
#define VDO1_MUTEX_SOF_SPLIT1      8
#define VDO1_MUTEX_SOF_SPLIT0_DLY  9
#define VDO1_MUTEX_SOF_SPLIT1_DLY  10

#define VDO1_VPP_MERGE0_P0_SEL_IN 0xF04
    #define VDO1_VPP_MERGE0_P0_SEL_IN_FROM_VPPSYS1   0
    #define VDO1_VPP_MERGE0_P0_SEL_IN_FROM_MDP_RDMA0 1
#define VDO1_VPP_MERGE0_P1_SEL_IN 0xF08
    #define VDO1_VPP_MERGE0_P1_SEL_IN_FROM_VPP3_ASYNC_SOUT 0
    #define VDO1_VPP_MERGE0_P1_SEL_IN_FROM_MDP_RDMA1       1
#define VDO1_VPP_MERGE1_P0_SEL_IN 0xF3C
    #define VDO1_VPP_MERGE1_P0_SEL_IN_FROM_VPP3_ASYNC_SOUT 0
    #define VDO1_VPP_MERGE1_P0_SEL_IN_FROM_MDP_RDMA2       1

#define VDO1_RSZ0_SEL_IN 0xF9C
    #define VDO1_RSZ0_SEL_IN_FROM_RDMA4_RSZ_SOUT 0
    #define VDO1_RSZ0_SEL_IN_FROM_RDMA2_RSZ_SOUT 1
    #define VDO1_RSZ0_SEL_IN_FROM_RDMA0_SOUT     2
#define VDO1_RSZ1_SEL_IN 0xFA0
    #define VDO1_RSZ1_SEL_IN_FROM_RDMA5_RSZ_SOUT 0
    #define VDO1_RSZ1_SEL_IN_FROM_RDMA3_RSZ_SOUT 1
    #define VDO1_RSZ1_SEL_IN_FROM_RDMA1_SOUT     2
#define VDO1_RSZ2_SEL_IN 0xFA4
    #define VDO1_RSZ2_SEL_IN_FROM_RDMA6_SOUT     0
    #define VDO1_RSZ2_SEL_IN_FROM_RDMA4_RSZ_SOUT 1
    #define VDO1_RSZ2_SEL_IN_FROM_RDMA2_RSZ_SOUT 2
#define VDO1_RSZ3_SEL_IN 0xFA8
    #define VDO1_RSZ3_SEL_IN_FROM_RDMA7_SOUT     0
    #define VDO1_RSZ3_SEL_IN_FROM_RDMA5_RSZ_SOUT 1
    #define VDO1_RSZ3_SEL_IN_FROM_RDMA3_RSZ_SOUT 2

#define VDO1_RDMA0_SEL_IN 0xFBC
    #define VDO1_RDMA0_SEL_IN_FROM_RDMA0_SOUT 0
    #define VDO1_RDMA0_SEL_IN_FROM_RSZ0_SOUT  1
#define VDO1_RDMA1_SEL_IN 0xFC0
    #define VDO1_RDMA1_SEL_IN_FROM_RDMA1_SOUT 0
    #define VDO1_RDMA1_SEL_IN_FROM_RSZ1_SOUT  1
#define VDO1_RDMA2_SEL_IN 0xFC4
    #define VDO1_RDMA2_SEL_IN_FROM_RDMA2_SOUT 0
    #define VDO1_RDMA2_SEL_IN_FROM_RSZ2_SOUT  1
    #define VDO1_RDMA2_SEL_IN_FROM_RSZ0_SOUT  2
#define VDO1_RDMA3_SEL_IN 0xFC8
    #define VDO1_RDMA3_SEL_IN_FROM_RDMA3_SOUT 0
    #define VDO1_RDMA3_SEL_IN_FROM_RSZ3_SOUT  1
    #define VDO1_RDMA3_SEL_IN_FROM_RSZ1_SOUT  2
#define VDO1_RDMA4_SEL_IN 0xFCC
    #define VDO1_RDMA4_SEL_IN_FROM_RDMA4_SOUT 0
    #define VDO1_RDMA4_SEL_IN_FROM_RSZ2_SOUT  1
    #define VDO1_RDMA4_SEL_IN_FROM_RSZ0_SOUT  2
#define VDO1_RDMA5_SEL_IN 0xFD0
    #define VDO1_RDMA5_SEL_IN_FROM_RDMA5_SOUT 0
    #define VDO1_RDMA5_SEL_IN_FROM_RSZ3_SOUT  1
    #define VDO1_RDMA5_SEL_IN_FROM_RSZ1_SOUT  2
#define VDO1_RDMA6_SEL_IN 0xFD4
    #define VDO1_RDMA6_SEL_IN_FROM_RDMA6_SOUT 0
    #define VDO1_RDMA6_SEL_IN_FROM_RSZ2_SOUT  1
#define VDO1_RDMA7_SEL_IN 0xFD8
    #define VDO1_RDMA7_SEL_IN_FROM_RDMA7_SOUT 0
    #define VDO1_RDMA7_SEL_IN_FROM_RSZ3_SOUT  1

#define VDO1_MIXER_IN1_SEL_IN 0xF24
    #define VDO1_MIXER_IN1_SEL_IN_FROM_HDR_VDO_FE0       0
    #define VDO1_MIXER_IN1_SEL_IN_FROM_MERGE0_ASYNC_SOUT 1
#define VDO1_MIXER_IN2_SEL_IN 0xF28
    #define VDO1_MIXER_IN2_SEL_IN_FROM_HDR_VDO_FE1       0
    #define VDO1_MIXER_IN2_SEL_IN_FROM_MERGE1_ASYNC_SOUT 1
#define VDO1_MIXER_IN3_SEL_IN 0xF2C
    #define VDO1_MIXER_IN3_SEL_IN_FROM_HDR_GFX_FE0       0
    #define VDO1_MIXER_IN3_SEL_IN_FROM_MERGE2_ASYNC_SOUT 1
#define VDO1_MIXER_IN4_SEL_IN 0xF30
    #define VDO1_MIXER_IN4_SEL_IN_FROM_HDR_GFX_FE1       0
    #define VDO1_MIXER_IN4_SEL_IN_FROM_MERGE3_ASYNC_SOUT 1

#define VDO1_MIXER_SOUT_SEL_IN 0xF68
    #define VDO1_MIXER_SOUT_SEL_IN_FROM_DISP_MIXER     0
    #define VDO1_MIXER_SOUT_SEL_IN_FROM_MIXER_IN1_SOUT 1
    #define VDO1_MIXER_SOUT_SEL_IN_FROM_MIXER_IN2_SOUT 2
    #define VDO1_MIXER_SOUT_SEL_IN_FROM_MIXER_IN3_SOUT 3
    #define VDO1_MIXER_SOUT_SEL_IN_FROM_MIXER_IN4_SOUT 4

#define VDO1_MERGE4_ASYNC_SEL_IN 0xF50
    #define VDO1_MERGE4_ASYNC_SEL_IN_FROM_HDR_VDO_BE0		0
    #define VDO1_MERGE4_ASYNC_SEL_IN_FROM_MIXER_OUT_SOUT	1
    #define VDO1_MERGE4_ASYNC_SEL_IN_FROM_MERGE0_ASYNC_SOUT	2
    #define VDO1_MERGE4_ASYNC_SEL_IN_FROM_MERGE1_ASYNC_SOUT	3
    #define VDO1_MERGE4_ASYNC_SEL_IN_FROM_MERGE2_ASYNC_SOUT	4
    #define VDO1_MERGE4_ASYNC_SEL_IN_FROM_MERGE3_ASYNC_SOUT	5

#define VDO1_DISP_DP_INTF0_SEL_IN 0xF14
    #define VDO1_DISP_DP_INTF0_SEL_IN_FROM_VPP_MERGE4_MOUT          0
    #define VDO1_DISP_DP_INTF0_SEL_IN_FROM_VDO0_MERGE_DL_ASYNC_MOUT 1
    #define VDO1_DISP_DP_INTF0_SEL_IN_FROM_VDO0_DSC_DL_ASYNC_MOUT   2
#define VDO1_DISP_DPI0_SEL_IN 0xF0C
    #define VDO1_DISP_DPI0_SEL_IN_FROM_VPP_MERGE4_MOUT          0
    #define VDO1_DISP_DPI0_SEL_IN_FROM_VDO0_MERGE_DL_ASYNC_MOUT 1
    #define VDO1_DISP_DPI0_SEL_IN_FROM_VDO0_DSC_DL_ASYNC_MOUT   2
#define VDO1_DISP_DPI1_SEL_IN 0xF10
    #define VDO1_DISP_DPI1_SEL_IN_FROM_VPP_MERGE4_MOUT          0
    #define VDO1_DISP_DPI1_SEL_IN_FROM_VDO0_MERGE_DL_ASYNC_MOUT 1
    #define VDO1_DISP_DPI1_SEL_IN_FROM_VDO0_DSC_DL_ASYNC_MOUT   2



#define VDO1_VPP3_ASYNC_SOUT_SEL 0xF54
    #define VDO1_VPP3_ASYNC_SOUT_SEL_TO_VPP_MERGE1_P0_SEL 0
    #define VDO1_VPP3_ASYNC_SOUT_SEL_TO_VPP_MERGE0_P1_SEL 1

#define VDO1_RDMA0_SOUT_SEL 0xF6C
    #define VDO1_RDMA0_SOUT_SEL_TO_RDMA0_SEL 0
    #define VDO1_RDMA0_SOUT_SEL_TO_RSZ0_SEL  1
#define VDO1_RDMA1_SOUT_SEL 0xF70
    #define VDO1_RDMA1_SOUT_SEL_TO_RDMA1_SEL 0
    #define VDO1_RDMA1_SOUT_SEL_TO_RSZ1_SEL  1
#define VDO1_RDMA2_SOUT_SEL 0xF74
    #define VDO1_RDMA2_SOUT_SEL_TO_RDMA2_SEL      0
    #define VDO1_RDMA2_SOUT_SEL_TO_RDMA2_RSZ_SOUT 1
#define VDO1_RDMA3_SOUT_SEL 0xF78
    #define VDO1_RDMA3_SOUT_SEL_TO_RDMA3_SEL      0
    #define VDO1_RDMA3_SOUT_SEL_TO_RDMA3_RSZ_SOUT 1
#define VDO1_RDMA4_SOUT_SEL 0xF7C
    #define VDO1_RDMA4_SOUT_SEL_TO_RDMA4_SEL      0
    #define VDO1_RDMA4_SOUT_SEL_TO_RDMA4_RSZ_SOUT 1
#define VDO1_RDMA5_SOUT_SEL 0xF80
    #define VDO1_RDMA5_SOUT_SEL_TO_RDMA5_SEL      0
    #define VDO1_RDMA5_SOUT_SEL_TO_RDMA5_RSZ_SOUT 1
#define VDO1_RDMA6_SOUT_SEL 0xF84
    #define VDO1_RDMA6_SOUT_SEL_TO_RDMA6_SEL 0
    #define VDO1_RDMA6_SOUT_SEL_TO_RSZ2_SEL  1
#define VDO1_RDMA7_SOUT_SEL 0xF88
    #define VDO1_RDMA7_SOUT_SEL_TO_RDMA7_SEL 0
    #define VDO1_RDMA7_SOUT_SEL_TO_RSZ3_SEL  1

#define VDO1_RDMA2_RSZ_SOUT_SEL 0xF8C
    #define VDO1_RDMA2_RSZ_SOUT_SEL_TO_RSZ2_SEL 0
    #define VDO1_RDMA2_RSZ_SOUT_SEL_TO_RSZ0_SEL 1
#define VDO1_RDMA3_RSZ_SOUT_SEL 0xF90
    #define VDO1_RDMA3_RSZ_SOUT_SEL_TO_RSZ3_SEL 0
    #define VDO1_RDMA3_RSZ_SOUT_SEL_TO_RSZ1_SEL 1
#define VDO1_RDMA4_RSZ_SOUT_SEL 0xF94
    #define VDO1_RDMA4_RSZ_SOUT_SEL_TO_RSZ2_SEL 0
    #define VDO1_RDMA4_RSZ_SOUT_SEL_TO_RSZ0_SEL 1
#define VDO1_RDMA5_RSZ_SOUT_SEL 0xF98
    #define VDO1_RDMA5_RSZ_SOUT_SEL_TO_RSZ3_SEL 0
    #define VDO1_RDMA5_RSZ_SOUT_SEL_TO_RSZ1_SEL 1

#define VDO1_RSZ0_SOUT_SEL 0xFAC
    #define VDO1_RSZ0_SOUT_SEL_TO_RDMA4 0
    #define VDO1_RSZ0_SOUT_SEL_TO_RDMA2 1
    #define VDO1_RSZ0_SOUT_SEL_TO_RDMA0 2
#define VDO1_RSZ1_SOUT_SEL 0xFB0
    #define VDO1_RSZ1_SOUT_SEL_TO_RDMA5 0
    #define VDO1_RSZ1_SOUT_SEL_TO_RDMA3 1
    #define VDO1_RSZ1_SOUT_SEL_TO_RDMA1 2
#define VDO1_RSZ2_SOUT_SEL 0xFB4
    #define VDO1_RSZ2_SOUT_SEL_TO_RDMA6 0
    #define VDO1_RSZ2_SOUT_SEL_TO_RDMA4 1
    #define VDO1_RSZ2_SOUT_SEL_TO_RDMA2 2
#define VDO1_RSZ3_SOUT_SEL 0xFB8
    #define VDO1_RSZ3_SOUT_SEL_TO_RDMA7 0
    #define VDO1_RSZ3_SOUT_SEL_TO_RDMA5 1
    #define VDO1_RSZ3_SOUT_SEL_TO_RDMA3 2

#define VDO1_MERGE0_ASYNC_SOUT_SEL 0xF40
    #define VDO1_MERGE0_ASYNC_SOUT_SEL_TO_HDR_VDO_FE0      0
    #define VDO1_MERGE0_ASYNC_SOUT_SEL_TO_MIXER_IN1_SEL    1
    #define VDO1_MERGE0_ASYNC_SOUT_SEL_TO_MERGE4_ASYNC_SEL 2
#define VDO1_MERGE1_ASYNC_SOUT_SEL 0xF44
    #define VDO1_MERGE1_ASYNC_SOUT_SEL_HDR_VDO_FE1         0
    #define VDO1_MERGE1_ASYNC_SOUT_SEL_TO_MIXER_IN2_SEL    1
    #define VDO1_MERGE1_ASYNC_SOUT_SEL_TO_MERGE4_ASYNC_SEL 2
#define VDO1_MERGE2_ASYNC_SOUT_SEL 0xF48
    #define VDO1_MERGE2_ASYNC_SOUT_SEL_HDR_GFX_FE0         0
    #define VDO1_MERGE2_ASYNC_SOUT_SEL_TO_MIXER_IN3_SEL    1
    #define VDO1_MERGE2_ASYNC_SOUT_SEL_TO_MERGE4_ASYNC_SEL 2
#define VDO1_MERGE3_ASYNC_SOUT_SEL 0xF4C
    #define VDO1_MERGE3_ASYNC_SOUT_SEL_HDR_GFX_FE1         0
    #define VDO1_MERGE3_ASYNC_SOUT_SEL_TO_MIXER_IN4_SEL    1
    #define VDO1_MERGE3_ASYNC_SOUT_SEL_TO_MERGE4_ASYNC_SEL 2

#define VDO1_MIXER_IN1_SOUT_SEL 0xF58
    #define VDO1_MIXER_IN1_SOUT_SEL_TO_DISP_MIXER     0
    #define VDO1_MIXER_IN1_SOUT_SEL_TO_MIXER_SOUT_SEL 1
#define VDO1_MIXER_IN2_SOUT_SEL 0xF5C
    #define VDO1_MIXER_IN2_SOUT_SEL_TO_DISP_MIXER     0
    #define VDO1_MIXER_IN2_SOUT_SEL_TO_MIXER_SOUT_SEL 1
#define VDO1_MIXER_IN3_SOUT_SEL 0xF60
    #define VDO1_MIXER_IN3_SOUT_SEL_TO_DISP_MIXER     0
    #define VDO1_MIXER_IN3_SOUT_SEL_TO_MIXER_SOUT_SEL 1
#define VDO1_MIXER_IN4_SOUT_SEL 0xF64
    #define VDO1_MIXER_IN4_SOUT_SEL_TO_DISP_MIXER     0
    #define VDO1_MIXER_IN4_SOUT_SEL_TO_MIXER_SOUT_SEL 1

#define VDO1_MIXER_OUT_SOUT_SEL 0xF34
    #define VDO1_MIXER_OUT_SOUT_SEL_TO_HDR_VDO_BE0      0
    #define VDO1_MIXER_OUT_SOUT_SEL_TO_MERGE4_ASYNC_SEL 1



#define VDO1_MERGE4_MOUT_EN 0xF18
    #define VDO1_MERGE4_MOUT_EN_TO_VDOSYS0      BIT(0)
    #define VDO1_MERGE4_MOUT_EN_TO_DPI0_SEL     BIT(1)
    #define VDO1_MERGE4_MOUT_EN_TO_DPI1_SEL     BIT(2)
    #define VDO1_MERGE4_MOUT_EN_TO_DP_INTF0_SEL BIT(3)

//#define VDO0_DSC_DL_ASYNC_MOUT_EN 0xF1C
//    #define VDO0_DSC_DL_ASYNC_MOUT_EN_TO_DPI0_SEL     BIT(0)
//    #define VDO0_DSC_DL_ASYNC_MOUT_EN_TO_DPI1_SEL     BIT(1)
//    #define VDO0_DSC_DL_ASYNC_MOUT_EN_TO_DP_INTF0_SEL BIT(2)

#define VDO0_MERGE_DL_ASYNC_MOUT_EN 0xF20
    #define VDO0_MERGE_DL_ASYNC_MOUT_EN_TO_DPI0_SEL     BIT(0)
    #define VDO0_MERGE_DL_ASYNC_MOUT_EN_TO_DPI1_SEL     BIT(1)
    #define VDO0_MERGE_DL_ASYNC_MOUT_EN_TO_DP_INTF0_SEL BIT(2)



#define VDO1_HDR_VDO_FE0_CFG0 0xD14
    //#define VDO1_HDR_VDO_FE0_CFG0_{R HDR_VDO_FE0 DEFAULT OUTPUT
#define VDO1_HDR_VDO_FE1_CFG0 0xD18
    //#define VDO1_HDR_VDO_FE1_CFG0_{R HDR_VDO_FE1 DEFAULT OUTPUT
#define VDO1_HDR_GFX_FE0_CFG0 0xD1C
#define VDO1_HDR_GFX_FE1_CFG0 0xD20
#define VDO1_HDR_VDO_BE0_CFG0 0xD24
    //#define VDO1_HDR_VDO_BE0_CFG0_{R BIT(MIXER DEFAULT OUTPUT)



#define VDO1_MIXER_IN1_PAD 0xD40
    #define VDO1_MIXER_IN1_PAD_BYPASS      0
    #define VDO1_MIXER_IN1_PAD_EXTEND_EVEN 1
    #define VDO1_MIXER_IN1_PAD_EXTEND_ODD  2
#define VDO1_MIXER_IN2_PAD 0xD44
    #define VDO1_MIXER_IN2_PAD_BYPASS      0
    #define VDO1_MIXER_IN2_PAD_EXTEND_EVEN 1
    #define VDO1_MIXER_IN2_PAD_EXTEND_ODD  2
#define VDO1_MIXER_IN3_PAD 0xD48
    #define VDO1_MIXER_IN3_PAD_BYPASS      0
    #define VDO1_MIXER_IN3_PAD_EXTEND_EVEN 1
    #define VDO1_MIXER_IN3_PAD_EXTEND_ODD  2
#define VDO1_MIXER_IN4_PAD 0xD4C
    #define VDO1_MIXER_IN4_PAD_BYPASS      0
    #define VDO1_MIXER_IN4_PAD_EXTEND_EVEN 1
    #define VDO1_MIXER_IN4_PAD_EXTEND_ODD  2


#ifdef CONFIG_MTK_WFD_OVER_VDO1
#define VDO0_SIN_SEL_VDO0A_VDO1 0xF34
    #define VDO0_SIN_SEL_VDO0A_VDO1_FROM_VPP_MERGE0  0
#define VDO0_SOUT_SEL_VDO1_VDO0 0xF38
    #define VDO0_SOUT_SEL_VDO1_VDO0_TO_VPP_MERGE0    0
#define VDO0_SEL_VPP_MERGE0     0xF60
    #define VDO0_SEL_VPP_MERGE0_I0_FROM_VDO1         2
    #define VDO0_SEL_VPP_MERGE0_O0_TO_VDO1          (2 << 4)
    #define VDO0_SEL_VPP_MERGE0_O0_TO_DISP_WDMA0    (5 << 4)
#define VDO0_SIN_SEL_DISP_WDMA0 0xF6C
    #define VDO0_SIN_SEL_DISP_WDMA0_FROM_VPP_MERGE0_O0  2
#endif

int mtk_vdo1_sel_in(
	struct mtk_drm_crtc *mtk_crtc,
	enum mtk_ddp_comp_id curr,
	enum mtk_ddp_comp_id next,
	unsigned int *addr);

int mtk_vdo1_sout_sel(
	struct mtk_drm_crtc *mtk_crtc,
	enum mtk_ddp_comp_id curr,
	enum mtk_ddp_comp_id next);

int mtk_vdo1_mout_en(
	enum mtk_ddp_comp_id curr,
	enum mtk_ddp_comp_id next,
	unsigned int *addr);

#endif // __MTK_VDO1_COMP_H__
