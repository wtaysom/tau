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
#include <linux/pagemap.h>
#include <linux/random.h>

#include "msg_debug.h"
#include "msg_internal.h"

enum {	KEY_HASH_SHIFT = 1,	// XXX: need to be bigger (>7)
	KEY_HASH_SIZE  = (1 << KEY_HASH_SHIFT),
	KEY_HASH_MASK  = (KEY_HASH_SIZE - 1),
	KEY_INC = 1 /*614889782588491411ULL*/};

static msg_spinlock_s	Gate_spinlock;
static datagate_s	*Gate_bucket[KEY_HASH_SIZE];	
static u64		Gate_id;
static kmem_cache_t	*Gate_pool;

#if 0
	#define LOCK_GATE()	\
		(msg_spinlock( &Gate_spinlock, WHERE))

	#define UNLOCK_GATE()	\
		(msg_spinunlock( &Gate_spinlock, WHERE))
#else
	#define LOCK_GATE()	((void) 0)
	#define UNLOCK_GATE()	((void) 0)
#endif



static inline datagate_s **hash (u64 key_id)
{
	return &Gate_bucket[key_id & KEY_HASH_MASK];
}

datagate_s *lookup_gate (u64 key_id)
{
	datagate_s	**bucket = hash(key_id);
	datagate_s	*next;
	datagate_s	*prev;
FN;
	next = *bucket;
	if (!next) {
		return NULL;
	}
	if (next->gt_id == key_id) {
		return next;
	}
	for (;;) {
		prev = next;
		next = prev->gt_hash;
		if (!next) {
			return NULL;
		}
		if (next->gt_id == key_id) {
			prev->gt_hash = next->gt_hash;
			next->gt_hash = *bucket;
			*bucket = next;
			return next;
		}
	}
}

datagate_s *find_gate (u64 key_id)
{
	datagate_s	*gate;
FN;
	gate = lookup_gate(key_id);
	if (gate) ++gate->gt_usecnt;
	return gate;
}

void add_gate (datagate_s *gate)
{
	datagate_s	**bucket;
FN;
	assert(!gate->gt_hash);
	do {
		Gate_id += KEY_INC;
	} while (!Gate_id);
	gate->gt_id = Gate_id;
	bucket = hash(Gate_id);
	gate->gt_hash = *bucket;
	*bucket = gate;
}

static void remove_gate (datagate_s *gate)
{
	datagate_s	**bucket;
	datagate_s	*next;
	datagate_s	*prev;
FN;
LOCKED_MSG;
	if (!gate) {
		goto exit;
	}
	bucket = hash(gate->gt_id);
	next = *bucket;
	if (!next) {
		goto exit;
	}
	if (next == gate) {
		*bucket = next->gt_hash;
		next->gt_hash = 0;
		goto exit;
	}
	for (;;) {
		prev = next;
		next = prev->gt_hash;
		if (!next) {
			break;
		}
		if (next == gate) {
			prev->gt_hash = next->gt_hash;
			next->gt_hash = 0;
			break;
		}
	}
exit:;
}

void release_gate (datagate_s *gate)
{
FN;
	assert(gate->gt_usecnt);
	--gate->gt_usecnt;
	if (gate->gt_usecnt || is_qmember(gate->gt_siblings)) {
		return;
	}
	remove_gate(gate);
	if ((gate->gt_type & ONCE) && (gate->gt_type & PERM)) {
		return;
	}
	if (gate->gt_type & (READ_DATA | WRITE_DATA)) {
		if (gate->dg_nr_pages > 0) {
			msg_put_user_pages(gate->dg_pages, gate->dg_nr_pages);
		}
		kfree(gate);
	} else {
		kmem_cache_free(Gate_pool, gate);
	}
	++Inst.gates_destroyed;
}

void free_gate (datagate_s *gate)
{
FN;
	rmv_dq( &gate->gt_siblings);
	release_gate(gate);
}

datagate_s *get_gate (key_s *key)
{
	datagate_s	**bucket = hash(key->k_id);
	datagate_s	*gate;
	datagate_s	*prev;
FN;
	gate = *bucket;
	if (!gate) return NULL;

	if (gate->gt_id == key->k_id) {
		if (gate->gt_type & ONCE) {
			*bucket = gate->gt_hash;
			rmv_dq( &gate->gt_siblings);
		}
		goto exit;
	}
	for (;;) {
		prev = gate;
		gate = prev->gt_hash;
		if (!gate) {
			return NULL;
		}
		if (gate->gt_id == key->k_id) {
			prev->gt_hash = gate->gt_hash;
			if (gate->gt_type & ONCE) {
				rmv_dq( &gate->gt_siblings);
			} else {
				gate->gt_hash = *bucket;
				*bucket = gate;
			}
			goto exit;
		}
	}
exit:
	++gate->gt_usecnt;
	return gate;
}


