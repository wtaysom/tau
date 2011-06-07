/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/

#define DEBUG 1

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/pagemap.h>
#include <linux/version.h>
#include <asm/uaccess.h>

#include "mdb_dev.h"

MODULE_AUTHOR("Paul Taysom <Paul.Taysom@novell.com>");
MODULE_DESCRIPTION("mdb character device");
MODULE_LICENSE("GPL");

#define WARNING	"<1>"

#if 0
#define FN	printk("<1>" "%s\n", __func__)
#else
#define FN	((void)0)
#endif

static ssize_t mdb_file_read (
	struct file	*file,
	char __user	*buf,
	size_t		count,
	loff_t		*ppos)
{
	Mdb_s	mdb;
FN;
	if (copy_from_user( &mdb, buf, count)) {
		return -EFAULT;
	}
	switch (mdb.mdb_cmd) {
	case MDB_READ:
		if (copy_to_user((void*)mdb.mdb_buf, (void*)mdb.mdb_addr,
				mdb.mdb_size))
		{
			return -EFAULT;
		}
		return mdb.mdb_size;

	case MDB_WRITE:
		if (copy_from_user((void*)mdb.mdb_addr, (void*)mdb.mdb_buf,
				mdb.mdb_size))
		{
			return -EFAULT;
		}
		return mdb.mdb_size;

	case MDB_PID2TASK:
		rcu_read_lock();
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,17))
		mdb.pid_task = (unsigned long)find_task_by_pid(mdb.pid_pid);
#else
		//mdb.pid_task = (unsigned long)find_task_by_vpid(mdb.pid_pid);
		mdb.pid_task = (unsigned long)pid_task(find_get_pid(mdb.pid_pid),
							PIDTYPE_PID);
#endif
		rcu_read_unlock();
		if (copy_to_user( buf, &mdb, count)) {
			return -EFAULT;
		}
		return 0;

	default:
		return -EINVAL;
	}

	return 0;
}

static int mdb_open (struct inode *inode, struct file *file)
{
FN;
	try_module_get(THIS_MODULE);	// increments use count on module

	inode->i_size = -1;

	file->private_data = NULL;

	return 0;
}

static int mdb_release (struct inode * inode, struct file * file)
{
FN;
	module_put(THIS_MODULE);
	return 0;
}

static struct file_operations mdb_file_operations = {
	.llseek		= generic_file_llseek,
	.read		= mdb_file_read,
	.open		= mdb_open,
	.release	= mdb_release,
};

static struct miscdevice kmdb_device = {
	MISC_DYNAMIC_MINOR, DEV_NAME, &mdb_file_operations
};

static int mdb_init (void)
{
	int	rc;
FN;
	rc = misc_register( &kmdb_device);
	if (rc) {
		printk(KERN_INFO "%s load failed\n", DEV_NAME);
	} else {
		printk(KERN_INFO "%s loaded\n", DEV_NAME);
	}
	return 0;
}

static void mdb_exit (void)
{
	int	rc;

	rc = misc_deregister( &kmdb_device);
	if (rc) {
		printk(KERN_INFO "%s un-load failed\n", DEV_NAME);
	} else {
		printk(KERN_INFO "%s un-loaded\n", DEV_NAME);
	}
	return;
}


module_init(mdb_init)
module_exit(mdb_exit)

