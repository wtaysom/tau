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

#include <linux/kernel.h>
#include <linux/module.h>

int	A_symbol;
EXPORT_SYMBOL(A_symbol);

static int a_init (void)
{
	printk(KERN_INFO "a loaded\n");

	A_symbol = 37;
	printk(KERN_INFO "A_symbol=%d\n", A_symbol);

	return 0;
}

static void a_exit (void)
{
	printk(KERN_INFO "A_symbol=%d\n", A_symbol);
	printk(KERN_INFO "a unloaded\n");
}

MODULE_AUTHOR("Paul Taysom");
MODULE_DESCRIPTION("a test module");
MODULE_LICENSE("GPL v2");

module_init(a_init)
module_exit(a_exit)

