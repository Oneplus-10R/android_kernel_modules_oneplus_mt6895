// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 oplus. All rights reserved.
 */

#include "../touchpanel_common.h"
#include "himax_common.h"
#include <linux/module.h>
/*******Part0:LOG TAG Declear********************/

#ifdef TPD_DEVICE
#undef TPD_DEVICE
#define TPD_DEVICE "himax_common"
#else
#define TPD_DEVICE "himax_common"
#endif

/*******Part1:Call Back Function implement*******/
int hx_test_data_pop_out(struct touchpanel_data *ts,
			 struct auto_testdata *hx_testdata)
{
	char *line = "=================================================\n";
	char *Company = "Himax for OPLUS: Driver Seneor Test\n";
	char *Info = "Test Info as follow\n";
	char *project_name_log = "OPLUS_";
	char *g_project_test_info_log = NULL;
	char *g_Company_info_log = NULL;
	#if LINUX_VERSION_CODE>= KERNEL_VERSION(5, 10, 0)
	#else
		mm_segment_t fs;
		loff_t pos = 0;
	#endif
	int ret_val = NO_ERR;

	TPD_INFO("%s: Entering!\n", __func__);

	g_Company_info_log = tp_devm_kzalloc(&ts->s_client->dev, 256, GFP_KERNEL);
	g_project_test_info_log = tp_devm_kzalloc(&ts->s_client->dev, 256, GFP_KERNEL);
	if ((g_Company_info_log == NULL) || (g_project_test_info_log == NULL)) {
		TPD_INFO("%s alloc error \n", __func__);
		ret_val = -1;
		goto SAVE_DATA_ERR;
	}
	/*Company Info*/
	snprintf(g_Company_info_log, 160, "%s%s%s%s", line, Company, Info, line);
	TPD_DETAIL("%s 000: %s \n", __func__, g_Company_info_log);

	/*project Info*/
	#if LINUX_VERSION_CODE>= KERNEL_VERSION(5, 10, 0)
		snprintf(g_project_test_info_log, 118,
			"Project_name: %s\nFW_ID: %8llX\nFW_Ver: %4llX\nPanel Info: TX_Num=%d RX_Num=%d\nTest stage: Mobile\n",
			project_name_log, hx_testdata->tp_fw, hx_testdata->dev_tp_fw,
			ts->hw_res.tx_num, ts->hw_res.rx_num);
		TPD_DETAIL("%s 001: %s \n", __func__, g_project_test_info_log);
	#else
		snprintf(g_project_test_info_log, 118,
			 "Project_name: %s%d\nFW_ID: %8X\nFW_Ver: %4X\nPanel Info: TX_Num=%d RX_Num=%d\nTest stage: Mobile\n",
			 project_name_log, get_project(), hx_testdata->tp_fw, hx_testdata->dev_tp_fw,
			 ts->hw_res.tx_num, ts->hw_res.rx_num);
	#endif

	if (IS_ERR(hx_testdata->fp)) {
		TPD_INFO("%s open file failed = %ld\n", __func__, PTR_ERR(hx_testdata->fp));
		ret_val = -EIO;
		goto SAVE_DATA_ERR;
	}
	#if LINUX_VERSION_CODE>= KERNEL_VERSION(5, 10, 0)
	#else
		fs = get_fs();
		set_fs(get_ds());
		vfs_write(hx_testdata->fp, g_Company_info_log,
			  (int)(strlen(g_Company_info_log)), &pos);
		pos = pos + (int)(strlen(g_Company_info_log));

		vfs_write(hx_testdata->fp, g_project_test_info_log,
				(int)(strlen(g_project_test_info_log)), &pos);
		pos = pos + (int)(strlen(g_project_test_info_log));

		vfs_write(hx_testdata->fp, hx_testdata->test_list_log,
				(int)(strlen(hx_testdata->test_list_log)), &pos);
		pos = pos + (int)(strlen(hx_testdata->test_list_log));

		set_fs(fs);
	#endif

SAVE_DATA_ERR:

	if (g_project_test_info_log) {
		tp_devm_kfree(&ts->s_client->dev, (void **)&g_project_test_info_log, 256);
		g_project_test_info_log = NULL;
	}

	if (g_Company_info_log) {
		tp_devm_kfree(&ts->s_client->dev, (void **)&g_Company_info_log, 256);
		g_Company_info_log = NULL;
	}

