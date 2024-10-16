/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Device Tree defines for LCM settings
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "mtk_lcm_lk_settings.h"

#ifndef MTK_LCM_SETTINGS_H
#define MTK_LCM_SETTINGS_H

#define MAX_UINT32 ((u32)~0U)
#define MAX_INT32 ((s32)(MAX_UINT32 >> 1))

#define MTK_PANEL_TABLE_OPS_COUNT (1024)
#define MTK_PANEL_COMPARE_ID_LENGTH (10)
#define MTK_PANEL_ATA_ID_LENGTH (10)

#define BL_PWM_MODE (0)
#define BL_I2C_MODE (1)

/* LCM PHY TYPE*/
#define MTK_LCM_MIPI_DPHY (0)
#define MTK_LCM_MIPI_CPHY (1)
#define MTK_LCM_MIPI_PHY_COUNT (MTK_LCM_MIPI_CPHY + 1)

/*redefine "mipi_dsi_pixel_format" from enum to macro for dts settings*/
#define MTK_MIPI_DSI_FMT_RGB888 (0)
#define MTK_MIPI_DSI_FMT_RGB666 (1)
#define MTK_MIPI_DSI_FMT_RGB666_PACKED (2)
#define MTK_MIPI_DSI_FMT_RGB565 (3)

/* redefine MTK_PANEL_OUTPUT_MODE from enum to macro for dts settings */
#define MTK_LCM_PANEL_SINGLE_PORT (0)
#define MTK_LCM_PANEL_DSC_SINGLE_PORT (1)
#define MTK_LCM_PANEL_DUAL_PORT (2)

/*redefine "mtk_drm_color_mode" from enum to macro for dts settings*/
#define MTK_LCM_COLOR_MODE_NATIVE (0)
#define MTK_LCM_COLOR_MODE_STANDARD_BT601_625 (1)
#define MTK_LCM_COLOR_MODE_STANDARD_BT601_625_UNADJUSTED (2)
#define MTK_LCM_COLOR_MODE_STANDARD_BT601_525 (3)
#define MTK_LCM_COLOR_MODE_STANDARD_BT601_525_UNADJUSTED (4)
#define MTK_LCM_COLOR_MODE_STANDARD_BT709 (5)
#define MTK_LCM_COLOR_MODE_DCI_P3 (6)
#define MTK_LCM_COLOR_MODE_SRGB (7)
#define MTK_LCM_COLOR_MODE_ADOBE_RGB (8)
#define MTK_LCM_COLOR_MODE_DISPLAY_P3 (9)

/* redefine "MIPITX_PHY_LANE_SWAP" from enum to macro for dts settings*/
#define LCM_LANE_0 (0)
#define LCM_LANE_1 (1)
#define LCM_LANE_2 (2)
#define LCM_LANE_3 (3)
#define LCM_LANE_CK (4)
#define LCM_LANE_RX (5)

/*redefine DSI mode flags to fix build error*/
/* video mode */
#define MTK_MIPI_DSI_MODE_VIDEO		(1U << 0)
/* video burst mode */
#define MTK_MIPI_DSI_MODE_VIDEO_BURST	(1U << 1)
/* video pulse mode */
#define MTK_MIPI_DSI_MODE_VIDEO_SYNC_PULSE	(1U << 2)
/* enable auto vertical count mode */
#define MTK_MIPI_DSI_MODE_VIDEO_AUTO_VERT	(1U << 3)
/* enable hsync-end packets in vsync-pulse and v-porch area */
#define MTK_MIPI_DSI_MODE_VIDEO_HSE		(1U << 4)
/* disable hfront-porch area */
#define MTK_MIPI_DSI_MODE_VIDEO_HFP		(1U << 5)
/* disable hback-porch area */
#define MTK_MIPI_DSI_MODE_VIDEO_HBP		(1U << 6)
/* disable hsync-active area */
#define MTK_MIPI_DSI_MODE_VIDEO_HSA		(1U << 7)
/* flush display FIFO on vsync pulse */
#define MTK_MIPI_DSI_MODE_VSYNC_FLUSH	(1U << 8)
/* disable EoT packets in HS mode */
#define MTK_MIPI_DSI_MODE_EOT_PACKET	(1U << 9)
/* device supports non-continuous clock behavior (DSI spec 5.6.1) */
#define MTK_MIPI_DSI_CLOCK_NON_CONTINUOUS	(1U << 10)
/* transmit data in low power */
#define MTK_MIPI_DSI_MODE_LPM		(1U << 11)
/* disable BLLP area */
#define MTK_MIPI_DSI_MODE_VIDEO_BLLP	(1U << 12)
/* disable EOF BLLP area */
#define MTK_MIPI_DSI_MODE_VIDEO_EOF_BLLP	(1U << 13)

/* LCM_FUNC used for common operation */
#define MTK_LCM_FUNC_DBI (0)
#define MTK_LCM_FUNC_DPI (1)
#define MTK_LCM_FUNC_DSI (2)
#define MTK_LCM_FUNC_END (MTK_LCM_FUNC_DSI + 1)

/* 0~127: used for common panel operation
 * customer is forbidden to use common panel operation
 */
