/*
 * idme.c
 *
 * Copyright 2013-2022 Amazon Technologies, Inc. All Rights Reserved.
 *
 * The code contained herein is licensed under the GNU General Public
 * License Version 2.
 * You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/kernel.h>

#define IDME_OF_BOARD_ID	"/idme/board_id"
#define IDME_OF_LCM_ID		"/idme/lcm_id"
#define IDME_OF_PRODUCT_ID2	"/idme/productid2"
#define BOOT_MODE_IDME_PATH	"/idme/bootmode"
#define IDME_OF_ALSCAL			"/idme/alscal"
#define IDME_OF_DEV_FLAGS		 "/idme/dev_flags"

#define IDME_OF_SENSORCAL	"/idme/sensorcal"
#define IDME_OF_TPCGCOLOR	"/idme/tp_cg_color"

#define PRODUCT_FEATURES_DIR "product_features"
#define PRODUCT_FEATURE_NAME_GPS "gps"
#define PRODUCT_FEATURE_NAME_WAN "wan"
#define PRODUCT_FEATURE_NAME_HDMI "hdmi"
#define MAC_SEC_KEY "mac_sec"
#define MAC_SEC_OWNER 1000

#define PRODUCT_FEATURE_STRING_GPS " "
#define PRODUCT_FEATURE_STRING_WAN " "
#define PRODUCT_FEATURE_STRING_SPACE " "

#define SENSOR_CAL_SIZE 96
char gbuffer[SENSOR_CAL_SIZE];

static int idme_proc_show(struct seq_file *seq, void *v)
{
	struct property *pp = (struct property *)seq->private;

	BUG_ON(!pp);

	seq_write(seq, pp->value, pp->length);

	return 0;
}

static int idme_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, idme_proc_show, PDE_DATA(inode));
}

static const struct proc_ops idme_fops = {
	.proc_open = idme_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

bool board_has_wan(void)
{
	struct device_node *ap;
	int len;

	ap = of_find_node_by_path(IDME_OF_BOARD_ID);
	if (ap) {
		const char *boardid = of_get_property(ap, "value", &len);

		if (len >= 2) {
			if (boardid[0] == '0' && boardid[5] == '1')
				return true;
		}
	}

	return false;
}
EXPORT_SYMBOL_GPL(board_has_wan);

static int prodfeat_show(struct seq_file *m, void *v)
{
	seq_printf(m, PRODUCT_FEATURE_STRING_SPACE);
	return 0;
}

static int prodfeat_open(struct inode *inode, struct file *file)
{
	return single_open(file, prodfeat_show, PDE_DATA(inode));
}

bool board_has_hdmi(void)
{
#if defined(CONFIG_MTK_HDMI_SUPPORT)
	return true;
#else
	return false;
#endif /*  defined(CONFIG_MTK_HDMI_SUPPORT) */
}
EXPORT_SYMBOL_GPL(board_has_hdmi);

