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
#include <asm/uaccess.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/netdevice.h>

#include <tau/msg.h>
#include <tau/msg_sys.h>
#include <tau/net.h>
#include "msg_debug.h"
#include "msg_internal.h"

void msg_put_user_pages (
	struct page	**pages,
	int		nr_pages)
{
	int	i;

	for (i = 0; i < nr_pages; i ++) {
		page_cache_release(pages[i]);
	}
}

int map_data (
	struct task_struct	*process,
	datagate_s	*gate,
	void		*start,
	unsigned	nr_pages,
	unint		type)
{
	int	rc;
FN;
	down_read( &process->mm->mmap_sem);
	rc = get_user_pages(process, process->mm, (addr)start,
				nr_pages, type & WRITE_DATA, 0,
				gate->dg_pages, NULL);
	up_read( &process->mm->mmap_sem);

	if (rc < 0) {
		return rc;
	}
	if (rc != nr_pages) {
		msg_put_user_pages(gate->dg_pages, rc);
		return EDATA;
	}
	gate->dg_nr_pages = nr_pages;
	gate->dg_page_offset = (addr)start & OFFSET_MASK;
	return 0;
}

int read_from_kernel (void *dest, void *gate, u64 offset, unint length)
{
	datagate_s	*g = gate;
	u8		*start;
FN;
	start = (u8 *)g->dg_start + offset;
	memmove(dest, start, length);
	return 0;
}

int read_from_user (void *dest, void *gate, u64 offset, unint length)
{
	datagate_s	*g = gate;
	addr		base;
	addr		start;
	struct page	*page;
	u8		*kaddr;
	unint		page_index;
	unint		start_in_page;
	unint		length_in_page;
FN;
	base  = g->dg_start & PAGE_CACHE_MASK;
	start = g->dg_start + offset;
	page_index    = (start - base) >> PAGE_CACHE_SHIFT;
	start_in_page = start & ~PAGE_CACHE_MASK;

	while (length) {
		if (page_index >= g->dg_nr_pages) {
			return ETOOBIG;
		}
		page = g->dg_pages[page_index];

		length_in_page = PAGE_CACHE_SIZE - start_in_page;
		if (length_in_page > length) {
			length_in_page = length;
		}
		kaddr = kmap_atomic(page, KM_USER0);
		memmove(dest, kaddr + start_in_page, length_in_page);
		kunmap_atomic(page, KM_USER0);

		dest   += length_in_page;
		length -= length_in_page;
		start_in_page  = 0;
		++page_index;
	}
	return 0;
}

int write_to_kernel (void *gate, void *src, u64 offset, unint length)
{
	datagate_s	*g = gate;
	u8		*start;
FN;
	start = (u8 *)g->dg_start + offset;
	memmove(start, src, length);
	return 0;
}

int write_to_user (void *gate, void *src, u64 offset, unint length)
{
	datagate_s	*g = gate;
	addr		base;
	addr		start;
	struct page	*page;
	u8		*kaddr;
	unint		page_index;
	unint		start_in_page;
	unint		length_in_page;
FN;
	base  = g->dg_start & PAGE_CACHE_MASK;
	start = g->dg_start + offset;
	page_index    = (start - base) >> PAGE_CACHE_SHIFT;
	start_in_page = start & ~PAGE_CACHE_MASK;

	while (length) {
		if (page_index >= g->dg_nr_pages) {
			return ETOOBIG;
		}
		page = g->dg_pages[page_index];

		length_in_page = PAGE_CACHE_SIZE - start_in_page;
		if (length_in_page > length) {
			length_in_page = length;
		}
		kaddr = kmap_atomic(page, KM_USER0);
		memmove(kaddr + start_in_page, src, length_in_page);
		kunmap_atomic(page, KM_USER0);

		src    += length_in_page;
		length -= length_in_page;
		start_in_page  = 0;
		++page_index;
	}
	return 0;
}

static int remote_read (unint flags, avatar_s *avatar, key_s *key, data_s *data)
{
	reply_s		reply;
	packet_s	p;
	void		*start = data->q_start;
	unsigned	length = data->q_length;
	datagate_s	*gate;
	int		rc;
FN;
	rc = new_gate(avatar,
			PERM|WRITE_DATA | ((flags & KERNEL_RW) ? KERNEL_GATE : 0),
			&reply, start, length, &gate);
	if (rc) {
		return EDATA;
	}
	MAKE_KEY(p.pk_passed_key, gate);
	p.pk_sys.q_length = length;
	p.pk_sys.q_offset = data->q_offset;
	p.pk_key = *key;
	UNLOCK_MSG;
	Net_start_read( &p);
	LOCK_MSG;
	wait_reply( &reply);
//XXX: need to check error code in here
	free_gate(gate);
	return 0;
}

int kernel_write_to_net (void *dest, void *src, u64 offset, unint length)
{
	memmove(dest, src, length);
	return 0;
}

int user_write_to_net (void *dest, void *src, u64 offset, unint length)
{
	return copy_from_user(dest, src, length);
}

