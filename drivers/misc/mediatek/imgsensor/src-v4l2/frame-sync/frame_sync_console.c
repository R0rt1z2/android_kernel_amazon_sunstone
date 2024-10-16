// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */


#include "frame_sync_console.h"





/******************************************************************************/
// Log message => see frame_sync_def.h
/******************************************************************************/
#define REDUCE_FS_CON_LOG
#define PFX "FrameSyncConsole"
/******************************************************************************/


// static struct device **pdev;





/******************************************************************************/
// frame sync console define
/******************************************************************************/
#define SHOW(buf, len, fmt, ...) { \
	len += snprintf(buf + len, PAGE_SIZE - len, fmt, ##__VA_ARGS__); \
}


#define FS_CON_CMD_SHIFT 100000000

enum fs_console_cmd_id {
	FS_CON_FORCE_DIS_SYNC = 1,
	FS_CON_SET_SYNC_TYPE = 2, /* TBD */

	FS_CON_AUTO_LISTEN_EXT_VSYNC = 40,
	FS_CON_FORCE_LISTEN_EXT_VSYNC = 41,
	/* last one (max value is 42) */
	FS_CON_LOG_TRACER = 42
};
/******************************************************************************/





/******************************************************************************/
// frame sync user/internal console control variables
/******************************************************************************/
/* --- frame sync user control variables --- */
/* control enable/disable some log */
unsigned int log_tracer;

/* force disable frame-sync / set sync */
static unsigned int force_dis_fs;

/* disable algo auto listen ext vsync */
static unsigned int auto_listen_ext_vsync;

/* listen ext vsync (control by user) */
static unsigned int listen_ext_vsync;


/* --- frame sync internal control variables --- */
/* listen ext vsync (control by algorithm) */
static unsigned int listen_vsync_alg;
/******************************************************************************/





/******************************************************************************/
// basic/utility function
/******************************************************************************/
static inline void fs_console_init_def_value(void)
{
	// *pdev = NULL;
	log_tracer = LOG_TRACER_DEF;
	force_dis_fs = 0;
	auto_listen_ext_vsync = ALGO_AUTO_LISTEN_VSYNC;

	// two stage frame-sync:
	// => use seninf-worker to trigger frame length calculation
	listen_ext_vsync = TWO_STAGE_FS;
	listen_vsync_alg = 0;
}


static inline enum fs_console_cmd_id fs_console_get_cmd_id(unsigned int cmd)
{
	return (enum fs_console_cmd_id)(cmd/FS_CON_CMD_SHIFT);
}


static inline unsigned int fs_console_get_cmd_value(unsigned int cmd)
{
	return (cmd%FS_CON_CMD_SHIFT);
}
/******************************************************************************/





/******************************************************************************/
// function used internally by frame-sync
/******************************************************************************/
unsigned int fs_con_check_usr_disable_sync(void)
{
	return force_dis_fs;
}


unsigned int fs_con_get_usr_listen_ext_vsync(void)
{
	return listen_ext_vsync;
}


unsigned int fs_con_get_usr_auto_listen_ext_vsync(void)
{
	return auto_listen_ext_vsync;
}


unsigned int fs_con_get_listen_vsync_alg_cfg(void)
{
	return (auto_listen_ext_vsync) ? listen_vsync_alg : 0;
}


void fs_con_set_listen_vsync_alg_cfg(unsigned int flag)
{
	listen_vsync_alg = flag;
}
/******************************************************************************/





/******************************************************************************/
// console command function
/******************************************************************************/
static void fs_console_set_log_tracer(unsigned int cmd)
{
	int len = 0;
	char str_buf[PAGE_SIZE] = {0};

	log_tracer = fs_console_get_cmd_value(cmd);

	SHOW(str_buf, len,
		"\n[fsync_console] set log_tracer to %u\n",
		log_tracer);
}


static void fs_console_set_force_disable_frame_sync(unsigned int cmd)
{
	int len = 0;
	char str_buf[PAGE_SIZE] = {0};

	force_dis_fs = fs_console_get_cmd_value(cmd);

	SHOW(str_buf, len,
		"\n[fsync_console] set force_dis_fs to %u\n",
		force_dis_fs);
}


