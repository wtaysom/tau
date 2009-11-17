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

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <style.h>
#include <debug.h>
#include <eprintf.h>

#include <blk.h>
#include <dirfs.h>

#include <dir_hdr.h>

super_s		*Super;

int is_local (u64 id)
{
	unsigned	share;

	share = id % Super->sp_total_shares;
	return !!((1ULL << share) & Super->sp_my_shares);
}

int is_backup (u64 id)
{
	unsigned	backup;

	backup = id % Super->sp_total_shares;
	return !!((1ULL << backup) & Super->sp_my_backup);
}

u64 dir_lookup (u64 parent, char *name)
{
	dname_s	*dn;
	dir_s	*p;
	unint	i;
	u64	child = 0;
FN;
PRu(parent);
PRs(name);
	if (is_local(parent)) {
		p = bget(Super->sp_dev, parent);

		for (i = 0; i < DIR_SZ; i++) {
			dn = &p->dr_n[i];
			if (dn->dn_num && (strcmp(dn->dn_name, name) == 0)) {
				child = dn->dn_num;
				break;
			}
		}
		brel(p);
	} else {
		/* Remote - Can't do a synchronous operation at this point.
		 * Should save the state or have my own threading at this
		 * point.
		 */
		fs_msg_s	m;
		int		rc;

		m.m_method      = PEER_LOOKUP;
		m.dir_parent_id = parent;
		strcpy(m.dir_name, name);
		rc = call_tau(remote(parent), &m);
		if (rc) {
			eprintf("dir_lookup call_tau failed %d", rc);
			return 0;
		}
		child = m.dir_id;
	}
	return child;
}

void send_dir_add (u64 parent, u64 child, char *name)
{
	fs_msg_s	m;
	int		rc;
FN;
	m.m_method      = BACKUP_ADDDIR;
	m.dir_parent_id = parent;
	m.dir_id        = child;
	strcpy(m.dir_name, name);
	rc = send_tau(backup(parent), &m);
	if (rc) {
		eprintf("send_dir_add send_tau failed %d", rc);
	}
}

void dir_add (u64 parent, u64 child, char *name)
{
	dname_s	*dn;
	dir_s	*p;
	unint	i;
FN;
	if (is_local(parent)) {
		p = bget(Super->sp_dev, parent);

		for (i = 0; i < DIR_SZ; i++) {
			dn = &p->dr_n[i];
			if (!dn->dn_num) {
				strncpy(dn->dn_name, name, NAME_SZ);
				dn->dn_num = child;
				send_dir_add(parent, child, name);
				bdirty(p);
				brel(p);
				return;
			}
		}
		brel(p);
		eprintf("dir_add no room left in dir %lld", parent);
	} else {
		/* Remote - Can't do a synchronous operation at this point.
		 * Should save the state or have my own threading at this
		 * point.
		 */
		fs_msg_s	m;
		int		rc;

		m.m_method      = PEER_ADDDIR;
		m.dir_parent_id = parent;
		m.dir_id        = child;
		strcpy(m.dir_name, name);
		rc = send_tau(remote(parent), &m);
		if (rc) {
			eprintf("dir_add send_tau failed %d", rc);
		}
	}
}

void backup_add (u64 parent, u64 child, char *name)
{
	dname_s	*dn;
	dir_s	*p;
	unint	i;
FN;
	assert(is_backup(parent));

	p = bget(Super->sp_dev, parent);

	for (i = 0; i < DIR_SZ; i++) {
		dn = &p->dr_n[i];
		if (!dn->dn_num) {
			strncpy(dn->dn_name, name, NAME_SZ);
			dn->dn_num = child;
			bdirty(p);
			brel(p);
			return;
		}
	}
	brel(p);
	eprintf("backup_add no room left in dir %lld", parent);
}

static void dir_init (u64 id)
{
	dir_s	*dir;
FN;
	assert(is_local(id));
	dir = bnew(Super->sp_dev, id);
	dir->dr_id = id;
	brel(dir);
}

void backup_init (u64 id)
{
	dir_s	*dir;
FN;
	assert(is_backup(id));
	if (id > Super->sp_backup_num) {
		Super->sp_backup_num = id;
		bflush(Super);
	}
	dir = bnew(Super->sp_dev, id);
	dir->dr_id = id;
	brel(dir);
}

void send_dir_new (u64 id)
{
	fs_msg_s	m;
	int		rc;
FN;
	m.m_method = BACKUP_NEWDIR;
	m.dir_id   = id;
	rc = send_tau(backup(id), &m);
	if (rc) {
		eprintf("send_dir_new send_tau failed %d", rc);
	}
}

u64 dir_new (void)
{
	u64		id;
FN;
	while (!is_local(++Super->sp_id))
		;
	id = Super->sp_id;
	bflush(Super);
	dir_init(id);
	send_dir_new(id);
	return id;
}