static unint num_pages (void *start, unint length)
{
	unint	end;

	if (!length) return 0;

	length += (addr)start & OFFSET_MASK;
	end = length & OFFSET_MASK;
	if (end) {
		length += PAGE_CACHE_SIZE - end;
	}
	return length >> PAGE_CACHE_SHIFT;
}

int init_reply_gate (sys_s *q, avatar_s *avatar, datagate_s *gate, void *tag)
{
	unint		nr_pages;
	unint		type;
	int		rc;
FN;
	type = ((PERM | ONCE) | PASS_ANY)
		| (q->q_type & (READ_DATA | WRITE_DATA | KERNEL_GATE));
	if (type & (READ_DATA | WRITE_DATA)) {
		if (!(type & KERNEL_GATE)) {
			nr_pages = num_pages(q->q_start, q->q_length);
			if (nr_pages > MAX_DATA_PAGES) {
PRd(nr_pages);
PRp(q->q_start);
PRx(q->q_length);
				return -ENOMEM;
			}
			rc = map_data(current, gate,
					q->q_start, nr_pages, type);
			if (rc) return rc;
		}
		gate->dg_start  = (addr)q->q_start;
		gate->dg_length = q->q_length;
	}
	gate->gt_hash    = NULL;
	gate->gt_avatar = avatar;
	gate->gt_type    = type;
	gate->gt_tag     = tag;
	gate->gt_usecnt  = 1;
	init_qlink(gate->gt_siblings);
	enq_dq( &avatar->av_gates, gate, gt_siblings);	//XXX: do we need to do this?
	add_gate(gate);
	return 0;
}

int new_gate (
	avatar_s	*avatar,
	unint		type,
	void		*tag,
	void		*start,
	u64		length,
	datagate_s	**pgate)
{
	datagate_s	*gate;
	unint		nr_pages;
	int		rc;
FN;
	if (type & (READ_DATA | WRITE_DATA)) {
		if (type & KERNEL_GATE) {
			UNLOCK_MSG;
			gate = kmalloc(sizeof(datagate_s), GFP_KERNEL);
			LOCK_MSG;
			gate->dg_nr_pages = 0;
		} else {
			nr_pages = num_pages(start, length);
			UNLOCK_MSG;
			gate = kmalloc(sizeof(datagate_s)
					+ nr_pages * sizeof(struct page *),
					GFP_KERNEL);
			LOCK_MSG;
			if (!gate) {
				return -ENOMEM;
			}
			memset(gate, 0, sizeof(datagate_s)
					+ nr_pages * sizeof(struct page *));
			rc = map_data(current, gate, start, nr_pages, type);
			if (rc) {
				kfree(gate);
				return rc;
			}
		}
		gate->dg_start  = (addr)start;
		gate->dg_length = length;
		++Inst.gates_created;
	} else if ((type & (PERM | ONCE)) == (PERM | ONCE)) {
		eprintk("bad gate type %lx", type);
		return -EBADGATE;
	} else {
		UNLOCK_MSG;
		gate = kmem_cache_alloc(Gate_pool, SLAB_KERNEL);
		LOCK_MSG;
		if (!gate) return -ENOMEM;
		memset(gate, 0, sizeof(gate_s));
		++Inst.gates_created;
	}
	gate->gt_hash    = NULL;
	gate->gt_avatar  = avatar;
	gate->gt_type    = type;
	gate->gt_tag     = tag;
	gate->gt_usecnt  = 1;
	gate->gt_node    = 0;
	init_qlink(gate->gt_siblings);
	enq_dq( &avatar->av_gates, gate, gt_siblings);
	add_gate(gate);
	*pgate = gate;
	return 0;
}

int read_gate (u64 id, datagate_s *buf)
{
	datagate_s	*gate;

	gate = lookup_gate(id);
	if (!gate) return EBADID;

	if (gate->gt_type & (READ_DATA | WRITE_DATA)) {
		memcpy(buf, gate, sizeof(datagate_s));
	} else {
		memcpy(buf, gate, sizeof(gate_s));
	}
	return 0;
}

int init_gate_pool (void)
{
FN;
	msg_init_spinlock( &Gate_spinlock, "Gate");
	get_random_bytes( &Gate_id, sizeof(Gate_id));
	Gate_id &= 0xff000000;

	Gate_pool = kmem_cache_create("gate_pool", sizeof(gate_s), 0,
				SLAB_HWCACHE_ALIGN|SLAB_RECLAIM_ACCOUNT,
				NULL, NULL);
	if (!Gate_pool) return -ENOMEM;
	return 0;
}

void destroy_gate_pool (void)
{
FN;
	if (Gate_pool && kmem_cache_destroy(Gate_pool)) {
		printk(KERN_INFO "gate_pool: not all gates were freed"
			" created=%llu destroyed=%llu\n",
			Inst.gates_created, Inst.gates_destroyed);
	}
}
