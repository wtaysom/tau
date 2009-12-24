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

#include <style.h>
#include <tau/debug.h>
#include <tau/disk.h>
#include <tau/fsmsg.h>
#include <crc.h>
#include <kache.h>
#include <tau_err.h>

enum {	UNIQUE_SHIFT = 8,
	UNIQUE_MASK = (1 << UNIQUE_SHIFT) - 1,
	HASH_MASK = ~UNIQUE_MASK };

typedef struct d_s {
	u64	d_ino;
	u64	d_off;
	u8	d_type;
	char	*d_name;
} d_s;

typedef struct dir_s {
	u64	dr_ino;
	u64	dr_off;
	u8	dr_type;
	char	dr_name[0];
} dir_s;

//===================================================================
static char *dump_name       (tree_s *tree, u64 rec_key, void *rec, unint size);
static u64   root_dir        (tree_s *tree);
static int   change_root_dir (tree_s *tree, void *root);
static int   pack_dir        (void *to, void *from, unint size);

tree_species_s	Kache_dir_species = {
	"Kache dir",
	ts_dump:	dump_name,
	ts_root:	root_dir,
	ts_change_root:	change_root_dir,
	ts_pack:	pack_dir};

//===================================================================

static u32 hash_dir (const u8 *s)
{
	/* Make sure sign bit is cleared and
	 * preserve the least significant byte
	 */
	return (hash_string_32_tau((const char *)s) >> (UNIQUE_SHIFT + 1))
			<< UNIQUE_SHIFT;
}

//===================================================================

typedef struct delete_search_s {
	const u8	*name;
	u64		key;
} delete_search_s;

static int delete_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	size)
{
	delete_search_s	*s = data;
	dir_s		*dir = rec;

	if ((s->key & HASH_MASK) < (rec_key & HASH_MASK)) {
		return qERR_NOT_FOUND;
	}
	assert((s->key & HASH_MASK) == (rec_key & HASH_MASK));
	if (strcmp(s->name, dir->dr_name) == 0) {
		s->key = rec_key;
		return 0;
	}
	return qERR_TRY_NEXT;
}

int kache_delete_dir (tree_s *ptree, const u8 *name)
{
	delete_search_s		s;
	int	rc;

	s.key = hash_dir(name);
	s.name = name;
	rc = kache_search(ptree, s.key, delete_search, &s);
	if (rc) {
		dprintk("delete_dir [%d] couldn't find %s\n", rc, name);
		return rc;
	}
	rc = kache_delete(ptree, s.key);
	if (rc) {
		eprintk("Failed to delete string \"%s\" err=%d\n",
			name, rc);
		return rc;
	}
	return 0;
}

//===================================================================

typedef struct insert_search_s {
	u64		key;
	bool		set_new_key;	/* used to resolve key collisions */
	d_s		*d;
} insert_search_s;

static int insert_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	size)
{
	insert_search_s	*s = data;
	d_s		*d = s->d;
	dir_s		*dir = rec;

	if ((s->key & HASH_MASK) < (rec_key & HASH_MASK)) {
		return qERR_NOT_FOUND;
	}
	assert((s->key & HASH_MASK) == (rec_key & HASH_MASK));
	if (strcmp(d->d_name, dir->dr_name) == 0) {
		return qERR_DUP;
	}
	if (!s->set_new_key) {
		if (s->key != rec_key) {
			s->set_new_key = TRUE;
		} else {
			s->key = rec_key + 1;
			if ((s->key & UNIQUE_MASK) == 0) {
				eprintk("OVERFLOW: %s\n", d->d_name);
				return qERR_HASH_OVERFLOW;
			}
		}
	}
	return qERR_TRY_NEXT;
}

static int pack_dir (void *to, void *from, unint size)
{
	insert_search_s	*s = from;
	d_s		*d = s->d;
	dir_s		*dir = to;

	assert(strlen(d->d_name) + 1 + sizeof(dir->dr_ino)
		+ sizeof(dir->dr_off) + sizeof(dir->dr_type) == size);
	dir->dr_ino  = d->d_ino;
	dir->dr_off  = d->d_off;
	dir->dr_type = d->d_type;
	strcpy(dir->dr_name, d->d_name);
	return 0;
}