u64 dir_create (void)
{
	static unsigned	r = 37;
FN;
	if (is_local(++r)) {
		return dir_new();
	} else {
		/* Remote - Can't do a synchronous operation at this point.
		 * Should save the state or have my own threading at this
		 * point.
		 */
		fs_msg_s	m;
		int		rc;

		m.m_method = PEER_NEWDIR;
		rc = call_tau(remote(r), &m);
		if (rc) {
			eprintf("dir_create call_tau failed %d", rc);
			return 0;
		}
		return m.dir_id;
	}
	return 0;
}

static void pr_indent (unint indent)
{
	while (indent--) {
		printf("\t");
	}
}

void dir_print (u64 id, unint indent)
{
	dname_s	*dn;
	dir_s	*d;
	unint	i;
FN;
	if (is_local(id)) {
		d = bget(Super->sp_dev, id);
		if (!d) {
			printf("%8lld not found\n", id);
			return;
		}
		pr_indent(indent);
		printf("%llu------------------\n", id);
		for (i = 0; i < DIR_SZ; i++) {
			dn = &d->dr_n[i];
			if (dn->dn_num) {
				pr_indent(indent);
				printf("%8llu %.*s\n",
					dn->dn_num, NAME_SZ, dn->dn_name);
				dir_print(dn->dn_num, indent + 1);
			}
		}
		brel(d);
	} else {
		/* Remote - Can't do a synchronous operation at this point.
		 * Should save the state or have my own threading at this
		 * point.
		 */
		fs_msg_s	m;
		int		rc;

		m.m_method       = PEER_PRINT;
		m.dir_id     = id;
		m.dir_indent = indent;
		rc = send_tau(remote(id), &m);
		if (rc) {
			eprintf("dir_print send_tau failed %d", rc);
		}
	}
}

void pr_tree (void)
{
FN;
	printf("<root>\n");
	dir_print(ROOT, 0);
}

void backup_print (u64 id, unint indent)
{
	dname_s	*dn;
	dir_s	*d;
	unint	i;
FN;
	if (is_backup(id)) {
		d = bget(Super->sp_dev, id);
		if (!d) {
			printf("%8lld not found\n", id);
			return;
		}
		pr_indent(indent);
		printf("%llu------------------\n", id);
		for (i = 0; i < DIR_SZ; i++) {
			dn = &d->dr_n[i];
			if (dn->dn_num) {
				pr_indent(indent);
				printf("%8llu %.*s\n",
					dn->dn_num, NAME_SZ, dn->dn_name);
				backup_print(dn->dn_num, indent + 1);
			}
		}
		brel(d);
	} else {
		/* Remote - Can't do a synchronous operation at this point.
		 * Should save the state or have my own threading at this
		 * point.
		 */
		fs_msg_s	m;
		int		rc;

		m.m_method       = BACKUP_PRINT;
		m.dir_id     = id;
		m.dir_indent = indent;
		rc = send_tau(backup(id), &m);
		if (rc) {
			eprintf("dir_print send_tau failed %d", rc);
		}
	}
}

void pr_backup (void)
{
FN;
	printf("<backup root>\n");
	backup_print(ROOT, 0);
}

const char *next_name (const char *path, char *name)
{
	unint	length;
FN;
	if (path == NULL) {
		return NULL;
	}
	/* Skip leading slashes */
	while (*path == SLASH) {
		++path;
	}
	for (length = 0;;) {
		if (*path == EOS){
			if (!length) path = NULL;
			break;
		}
		if (*path == SLASH) {
			++path;
			break;
		}
		if (length < NAME_SZ) {
			*name++ = *path++;
			++length;
		}
	}
	*name = EOS;
	return path;
}

void cr_dir (const char *path)
{
	char	name[NAME_SZ+1];
	u64	parent;
	u64	child;
FN;
	for (parent = ROOT;; parent = child) {
		path = next_name(path, name);
PRs(path);
		if (!path) break;
PRs(name);
		child = dir_lookup(parent, name);
		if (!child) {
			child = dir_create();
			dir_add(parent, child, name);
		}
	}
}

dir_s *read_dir (u64 id)
{
FN;
	return bget(Super->sp_dev, id);
}

void upshares (u64 mask)
{
	Super->sp_my_shares |= (Super->sp_my_backup & mask);
	bflush(Super);
}

void mkfs_dir (u64 total_shares, u64 my_shares, u64 my_backup)
{
	fs_msg_s	msg;
FN;
	Super->sp_total_shares = total_shares;
	Super->sp_my_shares    = my_shares;
	Super->sp_my_backup    = my_backup;

	Super->sp_id = ROOT;

	if (is_local(ROOT)) {
		dir_init(ROOT);
	}
	msg.m_method     = SHARE_ADD;
	msg.mk_my_shares = my_shares;
	msg.mk_my_backup = my_backup;
	peer_broadcast( &msg);
}

int init_fs (void)
{
	device_s	*dev;
FN;
	dev = init_blk("/tmp/dir");
PRp(Super);
	if (!dev) return 2;

	Super = bget(dev, SUPER);
	if (!Super) return 2;

	Super->sp_dev = dev;

	return 0;
}
