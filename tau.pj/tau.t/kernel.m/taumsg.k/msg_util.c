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
#include <linux/random.h>

#include <tau/debug.h>
#include "msg_internal.h"

void *zalloc_tau (unint size)
{
	void	*x;
FN;
	x = kmalloc(size, GFP_KERNEL);
	assert(x!=NULL);

	memset(x, 0, size);
	return x;
}
EXPORT_SYMBOL(zalloc_tau);

void free_tau (void *data)
{
FN;
	if (data) kfree(data);
}
EXPORT_SYMBOL(free_tau);


void guid_generate_tau (guid_t guid)
{
	u8	*g;

	get_random_bytes(guid, sizeof(guid));
	g = (u8 *)guid;
	g[7] = (g[7] & 0x0f) | 0x40;	/* Set guid version */
	g[8] = (g[8] & 0x3f) | 0x80;	/* Set guid variant */
}
EXPORT_SYMBOL(guid_generate_tau);

ki_t make_gate (
	void		*tag,
	unsigned	type)
{
	msg_s	msg;
	int	rc;

	msg.q.q_tag  = tag;
	msg.q.q_type = type;
	rc = create_gate_tau( &msg);
	if (rc) {
		printk("make_gate: create_gate_tau failed %d\n", rc);
		return 0;
	}
	return msg.q.q_passed_key;
}
EXPORT_SYMBOL(make_gate);