int insert_dir (tree_s *ptree, d_s *d)
{
	insert_search_s	s;
	unint		size;
	int		rc;

	s.key = hash_dir(d->d_name);
	s.d = d;
	size = strlen(d->d_name) + 1 + sizeof(d->d_ino)
			+ sizeof(d->d_off) + sizeof(d->d_type);
	rc = kache_insert(ptree, s.key, &s, size);
	if (rc == 0) return 0;

	if (rc != qERR_DUP) {
		eprintk("Failed to insert string \"%s\" err=%d\n",
			d->d_name, rc);
		return rc;
	}

	s.set_new_key = FALSE;
	rc = kache_search(ptree, s.key, insert_search, &s);
	if (rc == qERR_DUP) {
		dprintk("Duplicate string found \"%s\" err=%d\n",
			d->d_name, rc);
		return rc;
	}
	rc = kache_insert(ptree, s.key, &s, size);
	if (rc) {
		eprintk("Failed to insert string \"%s\" err=%d\n",
			d->d_name, rc);
		return rc;
	}
	return 0;
}

//===================================================================

typedef struct find_search_s {
	u64		key;
	u64		ino;
	const u8	*name;
} find_search_s;

static int find_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	size)
{
	find_search_s	*s = data;
	dir_s		*dir = rec;

	if ((s->key & HASH_MASK) < (rec_key & HASH_MASK)) {
		return qERR_NOT_FOUND;
	}
	assert((s->key & HASH_MASK) == (rec_key & HASH_MASK));
	if (strcmp(s->name, dir->dr_name) == 0) {
		s->ino = dir->dr_ino;
		return 0;
	}
	return qERR_TRY_NEXT;
}

u64 kache_find_dir (tree_s *ptree, const u8 *name)
{
	find_search_s	s;
	int		rc;

	s.key = hash_dir(name);
	s.ino = 0;
	s.name = name;

	rc = kache_search(ptree, s.key, find_search, &s);
	if (rc) return 0;
	return s.ino;
}

//===================================================================

typedef struct next_search_s {
	u64	key;
	u8	*name;
	u64	*ino;
} next_search_s;

static int next_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	size)
{
	next_search_s	*s = data;
	dir_s		*dir = rec;
FN;
	strcpy(s->name, dir->dr_name);
	*s->ino = dir->dr_ino;
	s->key = rec_key + 1;
	return 0;
}

int kache_next_dir (
	tree_s	*ptree,
	u64	key,
	u64	*next_key,
	u8	*name,
	u64	*ino)
{
	next_search_s	s;
	int		rc;
FN;
	s.key = key;
	s.name = name;
	s.ino  = ino;
	rc = kache_search(ptree, s.key, next_search, &s);
	if (!rc) {
		*next_key = s.key;
	}
	return rc;
}

//===================================================================

static char *dump_name (tree_s *tree, u64 rec_key, void *rec, unint size)
{
	static char	buf[2 * TAU_FILE_NAME_LEN];

	dir_s	*dir = rec;

	snprintf(buf, sizeof(buf), "%lld %s", dir->dr_ino, dir->dr_name);
	return buf;
}

static u64 root_dir (tree_s *tree)
{
	return tree->t_root;
}

static int change_root_dir (tree_s *tree, void *root)
{
FN;
	tree->t_root = (addr)root;

	return 0;
}

//===================================================================

static void *alloc_dir_list (void)
{
	dir_list_s	*dl;
FN;
	dl = (dir_list_s *)__get_free_page(GFP_KERNEL | __GFP_HIGHMEM);
	if (!dl) {
		eprintk("no memory");
	}
	dl->dl_next = 0;
	return dl;
}

static void free_dir_list (void *dl)
{
FN;
	free_page((addr)dl);
}