static void fs_console_set_algo_auto_listen_ext_vsync(unsigned int cmd)
{
	auto_listen_ext_vsync = fs_console_get_cmd_value(cmd);
}


static void fs_console_set_force_listen_ext_vsync(unsigned int cmd)
{
	listen_ext_vsync = fs_console_get_cmd_value(cmd);
}
/******************************************************************************/





/******************************************************************************/
// console entry function
/******************************************************************************/
static ssize_t fsync_console_show(
	struct device *dev, struct device_attribute *attr,
	char *buf)
{
	int len = 0;


	SHOW(buf, len,
		"\n\t[fsync_console] show all information\n");


	SHOW(buf, len,
		"\t\t[FORCE_DIS_SYNC:%u] force_dis_fs:%u\n",
		(unsigned int)FS_CON_FORCE_DIS_SYNC,
		force_dis_fs);


	SHOW(buf, len,
		"\t\t[SET_SYNC_TYPE:%u] TBD\n",
		(unsigned int)FS_CON_SET_SYNC_TYPE);


	SHOW(buf, len,
		"\t\t[AUTO_LISTEN_EXT_VSYNC:%u] auto_listen_ext_vsync:%u\n",
		(unsigned int)FS_CON_AUTO_LISTEN_EXT_VSYNC,
		auto_listen_ext_vsync);


	SHOW(buf, len,
		"\t\t[FORCE_LISTEN_EXT_VSYNC:%u] listen_ext_vsync:%u\n",
		(unsigned int)FS_CON_FORCE_LISTEN_EXT_VSYNC,
		listen_ext_vsync);


	SHOW(buf, len,
		"\t\t[LOG_TRACER:%u] log_tracer:%u\n",
		(unsigned int)FS_CON_LOG_TRACER,
		log_tracer);


	SHOW(buf, len, "\n");

	return len;
}


static ssize_t fsync_console_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	int ret = 0, len = 0;
	unsigned int cmd = 0;
	enum fs_console_cmd_id cmd_id = 0;
	char str_buf[PAGE_SIZE] = {0};


	/* convert string to unsigned int */
	ret = kstrtouint(buf, 0, &cmd);
	if (ret != 0) {
		SHOW(str_buf, len,
			"\n\t[fsync_console]: kstrtoint failed, input:%s, cmd:%u, ret:%d\n",
			buf,
			cmd,
			ret);
	}


	cmd_id = fs_console_get_cmd_id(cmd);

	switch (cmd_id) {
	case FS_CON_FORCE_DIS_SYNC:
		fs_console_set_force_disable_frame_sync(cmd);
		break;

	case FS_CON_LOG_TRACER:
		fs_console_set_log_tracer(cmd);
		break;

	case FS_CON_AUTO_LISTEN_EXT_VSYNC:
		fs_console_set_algo_auto_listen_ext_vsync(cmd);
		break;

	case FS_CON_FORCE_LISTEN_EXT_VSYNC:
		fs_console_set_force_listen_ext_vsync(cmd);
		break;

	default:
		break;
	}


	return count;
}


static DEVICE_ATTR_RW(fsync_console);

/******************************************************************************/





/******************************************************************************/
// sysfs create/remove file
/******************************************************************************/
int fs_con_create_sysfs_file(struct device *dev)
{
	int ret = 0;


	fs_console_init_def_value();

	if (dev == NULL) {
		LOG_MUST(
			"ERROR: failed to create sysfs file, dev is NULL, return\n");
		return 1;
	}

	// *pdev = dev;

	ret = device_create_file(dev, &dev_attr_fsync_console);
	if (ret) {
		LOG_MUST(
			"ERROR: device_create_file failed, ret:%d\n",
			ret);
	}


#if !defined(REDUCE_FS_CON_LOG)
	LOG_MUST("ret:%d", ret);
#endif // REDUCE_FS_CON_LOG


	return ret;
}


void fs_con_remove_sysfs_file(struct device *dev)
{
	if (dev == NULL) {
		LOG_MUST(
			"ERROR: *pdev is NULL, abort process device_remove_file, return\n");
		return;
	}

	device_remove_file(dev, &dev_attr_fsync_console);


#if !defined(REDUCE_FS_CON_LOG)
	LOG_MUST("sysfile remove\n");
#endif // REDUCE_FS_CON_LOG

}
/******************************************************************************/
