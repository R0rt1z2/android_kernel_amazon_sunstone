/*
* amzn_sign_of_life_platform.h
*
* life cycle reason related data structure.
*
* Copyright (C) 2015-2022 Amazon Technologies Inc. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#ifndef __AMZN_SIGN_OF_LIFE_PLATFORM_H
#define __AMZN_SIGN_OF_LIFE_PLATFORM_H

#include <linux/amzn_sign_of_life.h>

/* sign_of_life_reason_data */
struct sign_of_life_reason_data {
	life_cycle_reason_t reason_value;
	char  *life_cycle_reasons;
	char  *life_cycle_type;
};

/* platform specific lcr_data */
static struct sign_of_life_reason_data lcr_data[] = {
	/* The default value in case we fail to find a good reason */
	{LIFE_CYCLE_NOT_AVAILABLE, "Life Cycle Reason Not Available", "LCR_abnormal"},
	/* Normal software shutdown */
	{SHUTDOWN_BY_SW, "Software Shutdown", "LCR_normal"},
	/* Device shutdown due to long pressing of power key */
	{SHUTDOWN_BY_LONG_PWR_KEY_PRESS, "Long Pressed Power Key Shutdown", "LCR_abnormal"},
	/* Device shutdown due to overheated PMIC */
	{THERMAL_SHUTDOWN_REASON_PMIC, "PMIC Overheated Thermal Shutdown", "LCR_abnormal"},
	/* Device shutdown due to overheated battery */
	{THERMAL_SHUTDOWN_REASON_BATTERY, "Battery Overheated Thermal Shutdown", "LCR_abnormal"},
	/* Device shutdown due to overheated Soc chipset */
	{THERMAL_SHUTDOWN_REASON_SOC, "SOC Overheated Thermal Shutdown", "LCR_abnormal"},
	/* Device shutdown due to overheated PCB main board */
	{THERMAL_SHUTDOWN_REASON_PCB, "PCB Overheated Thermal Shutdown", "LCR_abnormal"},
	/* Device shutdown due to overheated WIFI chipset */
	{THERMAL_SHUTDOWN_REASON_WIFI, "WIFI Overheated Thermal Shutdown", "LCR_abnormal"},
	/* Device shutdown due to overheated BTS */
	{THERMAL_SHUTDOWN_REASON_BTS, "BTS Overheated Thermal Shutdown", "LCR_abnormal"},
	/* Device shutdown due to overheated Modem chipset */
	{THERMAL_SHUTDOWN_REASON_MODEM, "Modem Overheated Thermal Shutdown", "LCR_abnormal"},
	/* Device shutdown due to extream low battery level */
	{LIFE_CYCLE_SMODE_LOW_BATTERY, "Low Battery Shutdown", "LCR_normal"},
	/* Device shutdown due to sudden power lost */
	{SHUTDOWN_BY_SUDDEN_POWER_LOSS, "Sudden Power Loss Shutdown", "LCR_abnormal"},
	/* Device shutdown due to over current protect */
	{SHUTDOWN_BY_OCP, "PMIC Over Current Protect Shutdown", "LCR_abnormal"},
	/* Device shutdown due to unknown reason */
	{SHUTDOWN_BY_UNKNOWN_REASONS, "Unknown Shutdown", "LCR_abnormal"},
	/* Device powerup due to power key pressed */
	{COLDBOOT_BY_POWER_KEY, "Cold Boot By Power Key", "LCR_normal"},
	/* Device powerup due to USB cable plugged */
	{COLDBOOT_BY_USB, "Cold Boot By USB", "LCR_normal"},
	/* Device powerup due to power supply plugged */
	{COLDBOOT_BY_POWER_SUPPLY, "Cold Boot By Power Supply", "LCR_normal"},
	/* Device reboot due software reboot */
	{WARMBOOT_BY_SW, "Warm Boot By Software", "LCR_normal"},
	/* Device reboot due kernel panic */
	{WARMBOOT_BY_KERNEL_PANIC, "Warm Boot By Kernel Panic", "LCR_abnormal"},
	/* Device reboot due software watchdog timeout */
	{WARMBOOT_BY_KERNEL_WATCHDOG, "Warm Boot By Kernel Watchdog", "LCR_abnormal"},
	/* Device reboot due kernel HW watchdog timeout */
	{WARMBOOT_BY_HW_WATCHDOG, "Warm Boot By HW Watchdog", "LCR_abnormal"},
	/* Device reboot due to 2sec reboot */
	{WARMBOOT_BY_2SEC_REBOOT, "Warm Boot By 2sec reboot", "LCR_abnormal"},
	/* Device powerup into charger mode */
	{LIFE_CYCLE_SMODE_WARM_BOOT_USB_CONNECTED, "Power Off Charging Mode", "LCR_normal"},
	/* Device reboot into factory reset mode */
	{LIFE_CYCLE_SMODE_FACTORY_RESET, "Factory Reset Reboot", "LCR_normal"},
	/* Device reboot into recovery OTA mode */
	{LIFE_CYCLE_SMODE_OTA, "OTA Reboot", "LCR_normal"},
	/* Device RTC Parity Check Failed */
	{LIFE_CYCLE_SMODE_RTC_CHECK_FAIL, "RTC Check Fail", "LCR_normal"},
};

typedef enum {
	BOOT,
	SHUTDOWN,
	THERMAL_SHUTDOWN,
	SPECIAL,
	LIFE_REASON_NUM,
} life_reason_type;

/* sign of life operations */
typedef struct sign_of_life_ops {
	int (*read_boot_reason)(life_cycle_reason_t *boot_reason);
	int (*write_boot_reason)(life_cycle_reason_t boot_reason);
	int (*read_shutdown_reason)(life_cycle_reason_t *shutdown_reason);
	int (*write_shutdown_reason)(life_cycle_reason_t shutdown_reason);
	int (*read_thermal_shutdown_reason)(life_cycle_reason_t *thermal_shutdown_reason);
	int (*write_thermal_shutdown_reason)(life_cycle_reason_t thermal_shutdown_reason);
	int (*read_special_mode)(life_cycle_reason_t *special_mode);
	int (*write_special_mode)(life_cycle_reason_t special_mode);
	int (*lcr_reset)(void);
} sign_of_life_ops;

#ifdef CONFIG_AMAZON_DRV_TEST
extern life_cycle_reason_t lcr_boot;
extern int read_life_cycle_reason(life_reason_type type);
extern int clear_life_cycle_reason(void);
#endif

#endif