#define MTK_LCM_UTIL_TYPE_START (0x0)
#define MTK_LCM_UTIL_TYPE_RESET (0x1) /*panel reset*/
#define MTK_LCM_UTIL_TYPE_POWER_ON (0x2) /*panel power on*/
#define MTK_LCM_UTIL_TYPE_POWER_OFF (0x3) /*panel power off*/
#define MTK_LCM_UTIL_TYPE_POWER_VOLTAGE (0x4) /*change panel voltage*/
#define MTK_LCM_UTIL_TYPE_MDELAY (0x5) /* msleep */
#define MTK_LCM_UTIL_TYPE_UDELAY (0x6) /* udelay*/
#define MTK_LCM_UTIL_TYPE_TDELAY (0x7) /* tick delay*/
#define MTK_LCM_UTIL_TYPE_END (0xf)

#define MTK_LCM_CMD_TYPE_START (0x10)
#define MTK_LCM_CMD_TYPE_WRITE_BUFFER (0x11) /* mipi dcs write data to panel*/
#define MTK_LCM_CMD_TYPE_WRITE_CMD (0x12) /* mipi dcs write cmd to panel*/
#define MTK_LCM_CMD_TYPE_READ_BUFFER (0x13) /* mipi dcs read data from panel*/
#define MTK_LCM_CMD_TYPE_READ_CMD (0x14) /* mipi dcs read cmd from panel*/
#define MTK_LCM_CMD_TYPE_WRITE_BUFFER_CONDITION (0x15) /* mipi dcs write data by condition*/
#define MTK_LCM_CMD_TYPE_WRITE_BUFFER_RUNTIME_INPUT (0x16) /* mipi dcs write runtime input data*/
#define MTK_LCM_CMD_TYPE_END (0x1f)

#define MTK_LCM_CB_TYPE_START (0x20)
#define MTK_LCM_CB_TYPE_RUNTIME (0x21) /* runtime executed in callback*/
#define MTK_LCM_CB_TYPE_RUNTIME_INPUT (0x22) /* runtime executed with single input*/
#define MTK_LCM_CB_TYPE_RUNTIME_INPUT_MULTIPLE (0x23) /* runtime executed with multiple input*/
#define MTK_LCM_CB_TYPE_END (0x2f)

#define MTK_LCM_GPIO_TYPE_START (0x30)
#define MTK_LCM_GPIO_TYPE_MODE (0x31) /* gpio mode control*/
#define MTK_LCM_GPIO_TYPE_DIR_OUTPUT (0x32) /* gpio output direction */
#define MTK_LCM_GPIO_TYPE_DIR_INPUT (0x33) /* gpio input direction*/
#define MTK_LCM_GPIO_TYPE_OUT (0x34) /* gpio value control*/
#define MTK_LCM_GPIO_TYPE_END (0x3f)

#define MTK_LCM_LK_TYPE_START (0x40)
#define MTK_LCM_LK_TYPE_WRITE_PARAM (0x41) /* lk write dcs data w/ force update*/
#define MTK_LCM_LK_TYPE_PREPARE_PARAM_COUNT (0x42) /* lk dcs data count*/
#define MTK_LCM_LK_TYPE_PREPARE_PARAM (0x43) /* lk fixed dcs data value of 32bit*/
#define MTK_LCM_LK_TYPE_PREPARE_PARAM_FIX_BIT (0x44) /* lk fixed dcs data value of 8bit*/
#define MTK_LCM_LK_TYPE_PREPARE_PARAM_X0_MSB_BIT (0x45) /* runtime 8bit input of x0 msb*/
#define MTK_LCM_LK_TYPE_PREPARE_PARAM_X0_LSB_BIT (0x46) /* runtime 8bit input of x0 lsb*/
#define MTK_LCM_LK_TYPE_PREPARE_PARAM_X1_MSB_BIT (0x47) /* runtime 8bit input of x1 msb*/
#define MTK_LCM_LK_TYPE_PREPARE_PARAM_X1_LSB_BIT (0x48) /* runtime 8bit input of x1 lsb*/
#define MTK_LCM_LK_TYPE_PREPARE_PARAM_Y0_MSB_BIT (0x49) /* runtime 8bit input of y0 msb*/
#define MTK_LCM_LK_TYPE_PREPARE_PARAM_Y0_LSB_BIT (0x4a) /* runtime 8bit input of y0 lsb*/
#define MTK_LCM_LK_TYPE_PREPARE_PARAM_Y1_MSB_BIT (0x4b) /* runtime 8bit input of y1 msb*/
#define MTK_LCM_LK_TYPE_PREPARE_PARAM_Y1_LSB_BIT (0x4c) /* runtime 8bit input of y1 lsb*/
#define MTK_LCM_LK_TYPE_WRITE_PARAM_UNFORCE (0x4d) /* lk write dcs data w/o force update*/
#define MTK_LCM_LK_TYPE_END (0x4f)

/* 128~223: used for customization panel operation
 * customer operation can add operation here
 */
#define MTK_LCM_CUST_TYPE_START (0x80)
#define MTK_LCM_CUST_TYPE_END (0xdf)

#define MTK_LCM_PHASE_TYPE_START (0xf0)
#define MTK_LCM_PHASE_TYPE_END (0xf1)
#define MTK_LCM_TYPE_END (0xff)

#define MTK_LCM_UTIL_RESET_LOW   (0)
#define MTK_LCM_UTIL_RESET_HIGH  (1)

#define MTK_LCM_PHASE_KERNEL (1U)
#define MTK_LCM_PHASE_LK (1U << 1)
#define MTK_LCM_PHASE_LK_DISPLAY_ON_DELAY (1U << 2)

/* used for runtime input settings */
#define MTK_LCM_INPUT_TYPE_READBACK (0x1)
#define MTK_LCM_INPUT_TYPE_CURRENT_FPS (0x2)
#define MTK_LCM_INPUT_TYPE_CURRENT_BACKLIGHT (0x3)

#endif