	TPD_INFO("%s: End!\n", __func__);
	return ret_val;
}

static int hx_testdata_init(struct touchpanel_data *ts,
			    struct auto_testdata *hx_testdata)
{
	const struct firmware *fw = NULL;
	struct auto_test_header *test_head = NULL;
	uint32_t *p_data32 = NULL;
	char *fw_name;
	char *fw_name_test;
	char *p_node = NULL;
	char *postfix = "_TEST.img";
	uint8_t copy_len = 0;
	int ret = 0;

	TPD_INFO("%s: enter\n", __func__);
	fw = ts->com_test_data.limit_fw;
	/*step4: decode the limit image*/
	test_head = (struct auto_test_header *)fw->data;
	p_data32 = (uint32_t *)(fw->data + 16);

	if ((test_head->magic1 != Limit_MagicNum1)
			|| (test_head->magic2 != Limit_MagicNum2)) {
		TPD_INFO("limit image is not generated by oplus\n");
		return  -1;
	}

	TPD_INFO("current test item: %llx\n", test_head->test_item);
	/*step4:init hx_testdata*/
	hx_testdata->fw = fw;
	hx_testdata->fp = ts->com_test_data.result_data;
	hx_testdata->length = ts->com_test_data.result_max_len;
	hx_testdata->pos = &ts->com_test_data.result_cur_len;
	hx_testdata->tx_num = ts->hw_res.tx_num;
	hx_testdata->rx_num = ts->hw_res.rx_num;
	hx_testdata->irq_gpio = ts->hw_res.irq_gpio;
	hx_testdata->key_tx = ts->hw_res.key_tx;
	hx_testdata->key_rx = ts->hw_res.key_rx;
	hx_testdata->tp_fw = ts->panel_data.tp_fw;
	fw_name = ts->panel_data.fw_name;

	hx_testdata->test_list_log = tp_devm_kzalloc(&ts->s_client->dev, 256,
				     GFP_KERNEL);

	if (hx_testdata->test_list_log == NULL) {
		TPD_INFO("test_list_log kzalloc error!\n");
		return -1;
	}

	fw_name_test = tp_devm_kzalloc(&ts->s_client->dev, MAX_FW_NAME_LENGTH,
				       GFP_KERNEL);

	if (fw_name_test == NULL) {
		TPD_INFO("fw_name_test kzalloc error!\n");
		return -1;
	}

	p_node = strstr(fw_name, ".");
	copy_len = p_node - fw_name;
	memcpy(fw_name_test, fw_name, copy_len);
	strlcat(fw_name_test, postfix, MAX_FW_NAME_LENGTH);
	TPD_INFO("%s : fw_name_test is %s\n", __func__, fw_name_test);

	ret = request_firmware(&hx_testdata->fw_test, fw_name_test, ts->dev);

	if (ret < 0) {
		TP_INFO(ts->tp_index, "Request test firmware failed - %s (%d)\n", fw_name_test,
			ret);
	}

	if (fw_name_test) {
		tp_devm_kfree(&ts->s_client->dev, (void **)&fw_name_test, MAX_FW_NAME_LENGTH);
		fw_name_test = NULL;
	}

	return ret;
}

int hx_auto_test(struct seq_file *s,  struct touchpanel_data *ts)
{
	struct himax_proc_operations *hx_ops;
	int ret = 0;

	struct auto_testdata hx_testdata = {
		.tx_num = 0,
		.rx_num = 0,
		.irq_gpio = -1,
		.key_tx = 0,
		.key_rx = 0,
		.tp_fw = 0,
		.dev_tp_fw = 0,
		.fw = NULL,
		.fw_test = NULL,
		.test_list_log = NULL,
		.list_write_count = 0,
		.pos = NULL,
	};

	if (!ts) {
		return -1;
	}

	hx_ops = (struct himax_proc_operations *)ts->private_data;

	if (!hx_ops) {
		return -1;
	}

	/*Init Log Data */
	ret = hx_testdata_init(ts, &hx_testdata);

	if (ret < 0) {
		goto RET_OUT;
	}

	/*step1:disable_irq && get mutex locked*/
	if (!ts->is_noflash_ic) {
		disable_irq_nosync(ts->client->irq);

	} else {
		disable_irq_nosync(ts->s_client->irq);
	}

	/*mutex_lock(&ts->mutex);*/

	ret = hx_ops->test_prepare(ts->chip_data, &hx_testdata);

	if (ret < 0) {
		goto RET_OUT;
	}

	ret = hx_ops->int_pin_test(s, ts->chip_data, &hx_testdata);
	ret += hx_ops->self_test(s, ts->chip_data, &hx_testdata);

	hx_test_data_pop_out(ts, &hx_testdata);

	/*Save Log Data */

	seq_printf(s, "imageid = 0x%llx, deviceid = 0x%llx\n", hx_testdata.tp_fw,
		   hx_testdata.dev_tp_fw);
	seq_printf(s, "%d error(s). %s\n", ret, ret ? "" : "All test passed.");
	TPD_INFO(" TP auto test %d error(s). %s\n", ret, ret ? "" : "All test passed.");

	if (ts->auto_test_force_pass_support) {
		seq_printf(s, "0");
	}

RET_OUT:

	if (hx_testdata.test_list_log) {
		tp_devm_kfree(&ts->s_client->dev, (void **)&hx_testdata.test_list_log, 256);
		hx_testdata.test_list_log = NULL;
	}

	return ret;
}
EXPORT_SYMBOL(hx_auto_test);

