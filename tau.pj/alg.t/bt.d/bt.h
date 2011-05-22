/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _BT_H_
#define _BT_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

#ifndef _LUMP_H_
#include <lump.h>
#endif

enum {	BT_ERR_NOT_FOUND = 2000,	/* Key not found */
	BT_ERR_BAD_NODE,		/* Internal error: bad node */
	FAILURE = -1
};

typedef struct Btree_s Btree_s;

typedef int (*Apply_f)(Lump_s key, Lump_s val, void *user);

Btree_s *t_new(char *file, int num_bufs);
void     t_dump(Btree_s *t);
int      t_insert(Btree_s *t, Lump_s key, Lump_s val);
Lump_s   t_find(Btree_s *t, Lump_s key);
void     pr_all_records(Btree_s *t);
int      t_map(Btree_s *t, Apply_f func, void *user);
int      t_audit(Btree_s *t);

#endif
