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

#include <stdio.h>

#include <tau/tag.h>
#include <tau/switchboard.h>
#include <tau/msg_util.h>

#include <debug.h>
#include <eprintf.h>

#include <dirfs.h>

#include <sage_hdr.h>

typedef struct fs_type_s {
	type_s		tf_tag;
	method_f	tf_supplied;
} fs_type_s;

typedef struct fs_s {
	type_s		*fs_type;
} fs_s;

static void supplied_fs (void *m);

fs_type_s	Fs_type = { {SW_REPLY_MAX, 0 },
				supplied_fs };

fs_s		Fs = { &Fs_type.tf_tag };

enum { MAX_FS = 10 };

unsigned	Fs_keys[MAX_FS];
unsigned	*Fs_next = Fs_keys;

static void supplied_fs (void *m)
{
	msg_s		*msg = m;
	unsigned	fs_key;
FN;
	fs_key = msg->q.q_passed_key;
// Need to really get a resource key but this is fine for now.
	if (Fs_next == &Fs_keys[MAX_FS]) {
		eprintf("Fs_keys is full\n");
		destroy_key_tau(fs_key);
		return;
	}
	*Fs_next++ = fs_key;
}

/*static*/ int multi_cast (void *m)
{
	unsigned	*fs_key;
	int		rc;

	for (fs_key = Fs_keys; fs_key != Fs_next; fs_key++) {
		rc = send_tau(*fs_key, m);
		if (rc) {
			eprintf("multi_cast send_tau failed %d", rc);
			return rc;
		}
	}
	return 0;
}

static u64 share_mask (int num_nodes, int node_instance)
{
	u64	mask = 0;
	int	i;

	for (i = node_instance; i < MAX_SHARES; i += num_nodes) {
		mask |= (1 << i);
	}
	return mask;
}

int make_fs (void)
{
	fs_msg_s	m;
	int		i;
	int		n;
	int		rc;
FN;
	m.m_method        = FS_MAKE;
	m.mk_total_shares = MAX_SHARES;
	n = Fs_next - Fs_keys;
	for (i = 0; i < n; i++) {
		m.mk_my_shares = share_mask(n, i);
		m.mk_my_backup = share_mask(n, (i + 1) % n);
		rc = send_tau(Fs_keys[i], &m);
		if (rc) {
			eprintf("make_fs send_tau failed %d", rc);
			return rc;
		}
	}
	return 0;
}

void init_fs (void)
{
	ki_t	key;
	int	rc;
FN;
	key = make_gate( &Fs, REQUEST | PASS_OTHER);

	rc = sw_request("dir_sage", key);
	if (rc) failure("sw_request", rc);
}