int hx_black_screen_test(struct black_gesture_test *p_black_gesture_test,
			 struct touchpanel_data *ts)
{
	struct himax_proc_operations *hx_ops;
	int ret = 0;

	struct auto_testdata hx_testdata = {
		.tx_num = 0,
		.rx_num = 0,
		.irq_gpio = -1,
		.key_tx = 0,
		.key_rx = 0,
		.tp_fw = 0,
		.dev_tp_fw = 0,
		.fw = NULL,
		.fw_test = NULL,
		.test_list_log = NULL,
		.list_write_count = 0,
		.pos = NULL,
	};

	if (!ts) {
		return -1;
	}

	hx_ops = (struct himax_proc_operations *)ts->private_data;

	if (!hx_ops) {
		return -1;
	}

	/*Init Log Data */
	ret = hx_testdata_init(ts, &hx_testdata);

	if (ret < 0) {
		goto RET_OUT;
	}

	ret = hx_ops->test_prepare(ts->chip_data, &hx_testdata);

	if (ret < 0) {
		goto RET_OUT;
	}

	ret = hx_ops->blackscreen_test(ts->chip_data, p_black_gesture_test->message,
				       p_black_gesture_test->message_size, &hx_testdata);

	hx_test_data_pop_out(ts, &hx_testdata);
	/*Save Log Data */
	snprintf(p_black_gesture_test->message, p_black_gesture_test->message_size,
		 "%s%d errors. %s", p_black_gesture_test->message, ret,
		 ret ? "" : "All test passed.");
	TPD_INFO("%d errors. %s\n", ret, ret ? "" : "All test passed.");

	hx_ops->test_finish(ts->chip_data);

	/*mutex_unlock(&ts->mutex);*/
RET_OUT:

	if (hx_testdata.test_list_log) {
		tp_devm_kfree(&ts->s_client->dev, (void **)&hx_testdata.test_list_log, 256);
		hx_testdata.test_list_log = NULL;
	}

	return ret;
}
EXPORT_SYMBOL(hx_black_screen_test);

#define HX_TP_PROC_REGISTER
#define HX_TP_PROC_DEBUG
#define HX_TP_PROC_FLASH_DUMP
#define HX_TP_PROC_SELF_TEST
#define HX_TP_PROC_RESET
#define HX_TP_PROC_SENSE_ON_OFF
#define HX_TP_PROC_2T2R


static ssize_t himax_proc_register_read(struct file *file, char *buff,
					size_t len, loff_t *pos)
{
	struct touchpanel_data *ts = PDE_DATA(file_inode(file));
	struct himax_proc_operations *hx_ops;
	ssize_t ret = 0;

	if (!ts) {
		return 0;
	}

	hx_ops = (struct himax_proc_operations *)ts->private_data;

	if (!hx_ops) {
		return 0;
	}

	if (!hx_ops->himax_proc_register_read) {
		if (copy_to_user(buff, "Not support auto-test proc node\n",
				 strlen("Not support auto-test proc node\n"))) {
			TPD_INFO("%s,here:%d\n", __func__, __LINE__);
		}

		return 0;
	}

	mutex_lock(&ts->mutex);
	ret = hx_ops->himax_proc_register_read(file, buff, len, pos);
	mutex_unlock(&ts->mutex);
	return ret;
}

