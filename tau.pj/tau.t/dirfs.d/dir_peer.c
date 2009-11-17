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
#include <stdlib.h>
#include <string.h>

#include <tau/tag.h>
#include <tau/switchboard.h>
#include <tau/msg_util.h>

#include <debug.h>
#include <eprintf.h>

#include <dir_hdr.h>

enum { MAX_PEERS = 10 };

/* Hello */
enum { HELLO, HELLO_OPS };

typedef struct hello_type_s {
	type_s		th_tag;
	method_f	th_hello;
} hello_type_s;

typedef struct hello_s {
	type_s		*hi_type;
} hello_s;

static void hello (void *m);

hello_type_s	Hello_type = { {HELLO_OPS, 0 },
				hello };
hello_s		Hello = { &Hello_type.th_tag };

/* Welcome */
typedef struct welcome_type_s {
	type_s		tw_tag;
	method_f	tw_welcome;
} welcome_type_s;

typedef struct welcome_s {
	type_s		*wl_type;
} welcome_s;

static void welcome (void *m);

welcome_type_s	Welcome_type = { {1, 0 },
				welcome };
welcome_s	Welcome = { &Welcome_type.tw_tag };

/* Share */
typedef struct share_type_s {
	type_s		ts_tag;
	method_f	ts_peer_key;
	method_f	ts_add_shares;
	method_f	ts_del_shares;
} share_type_s;

typedef struct share_s {
	type_s		*sh_type;
	unsigned	sh_peer_key;
	u64		sh_share_mask;
	u64		sh_backup_mask;
} share_s;

static void share_destroyed (void *m);
static void peer_key   (void *m);
static void add_shares (void *m);
static void del_shares (void *m);

share_type_s	Share_type = { {SHARE_OPS, share_destroyed },
				peer_key,
				add_shares,
				del_shares };

/* Peer */
typedef struct peer_type_s {
	type_s		tp_tag;
	method_f	tp_lookup;
	method_f	tp_add_dir;
	method_f	tp_new_dir;
	method_f	tp_print_dir;
	method_f	tp_readdir;
	method_f	tp_backup_add;
	method_f	tp_backup_new;
	method_f	tp_backup_print;
} peer_type_s;

typedef struct peer_s {
	type_s		*pr_type;
	unsigned	pr_share_key;
} peer_s;

static peer_s *peer_new (void);

static void peer_destroyed   (void *m);
static void peer_lookup      (void *msg);
static void peer_add_dir     (void *msg);
static void peer_new_dir     (void *msg);
static void peer_print_dir   (void *msg);
static void readdir_peer     (void *m);
static void backup_add_dir   (void *msg);
static void backup_new_dir   (void *msg);
static void backup_print_dir (void *msg);

peer_type_s	Peer_type = { {PEER_OPS, peer_destroyed },
				peer_lookup,
				peer_add_dir,
				peer_new_dir,
				peer_print_dir,
				readdir_peer,
				backup_add_dir,
				backup_new_dir,
				backup_print_dir};

/****************************************************************/

share_s	*Share[MAX_SHARES];
share_s	*Backup[MAX_SHARES];

unsigned remote (u64 id)
{
	unsigned	share;

	share = id % MAX_SHARES; // Need to resolve total_shares and MAX_SHARES
	return Share[share]->sh_peer_key;
}

unsigned backup (u64 id)
{
	unsigned	share;

	share = id % MAX_SHARES; // Need to resolve total_shares and MAX_SHARES
	return Backup[share]->sh_peer_key;
}	

/* Hello */
static void hello (void *m)
{
	msg_s	*msg = m;
	ki_t	share_key = msg->q.q_passed_key;
	ki_t	peer_key;
	peer_s	*peer;
	int	rc;
FN;
	peer = peer_new();
	peer->pr_type = &Peer_type.tp_tag;
	peer->pr_share_key = share_key;

	peer_key = make_gate(peer, RESOURCE | PASS_ANY);
	if (!peer_key) {
		free(peer);
		return;
	}
	msg->q.q_passed_key = peer_key;
	msg->m_method = SHARE_PEER;
	rc = send_key_tau(share_key, msg);
	if (rc) {
		destroy_key_tau(peer_key);
	}
}

/* Welcome */
static void welcome (void *m)
{
	msg_s	*msg = m;
	ki_t	hello_key = msg->q.q_passed_key;
	ki_t	share_key;
	share_s	*share;
	int	rc;
FN;
	share = ezalloc(sizeof(*share));
	share->sh_type = &Share_type.ts_tag;

	share_key = make_gate(share, RESOURCE | PASS_ANY);
	if (!share_key) {
		free(share);
		return;
	}
	msg->q.q_passed_key = share_key;
	msg->m_method = HELLO;
	rc = send_key_tau(hello_key, msg);
	if (rc) {
		destroy_key_tau(share_key);
	}
	destroy_key_tau(hello_key);
}

/* Share */
static void assign_share_owner (share_s **shares, share_s *owner, u64 mask)
{
	unsigned	i;
FN;
	for (i = 0; i < MAX_SHARES; i++, shares++) {
		if (mask & (1 << i)) {
			if (*shares) {
				printf("Share %d already claimed by %p\n",
					i, *shares);
			} else {
				*shares = owner;
			}
		}
	}
}

