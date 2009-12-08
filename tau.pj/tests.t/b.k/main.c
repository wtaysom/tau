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

extern int A_symbol;

int B_symbol;
EXPORT_SYMBOL(B_symbol);

static int b_init (void)
{
	printk(KERN_INFO "b loaded\n");
	printk(KERN_INFO "A_symbol=%d\n", A_symbol);
	A_symbol = 107;
	B_symbol = 1003;
	return 0;
}

static void b_exit (void)
{
	printk(KERN_INFO "b unloaded\n");
}

MODULE_AUTHOR("Paul Taysom");
MODULE_DESCRIPTION("b test module");
MODULE_LICENSE("GPL v2");

module_init(b_init)
module_exit(b_exit)