static ssize_t himax_proc_register_write(struct file *file, const char *buff,
		size_t len, loff_t *pos)
{
	struct touchpanel_data *ts = PDE_DATA(file_inode(file));
	struct himax_proc_operations *hx_ops;
	ssize_t ret = 0;

	if (!ts) {
		return 0;
	}

	hx_ops = (struct himax_proc_operations *)ts->private_data;

	if (!hx_ops) {
		return 0;
	}

	if (!hx_ops->himax_proc_register_write) {
		TPD_INFO("Not support auto-test proc node %s %d\n", __func__, __LINE__);
		return 0;
	}

	mutex_lock(&ts->mutex);
	ret = hx_ops->himax_proc_register_write(file, buff, len, pos);
	mutex_unlock(&ts->mutex);
	return ret;
}

#if LINUX_VERSION_CODE>= KERNEL_VERSION(5, 10, 0)

static const struct proc_ops himax_proc_register_ops = {
	.proc_open = simple_open,
	.proc_read = himax_proc_register_read,
	.proc_write = himax_proc_register_write,
};
#else
static struct file_operations himax_proc_register_ops = {
	.owner = THIS_MODULE,
	.read = himax_proc_register_read,
	.write = himax_proc_register_write,
};
#endif

#if LINUX_VERSION_CODE>= KERNEL_VERSION(5, 10, 0)
static int himax_proc_diag_read(struct seq_file *s, void *v)
#else
static ssize_t himax_proc_diag_read(struct file *file, char *buff, size_t len,
					loff_t *pos)
#endif
{
	#if LINUX_VERSION_CODE>= KERNEL_VERSION(5, 10, 0)
		struct touchpanel_data *ts = s->private;
	#else
		struct touchpanel_data *ts = PDE_DATA(file_inode(file));
	#endif
	struct himax_proc_operations *hx_ops;
	ssize_t ret = 0;

	if (!ts) {
		return 0;
	}

	hx_ops = (struct himax_proc_operations *)ts->private_data;

	if (!hx_ops) {
		return 0;
	}
	#if LINUX_VERSION_CODE>= KERNEL_VERSION(5, 10, 0)
		if (!hx_ops->himax_proc_diag_read) {
			seq_printf(s, "Not support auto-test proc node\n");
			TPD_INFO("%s,here:%d\n", __func__, __LINE__);
			return 0;
		}
		ret = hx_ops->himax_proc_diag_read(s, v);
	#else
		if (!hx_ops->himax_proc_diag_read) {
			if (copy_to_user(buff, "Not support auto-test proc node\n",
					strlen("Not support auto-test proc node\n"))) {
				TPD_INFO("%s,here:%d\n", __func__, __LINE__);
			}

			return 0;
		}
		ret = hx_ops->himax_proc_diag_read(file, buff, len, pos);
	#endif
	return ret;
}

static ssize_t himax_proc_diag_write(struct file *file, const char *buff,
				     size_t len, loff_t *pos)
{
	struct touchpanel_data *ts = PDE_DATA(file_inode(file));
	struct himax_proc_operations *hx_ops;
	ssize_t ret = 0;

	if (!ts) {
		return 0;
	}

	hx_ops = (struct himax_proc_operations *)ts->private_data;

	if (!hx_ops) {
		return 0;
	}

	if (!hx_ops->himax_proc_diag_write) {
		TPD_INFO("Not support auto-test proc node %s %d\n", __func__, __LINE__);
		return 0;
	}

	ret = hx_ops->himax_proc_diag_write(file, buff, len, pos);
	return ret;
}


#if LINUX_VERSION_CODE>= KERNEL_VERSION(5, 10, 0)
static int himax_proc_diag_open(struct inode *inode, struct file *file)
{
	return single_open(file, himax_proc_diag_read, PDE_DATA(inode));
}