static const struct proc_ops hdmi_fops = {
	.proc_open = prodfeat_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

unsigned int idme_get_board_type(void)
{
	struct device_node *ap = NULL;
	char board_type[5] = { 0 };
	const char *board_id = NULL;
	unsigned int rs = 0;

	ap = of_find_node_by_path(IDME_OF_BOARD_ID);
	if (ap)
		board_id = (const char *)of_get_property(ap, "value", NULL);
	else
		pr_err("of_find_node_by_path failed\n");

	strlcpy(board_type, board_id, sizeof(board_type));
	if (unlikely(kstrtouint(board_type, 16, &rs)))
		pr_err("%s kstrtouint failed!\v", __func__);

	return rs;
}
EXPORT_SYMBOL_GPL(idme_get_board_type);

static s8 castChar(s8 c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return (s8)255;
}

s8 *hexToBytes(s8 *inhex)
{
	s8 *retval;
	s8 *p;
	int len, i;

	len = strlen(inhex) / 2;
	retval = gbuffer;
	for (i = 0, p = (uint8_t *) inhex; i < len; i++) {
		retval[i] = (castChar(*p) << 4) | castChar(*(p+1));
		p += 2;
	}
	retval[len] = 0;
	return retval;
}

unsigned int idme_get_sensorcal(s16 *data, uint8_t size)
{
	struct device_node *ap = NULL;
	char *sensorcal = NULL;
	unsigned int rs = 0;
	uint8_t i = 0;
	s8 *retdata = NULL;

	pr_debug("%s enter\n", __func__);
	ap = of_find_node_by_path(IDME_OF_SENSORCAL);
	if (ap) {
		sensorcal = (char *)of_get_property(ap, "value", NULL);
		pr_notice("sensorcal %s\n", sensorcal);
	} else {
		pr_err("of_find_node_by_path failed\n");
	}

	if(sensorcal) {
		retdata = hexToBytes(sensorcal);
	} else {
		pr_err("sensorcal is NULL!\n");
		return -1;
	}

	if (!retdata) {
		pr_notice("retdata %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n",
		retdata[0], retdata[1], retdata[2], retdata[3], retdata[4], retdata[5],
		retdata[6], retdata[7], retdata[8], retdata[9], retdata[10], retdata[11]);
	}

	if (size > 6)
		size = 6;
	for (i = 0; i < size; i++)
		data[i] = (s16)((retdata[i*2] & 0x00FF) | (retdata[i*2+1] << 8));

	pr_debug("%s exit\n", __func__);

	return rs;
}
EXPORT_SYMBOL_GPL(idme_get_sensorcal);

unsigned int idme_get_board_rev(void)
{
	struct device_node *ap = NULL;
	char board_rev[3] = { 0 };
	const char *board_id = NULL;
	unsigned int rs = 0;

	ap = of_find_node_by_path(IDME_OF_BOARD_ID);
	if (ap)
		board_id = (const char *)of_get_property(ap, "value", NULL);
	else
		pr_err("of_find_node_by_path failed\n");

	strlcpy(board_rev, (board_id + 7), sizeof(board_rev));

	if (unlikely(kstrtouint(board_rev, 16, &rs)))
		pr_err("%s kstrtouint failed!\v", __func__);

	return rs;
}
EXPORT_SYMBOL_GPL(idme_get_board_rev);

unsigned int idme_get_battery_info(int index, size_t length)
{
	struct device_node *ap = NULL;
	const char *product_id2 = NULL;
	char battery_info[32] = {0};
	unsigned int rs = 0;

	ap = of_find_node_by_path(IDME_OF_PRODUCT_ID2);
	if (ap)
		product_id2 = (const char *)of_get_property(ap, "value", NULL);
	else
		pr_err("of_find_node_by_path failed\n");

	strlcpy(battery_info, product_id2 + 31 - index, length+1);
	if (unlikely(kstrtouint(battery_info, 16, &rs)))
		pr_err("%s kstrtouint failed, index = %d, length = %d!\v", __func__, index, (int)length);

	return rs;
}
EXPORT_SYMBOL_GPL(idme_get_battery_info);

unsigned int idme_get_lcmid_info(void)
{
	struct device_node *ap = NULL;
	const char *idme_lcm_id = NULL;
	unsigned int rs = 0;
	char char_lcd_id[4] = {0};

	pr_debug("[%s] enter.\n", __func__);

	ap = of_find_node_by_path(IDME_OF_LCM_ID);
	if (ap) {
		idme_lcm_id = (const char *)of_get_property(ap, "value", NULL);
	} else {
		pr_err("of_find_node_by_path failed\n");
		return 0;
	}

	strlcpy(char_lcd_id, idme_lcm_id, sizeof(char_lcd_id));
	if (unlikely(kstrtouint(char_lcd_id, 16, &rs)))
		pr_err("%s kstrtouint failed.\v");

	pr_debug("[%s] idme_lcm_id=%d, ext.\n", __func__, rs);

	return rs;
}
EXPORT_SYMBOL_GPL(idme_get_lcmid_info);

unsigned int idme_get_bootmode(void)
{
	struct device_node *idme_node;
	int len;

	idme_node = of_find_node_by_path(BOOT_MODE_IDME_PATH);
	if (idme_node) {
		const char *bootmode = of_get_property(idme_node, "value", &len);

		if (len > 0)
			return bootmode[0] - '0';
	}

	return 0;
}
EXPORT_SYMBOL_GPL(idme_get_bootmode);

unsigned int idme_get_tp_cg_color(void)
{
	struct device_node *ap = NULL;
	char tp_cg_type[8] = { 0 };
	const char *tp_cg = NULL;
	unsigned int rs = 0;

	ap = of_find_node_by_path(IDME_OF_TPCGCOLOR);
	if (ap) {
		tp_cg = (const char *)of_get_property(ap, "value", NULL);
	} else {
		pr_err("of_find_node_by_path failed,set default value 0\n");
		return 0;
	}

	if ((int)strlen(tp_cg) == 0) {
		pr_info("tp_cg_color len = 0,set default value 0\n");
		return 0;
	}
	strlcpy(tp_cg_type, tp_cg, sizeof(tp_cg_type));
	if (unlikely(kstrtouint(tp_cg_type, 16, &rs)))
		pr_err("%s kstrtouint failed!\v", __func__);

	pr_info("tp_cg_value=%d\n", rs);
	return rs;
}
EXPORT_SYMBOL_GPL(idme_get_tp_cg_color);

unsigned int idme_get_alscal_value(void)
{
	struct device_node *ap = NULL;
	char *alscal = NULL;
	s8 *retdata = NULL;
	unsigned int alscal_value = 0;

	pr_info("%s enter\n", __func__);
	ap = of_find_node_by_path(IDME_OF_ALSCAL);
	if (ap) {
		alscal = (char *)of_get_property(ap, "value", NULL);
		pr_info("alscal %s\n", alscal);
	} else {
		pr_err("of_find_node_by_path failed\n");
	}

	retdata = hexToBytes(alscal);
	if (!retdata)
		pr_info("retdata %02x, %02x, %02x, %02x\n", retdata[0], retdata[1], retdata[2], retdata[3]);
	pr_info("retdata %02x, %02x, %02x, %02x\n", retdata[0], retdata[1], retdata[2], retdata[3]);

	alscal_value |= (unsigned int)(retdata[0] & 0x000000FF);
	alscal_value |= (unsigned int)((retdata[1] & 0x000000FF) << 8);
	alscal_value |= (unsigned int)((retdata[2] & 0x000000FF) << 16);
	alscal_value |= (unsigned int)((retdata[3] & 0x000000FF) << 24);

	pr_info("alscal_value %d\n", alscal_value);
	pr_info("%s exit\n", __func__);

	return alscal_value;
}
EXPORT_SYMBOL_GPL(idme_get_alscal_value);

u64 idme_get_dev_flags_value(void)
{
	struct device_node *ap = NULL;
	char *devflags = NULL;
	int res;
	u64 ret;

	ap = of_find_node_by_path(IDME_OF_DEV_FLAGS);
	if (ap) {
		devflags = (char *)of_get_property(ap, "value", NULL);
		pr_info("dev_flags %s\n", devflags);
	} else {
		pr_err("of_find_node_by_path failed\n");
	}


	res = kstrtoull(devflags, 16, &ret);
	if (res)
		ret = 0;
	pr_info("dev_flags: %llu\n", ret);

	return ret;
}
EXPORT_SYMBOL_GPL(idme_get_dev_flags_value);

static int __init idme_init(void)
{
	struct proc_dir_entry *proc_idme = NULL;
	struct device_node *root = NULL, *child = NULL;
	struct property *pp_value = NULL;
	int perm = 0;
	static struct proc_dir_entry *proc_product_features_dir;
	struct proc_dir_entry *child_pde = NULL;
	bool access_restrict = false;

	root = of_find_node_by_path("/idme");
	if (!root)
		return -EINVAL;

	/* Create the root IDME procfs node */
	proc_idme = proc_mkdir("idme", NULL);
	if (!proc_idme) {
		of_node_put(root);
		return -ENOMEM;
	}

	/* Populate each IDME field */
	for (child = NULL; (child = of_get_next_child(root, child)); ) {
		pp_value = of_find_property(child, "value", NULL);

		if (strcmp(child->name, MAC_SEC_KEY) == 0)
			access_restrict = true;
		else
			access_restrict = false;

		if (!pp_value)
			continue;

		if (of_property_read_u32(child, "permission", &perm))
			continue;

		/* These values aren't writable anyways */
		perm &= ~(S_IWUGO);

		if (access_restrict)
			perm = 0400;

		child_pde = proc_create_data(child->name, perm, proc_idme,
			&idme_fops, pp_value);

		if (child_pde && access_restrict)
			proc_set_user(child_pde, KUIDT_INIT(MAC_SEC_OWNER), KGIDT_INIT(0));
	}

	of_node_put(child);
	of_node_put(root);

	proc_product_features_dir = proc_mkdir("product_features", NULL);
	if (proc_product_features_dir) {
		if (board_has_hdmi()) {
			proc_create(PRODUCT_FEATURE_NAME_HDMI,
					S_IRUGO,
					proc_product_features_dir,
					&hdmi_fops);
		}
	}


	return 0;
}

fs_initcall(idme_init);
MODULE_DESCRIPTION("Amazon idme driver");
MODULE_LICENSE("GPL v2");