unint unpack (
	dir_list_s	*dl,
	unint		offset,
	d_s		*d)
{
	unint	i;

	if (offset >= dl->dl_next) return 0;

	i = offset;
	UNPACK(d->d_ino, dl->dl_list, i);
	UNPACK(d->d_off, dl->dl_list, i);
	//UNPACK(d->d_type, dl->dl_data, i);
	d->d_name = &dl->dl_list[i];
	i += strlen(d->d_name) + 1;

	return i;
}

static dir_list_s *get_dir_list (unint ino_key, u64 pos)
{
	dir_list_s	*dl;
	fsmsg_s		msg;
	int		rc;
FN;
	dl = alloc_dir_list();
	if (!dl) {
		eprintk("alloc_dir_list");
		return NULL;
	}
	msg.m_method = FILE_READDIR;
	msg.rd_cookie = pos;
	enter_tau(Fs_avatar);//XXX: don't like this
	rc = getdata_tau(ino_key, &msg, PAGE_SIZE, dl);
	exit_tau();
	if (rc) {
		free_dir_list(dl);
		eprintk("getdata_tau %d", rc);
		return NULL;
	}
	return dl;
}

u64 do_readdir (unint key, void *dirent, filldir_t filldir, u64 pos)
{
	dir_list_s	*dl = NULL;
	d_s		d;
	unint		offset;
	int		done;
FN;
	dl = get_dir_list(key, pos);
	if (!dl) goto exit;

	for (offset = 0;;) {
		offset = unpack(dl, offset, &d);
		if (!offset) {
			free_dir_list(dl);
			++pos;
			dl = get_dir_list(key, pos);
			if (!dl) goto exit;
			offset = unpack(dl, 0, &d);
			if (!offset) goto exit;
		}
		done = filldir(dirent, d.d_name,
				strlen(d.d_name), d.d_ino, d.d_off, 0/*type?*/);
		if (done) break;
		pos = d.d_off;
	}
exit:
	if (dl) free_dir_list(dl);
	return pos;
}

int fill_dir_tree (knode_s *knode)
{
	dir_list_s	*dl = NULL;
	loff_t		pos = 0;
	d_s		d;
	unint		offset;
	int		rc;
FN;
	dl = get_dir_list(knode->ki_key, pos);
	if (!dl) goto exit;

	for (offset = 0;;) {
		offset = unpack(dl, offset, &d);
		if (!offset) {
			++pos;
			free_dir_list(dl);
			dl = get_dir_list(knode->ki_key, pos);
			if (!dl) goto exit;
			offset = unpack(dl, 0, &d);
			if (!offset) goto exit;
		}
		rc = insert_dir( &knode->ki_tree, &d);
		if (rc) goto exit;
		pos = d.d_off;
	}
exit:
	if (dl) free_dir_list(dl);
	return 0;
}

//===================================================================

int add_dir_entry (
	knode_s		*parent,
	struct dentry	*dentry,
	u64		ino,
	int		mode)
{
	struct qstr	*qname = &dentry->d_name;
	fsmsg_s		m;
	int		rc;
FN;
	m.crt_parent = 0;
	m.crt_mode   = mode;
	m.crt_child  = ino;
	m.crt_uid    = 0;
	m.crt_gid    = 0;
	m.m_method   = ADD_DIR_ENTRY;
	enter_tau(parent->ki_replica->rp_avatar);
	rc = putdata_tau(parent->ki_key, &m, qname->len, qname->name);
	exit_tau();
	if (rc) {
		eprintk("putdata_tau %d", rc);
		return rc;
	}
	return 0;
}

u64 lookup_dir_entry (
	knode_s		*parent,
	struct dentry	*dentry)
{
	struct qstr	*qname = &dentry->d_name;
	fsmsg_s		m;
	int		rc;
FN;
	m.crt_parent = 0;
	m.crt_mode   = 0;
	m.crt_child  = 0;
	m.crt_uid    = 0;
	m.crt_gid    = 0;
	m.m_method   = LOOKUP_DIR_ENTRY;
	enter_tau(parent->ki_replica->rp_avatar);
	rc = putdata_tau(parent->ki_key, &m, qname->len, qname->name);
	exit_tau();
	if (rc) {
		eprintk("putdata_tau %d", rc);
		return 0;
	}
	return m.crr_ino;
}