static const struct proc_ops himax_proc_diag_ops = {
	.proc_open = himax_proc_diag_open,
	.proc_write = himax_proc_diag_write,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static struct file_operations himax_proc_diag_ops = {
	.owner = THIS_MODULE,
	.read = himax_proc_diag_read,
	.write = himax_proc_diag_write,
};
#endif

static ssize_t himax_proc_DD_debug_read(struct file *file, char *buff,
					size_t len, loff_t *pos)
{
	struct touchpanel_data *ts = PDE_DATA(file_inode(file));
	struct himax_proc_operations *hx_ops;
	ssize_t ret = 0;

	if (!ts) {
		return 0;
	}

	hx_ops = (struct himax_proc_operations *)ts->private_data;

	if (!hx_ops) {
		return 0;
	}

	if (!hx_ops->himax_proc_DD_debug_read) {
		if (copy_to_user(buff, "Not support auto-test proc node\n",
				 strlen("Not support auto-test proc node\n"))) {
			TPD_INFO("%s,here:%d\n", __func__, __LINE__);
		}

		return 0;
	}

	ret = hx_ops->himax_proc_DD_debug_read(file, buff, len, pos);
	return ret;
}

static ssize_t himax_proc_DD_debug_write(struct file *file, const char *buff,
		size_t len, loff_t *pos)
{
	struct touchpanel_data *ts = PDE_DATA(file_inode(file));
	struct himax_proc_operations *hx_ops;
	ssize_t ret = 0;

	if (!ts) {
		return 0;
	}

	hx_ops = (struct himax_proc_operations *)ts->private_data;

	if (!hx_ops) {
		return 0;
	}

	if (!hx_ops->himax_proc_DD_debug_write) {
		TPD_INFO("Not support dd proc node %s %d\n", __func__, __LINE__);
		return 0;
	}

	ret = hx_ops->himax_proc_DD_debug_write(file, buff, len, pos);
	return ret;
}


#if LINUX_VERSION_CODE>= KERNEL_VERSION(5, 10, 0)

static const struct proc_ops himax_proc_dd_debug_ops = {
	.proc_open = simple_open,
	.proc_read = himax_proc_DD_debug_read,
	.proc_write = himax_proc_DD_debug_write,
};
#else
static struct file_operations himax_proc_dd_debug_ops = {
	.owner = THIS_MODULE,
	.read = himax_proc_DD_debug_read,
	.write = himax_proc_DD_debug_write,
};
#endif

static ssize_t himax_proc_FW_debug_read(struct file *file, char *buff,
					size_t len, loff_t *pos)
{
	struct touchpanel_data *ts = PDE_DATA(file_inode(file));
	struct himax_proc_operations *hx_ops;
	ssize_t ret = 0;

	if (!ts) {
		return 0;
	}

	hx_ops = (struct himax_proc_operations *)ts->private_data;

	if (!hx_ops) {
		return 0;
	}

	if (!hx_ops->himax_proc_FW_debug_read) {
		TPD_INFO("Not support fw proc node %s %d\n", __func__, __LINE__);
		return 0;
	}

	ret = hx_ops->himax_proc_FW_debug_read(file, buff, len, pos);
	return ret;
}

#if LINUX_VERSION_CODE>= KERNEL_VERSION(5, 10, 0)

static const struct proc_ops himax_proc_fw_debug_ops = {
	.proc_open = simple_open,
	.proc_read = himax_proc_FW_debug_read,
};
#else
static struct file_operations himax_proc_fw_debug_ops = {
	.owner = THIS_MODULE,
	.read = himax_proc_FW_debug_read,
};
#endif
static ssize_t himax_proc_reset_write(struct file *file, const char *buff,
				      size_t len, loff_t *pos)
{
	struct touchpanel_data *ts = PDE_DATA(file_inode(file));
	struct himax_proc_operations *hx_ops;
	ssize_t ret = 0;

	if (!ts) {
		return 0;
	}

	hx_ops = (struct himax_proc_operations *)ts->private_data;

	if (!hx_ops) {
		return 0;
	}

	if (!hx_ops->himax_proc_reset_write) {
		TPD_INFO("Not support reset proc node %s %d\n", __func__, __LINE__);
		return 0;
	}

	ret = hx_ops->himax_proc_reset_write(file, buff, len, pos);
	return ret;
}

#if LINUX_VERSION_CODE>= KERNEL_VERSION(5, 10, 0)

static const struct proc_ops himax_proc_reset_ops = {
	.proc_open = simple_open,
	.proc_write = himax_proc_reset_write,
};
#else
static struct file_operations himax_proc_reset_ops = {
	.owner = THIS_MODULE,
	.write = himax_proc_reset_write,
};
#endif

static ssize_t himax_proc_sense_on_off_write(struct file *file,
		const char *buff, size_t len, loff_t *pos)
{
	struct touchpanel_data *ts = PDE_DATA(file_inode(file));
	struct himax_proc_operations *hx_ops;
	ssize_t ret = 0;

	if (!ts) {
		return 0;
	}

	hx_ops = (struct himax_proc_operations *)ts->private_data;

	if (!hx_ops) {
		return 0;
	}

	if (!hx_ops->himax_proc_sense_on_off_write) {
		TPD_INFO("Not support senseonoff proc node %s %d\n", __func__, __LINE__);
		return 0;
	}

	ret = hx_ops->himax_proc_sense_on_off_write(file, buff, len, pos);
	return ret;
}

#if LINUX_VERSION_CODE>= KERNEL_VERSION(5, 10, 0)

static const struct proc_ops himax_proc_sense_on_off_ops = {
	.proc_open = simple_open,
	.proc_write = himax_proc_sense_on_off_write,
};
#else
static struct file_operations himax_proc_sense_on_off_ops = {
	.owner = THIS_MODULE,
	.write = himax_proc_sense_on_off_write,
};
#endif

int himax_create_proc(struct touchpanel_data *ts,
		      struct himax_proc_operations *hx_ops)
{
	int ret = 0;

	/* touchpanel_auto_test interface*/
	struct proc_dir_entry *prEntry_himax = NULL;
	struct proc_dir_entry *prEntry_tmp = NULL;
	ts->private_data = hx_ops;

	if (prEntry_tmp == NULL) {
		ret = -ENOMEM;
		TPD_INFO("%s: Couldn't create proc entry, %d\n", __func__, __LINE__);
	}

	/*proc files:/proc/touchpanel/debug_info/himax*/
	prEntry_himax = proc_mkdir("himax", ts->prEntry_debug_tp);

	if (prEntry_himax == NULL) {
		ret = -ENOMEM;
		TPD_INFO("%s: Couldn't create /proc/touchpanel/debug_info/himax proc entry\n",
			 __func__);
	}

	/*proc files: /proc/touchpanel/debug_info/himax/register*/
	prEntry_tmp = proc_create_data("register", 0666, prEntry_himax,
				       &himax_proc_register_ops, ts);

	if (prEntry_tmp == NULL) {
		ret = -ENOMEM;
		TPD_INFO("%s: Couldn't create proc entry, %d\n", __func__, __LINE__);
	}

	/*proc files: /proc/touchpanel/debug_info/himax/diag*/
	prEntry_tmp = proc_create_data("diag", 0666, prEntry_himax,
				       &himax_proc_diag_ops, ts);

	if (prEntry_tmp == NULL) {
		ret = -ENOMEM;
		TPD_INFO("%s: Couldn't create proc entry, %d\n", __func__, __LINE__);
	}

	/*proc files: /proc/touchpanel/debug_info/himax/dd*/
	prEntry_tmp = proc_create_data("dd", 0666, prEntry_himax,
				       &himax_proc_dd_debug_ops, ts);

	if (prEntry_tmp == NULL) {
		ret = -ENOMEM;
		TPD_INFO("%s: Couldn't create proc entry, %d\n", __func__, __LINE__);
	}

	/*proc files: /proc/touchpanel/debug_info/himax/fw*/
	prEntry_tmp = proc_create_data("fw", 0666, prEntry_himax,
				       &himax_proc_fw_debug_ops, ts);

	if (prEntry_tmp == NULL) {
		ret = -ENOMEM;
		TPD_INFO("%s: Couldn't create proc entry, %d\n", __func__, __LINE__);
	}

	/*proc files: /proc/touchpanel/debug_info/himax/reset*/
	prEntry_tmp = proc_create_data("reset", 0666, prEntry_himax,
				       &himax_proc_reset_ops, ts);

	if (prEntry_tmp == NULL) {
		ret = -ENOMEM;
		TPD_INFO("%s: Couldn't create proc entry, %d\n", __func__, __LINE__);
	}

	/*proc files: /proc/touchpanel/debug_info/himax/senseonoff*/
	prEntry_tmp = proc_create_data("senseonoff", 0666, prEntry_himax,
				       &himax_proc_sense_on_off_ops, ts);

	if (prEntry_tmp == NULL) {
		ret = -ENOMEM;
		TPD_INFO("%s: Couldn't create proc entry, %d\n", __func__, __LINE__);
	}

	return ret;
}
EXPORT_SYMBOL(himax_create_proc);

MODULE_DESCRIPTION("Touchscreen Himax Common Interface");
MODULE_LICENSE("GPL");