static void clear_share_owner (share_s **shares, share_s *owner, u64 mask)
{
	unsigned	i;
FN;
	for (i = 0; i < MAX_SHARES; i++, shares++) {
		if (mask & (1 << i)) {
			if (*shares == owner) {
				*shares = NULL;
			} else {
				printf("Share %d claimed by %p not %p\n",
					i, *shares, owner);
			}
		}
	}
}

static void move_shares (share_s **to, share_s **from, u64 mask)
{
	unsigned	i;
FN;
	for (i = 0; i < MAX_SHARES; i++, to++, from++) {
		if (mask & (1 << i)) {
			*to = *from;
			*from = NULL;	// This is a move, not a copy!
		}
	}
}

static void share_destroyed (void *m)
{
	msg_s		*msg = m;
	share_s		*owner = msg->q.q_tag;
FN;
	clear_share_owner(Share,  owner, owner->sh_share_mask);
	clear_share_owner(Backup, owner, owner->sh_backup_mask);

	move_shares(Share, Backup, owner->sh_share_mask);

	upshares(owner->sh_share_mask);

	destroy_key_tau(owner->sh_peer_key);
	free(owner);
}	

static void peer_key (void *m)
{
	msg_s		*msg = m;
	share_s		*share = msg->q.q_tag;
FN;
	share->sh_peer_key = msg->q.q_passed_key;
}

static void add_shares (void *m)
{
	fs_msg_s	*msg = m;
	share_s		*owner = msg->q.q_tag;
FN;
	owner->sh_share_mask  = msg->mk_my_shares;
	owner->sh_backup_mask = msg->mk_my_backup;

	assign_share_owner(Share,  owner, msg->mk_my_shares);
	assign_share_owner(Backup, owner, msg->mk_my_backup);
}

static void del_shares (void *m)
{
	fs_msg_s	*msg = m;
	share_s		*owner = msg->q.q_tag;
	u64		share_mask  = msg->mk_my_shares;
	u64		backup_mask = msg->mk_my_backup;
FN;
	owner->sh_share_mask  = share_mask;
	owner->sh_backup_mask = backup_mask;

	clear_share_owner(Share,  owner, share_mask);
	clear_share_owner(Backup, owner, backup_mask);
}

/* PEER */
peer_s	Peer[MAX_PEERS];
peer_s	*NextPeer = Peer;

static void peer_destroyed (void *m)
{
	msg_s	*msg = m;
	peer_s	*peer = msg->q.q_tag;
FN;
	destroy_key_tau(peer->pr_share_key);
	zero(*peer);
}	

static peer_s *peer_new (void)
{
	if (NextPeer == &Peer[MAX_PEERS]) {
		eprintf("Out of Peers %d", MAX_PEERS);
		return NULL;
	}
	return NextPeer++;
}

void peer_broadcast (void *m)
{
	msg_s	*msg = m;
	peer_s	*peer;
	int	rc;

	for (peer = Peer; peer != NextPeer; peer++) {
		if (peer->pr_share_key) {
			rc = send_tau(peer->pr_share_key, msg);
			if (rc) {
				eprintf("peer_broadcast send_tau %d", rc);
			}
		}
	}
}

static void peer_lookup (void *msg)
{
	fs_msg_s	*m = msg;
	int		rc;
FN;
	m->dir_id = dir_lookup(m->dir_parent_id, m->dir_name);
	rc = send_tau(m->q.q_passed_key, m);//should use share key
	if (rc) {
		eprintf("peer_lookup send_tau failed %d", rc);
	}
}

static void peer_add_dir (void *msg)
{
	fs_msg_s	*m = msg;
FN;
	dir_add(m->dir_parent_id, m->dir_id, m->dir_name);
}

static void peer_new_dir (void *msg)
{
	fs_msg_s	*m = msg;
	int		rc;
FN;
	m->dir_id = dir_new();
	rc = send_tau(m->q.q_passed_key, m);//should use share key
	if (rc) {
		eprintf("peer_enw send_tau failed %d", rc);
	}
}

static void peer_print_dir (void *msg)
{
	fs_msg_s	*m = msg;
FN;
	dir_print(m->dir_id, m->dir_indent);
}

static void backup_print_dir (void *msg)
{
	fs_msg_s	*m = msg;
FN;
	backup_print(m->dir_id, m->dir_indent);
}

static void readdir_peer (void *msg)
{
	readdir_cmd(msg);
}

static void backup_add_dir (void *msg)
{
	fs_msg_s	*m = msg;
FN;
	backup_add(m->dir_parent_id, m->dir_id, m->dir_name);
}

static void backup_new_dir (void *msg)
{
	fs_msg_s	*m = msg;
FN;
	backup_init(m->dir_id);
}

int init_hello (void)
{
	ki_t	key;
	int	rc;

	key = make_gate( &Hello, REQUEST | PASS_OTHER);
	rc = sw_post("dir_hello", key);
	if (rc) failure("sw_post", rc);

	key = make_gate( &Welcome, REQUEST | PASS_OTHER);
	rc = sw_request("dir_hello", key);
	if (rc) failure("sw_request", rc);

	return rc;
}