int remote_rw_data (unint flags, avatar_s *avatar, key_s *key, data_s *data)
{
	int	rc;
FN;
	if (flags & WRITE_RW) {
		UNLOCK_MSG;
		rc = Net_write(key, data,
				(flags & KERNEL_RW) ? kernel_write_to_net
						    : user_write_to_net);
		LOCK_MSG;
	} else {
		rc = remote_read(flags, avatar, key, data);
	}
	return 0;
}

int local_rw_data (unint flags, datagate_s *gate, data_s *data)
{
	struct page	**pagep;
	char		*owner_start   = data->q_start;
	u64		owner_length   = data->q_length;
	u64		avatar_offset = data->q_offset;
	addr		avatar_start  = (addr)gate->dg_start;
	u64		avatar_length = gate->dg_length;
	u64		page_start;
	unint		page_index;
	unint		page_offset;
	unint		page_length;
	char		*kaddr;
	int		rc = 0;
FN;
	if (avatar_offset >= avatar_length) return ETOOBIG;

	avatar_length -= avatar_offset;
	if (owner_length > avatar_length) return ETOOBIG;

	if (gate->gt_type & KERNEL_GATE) {
		kaddr = (char *)avatar_start + avatar_offset;
		if (flags & KERNEL_RW) {
			if (flags & WRITE_RW) {
				memmove(kaddr, owner_start,
					owner_length);
			} else {
				memmove(owner_start, kaddr,
					owner_length);
			}
		} else {
			if (flags & WRITE_RW) {
				rc = copy_from_user(kaddr, owner_start,
							owner_length);
			} else {
				rc = copy_to_user(owner_start, kaddr,
							owner_length);
			}
		}
		return rc;
	}

	page_start = avatar_start >> PAGE_CACHE_SHIFT;
	avatar_start += avatar_offset;
	page_index = (avatar_start >> PAGE_CACHE_SHIFT) - page_start;

	if (owner_length > avatar_length) return -EDATA;

	page_offset = avatar_start & OFFSET_MASK;
	page_length = PAGE_CACHE_SIZE - page_offset;

	for (pagep = &gate->dg_pages[page_index]; owner_length; pagep++) {
		if (page_length > owner_length) {
			page_length = owner_length;
		}
		kaddr = kmap(*pagep);
		kaddr += page_offset;
		if (flags & KERNEL_RW) {
			if (flags & WRITE_RW) {
				memmove(kaddr, owner_start,
					page_length);
			} else {
				memmove(owner_start, kaddr,
					page_length);
			}
		} else {
			if (flags & WRITE_RW) {
				rc = copy_from_user(kaddr, owner_start,
							page_length);
			} else {
				rc = copy_to_user(owner_start, kaddr,
							page_length);
			}
		}
		kunmap(*pagep);
		if (rc) return -EDATA;

		owner_start  += page_length;
		owner_length -= page_length;
		page_offset = 0;
		page_length = PAGE_CACHE_SIZE;
	}
	return 0;
}

int rw_data (
	unint		flags,
	avatar_s	*avatar,
	unint		key_index,
	data_s		*data)
{
	key_s		*key;
	datagate_s	*gate;
	int		rc = 0;

	key = get_key(avatar, key_index);
	if (!key) {
		return EBADKEY;
	}
	if (key->k_node) {
		return remote_rw_data(flags, avatar, key, data);
	}
	gate = find_gate(key->k_id);
	if (!gate) {
		return EBROKEN;
	}
	rc = local_rw_data(flags, gate, data);
	release_gate(gate);
	return rc;
}

int copy_to_avatar_tau (packet_s *p)
{
	key_s		*key = &p->pk_key;
	reply_s		*reply = NULL;
	datagate_s	*gate;
	data_s		data;
	int		rc;
FN;
	LOCK_MSG;
	gate = find_gate(key->k_id);
	if (!gate) {
		UNLOCK_MSG;
		return EBROKEN;
	}
	data.q_start  = p->pk_body;
	data.q_length = p->pk_sys.q_length;
	data.q_offset = p->pk_sys.q_offset;

	rc = local_rw_data(KERNEL_RW | WRITE_RW, gate, &data);

	if (p->pk_flags & LAST) {
		reply = gate->gt_tag;
	}

	release_gate(gate);
	if (reply) {
		wake_reply(reply);
	}
	UNLOCK_MSG;

	return rc;
}
EXPORT_SYMBOL(copy_to_avatar_tau);

int read_from_avatar_tau (packet_s *p)
{	
	key_s		*key = &p->pk_key;
	datagate_s	*gate;
	int		rc;
FN;
	LOCK_MSG;
	gate = find_gate(key->k_id);
	UNLOCK_MSG;
	if (!gate) {
		return EBROKEN;
	}
//XXX:offset not used
	rc = Net_read( &p->pk_passed_key, (data_s *)&p->pk_sys.q_start,
			(gate->gt_type & KERNEL_GATE) ? read_from_kernel
						      : read_from_user, gate);
	LOCK_MSG;
	release_gate(gate);
	UNLOCK_MSG;

	return rc;
}
EXPORT_SYMBOL(read_from_avatar_tau);
