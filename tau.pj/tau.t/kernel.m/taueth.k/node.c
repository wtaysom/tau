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

/*
 * slot 0 is reserved for the local node but is not filled in because
 * it may represent multiple networks.
 */

#include <linux/module.h>

#define tMASK	tNET
#include <tau/debug.h>
#include <tau/net.h>

enum {	NODE_HASH_SHIFT	= 3,
	NODE_HASH_SIZE	= (1 << NODE_HASH_SHIFT),
	NODE_HASH_MASK	= (NODE_HASH_SIZE - 1),
	NO_SLOT         = -1 };

node_s	*Tau_node_slot[MAX_NODES];
EXPORT_SYMBOL(Tau_node_slot);

static node_s	*Node_bucket[NODE_HASH_SIZE];

static snint alloc_slot (node_s *node)
{
	unint	slot;
FN;
	for (slot = 0; slot < MAX_NODES; slot++) {
		if (!Tau_node_slot[slot]) {
			Tau_node_slot[slot] = node;
			node->n_slot = slot;
			return slot;
		}
	}
	printk("<2>" "Out of node slots\n");
	return NO_SLOT;
}

static inline node_s **hash (u8 *netaddr, unint size)
{
	u8	*m;
	int	sum = 0;

	for (m = netaddr; m < &netaddr[size]; m++) {
		sum += *m;
	}
	return &Node_bucket[sum & NODE_HASH_MASK];
}

static void add_node (node_s *node, unint size)
{
	node_s	**bucket;

	bucket = hash(node->n_netaddr, size);
	node->n_next = *bucket;
	*bucket = node;
}

node_s *find_node (u8 *netaddr, unint size)
{
	node_s	**bucket = hash(netaddr, size);
	node_s	*next;
	node_s	*prev;

	if (size > NET_ADDR_SIZE) return NULL;

	next = *bucket;
	if (!next) {
		return NULL;
	}
	if (memcmp(next->n_netaddr, netaddr, size) == 0) {
		return next;
	}
	for (;;) {
		prev = next;
		next = prev->n_next;
		if (!next) {
			return NULL;
		}
		if (memcmp(next->n_netaddr, netaddr, size) == 0) {
			prev->n_next = next->n_next;
			next->n_next = *bucket;
			*bucket = next;
			return next;
		}
	}
}

node_s *new_node (u8 *netaddr, unint size)
{
	node_s	*node;
	unint	slot = 0;
FN;
	if (size > NET_ADDR_SIZE) return NULL;
	node = kmalloc(sizeof(*node), GFP_ATOMIC);
	if (!node) goto exit;
	zero(*node);

	slot = alloc_slot(node);
	if (slot == NO_SLOT) {
		kfree(node);
		return NULL;
	}

	memcpy(node->n_netaddr, netaddr, size);
	add_node(node, size);
exit:
	return node;
}

void delete_node (node_s *node, unint size)
{
	node_s	**bucket;
	node_s	*next;
	node_s	*prev;

	if (!node) {
		return;
	}
	bucket = hash(node->n_netaddr, size);
	next = *bucket;
	if (!next) {
		return;
	}
	if (next == node) {
		*bucket = next->n_next;
		return;
	}
	for (;;) {
		prev = next;
		next = prev->n_next;
		if (!next) {
			return;
		}
		if (next == node) {
			prev->n_next = next->n_next;
			return;
		}
	}
}
