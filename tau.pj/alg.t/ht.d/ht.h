/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _HT_H_
#define _HT_H_ 1

#include <lump.h>
#include <style.h>

#include "crfs.h"

enum {	HT_ERR_NOT_FOUND = 2000, /* Key not found */
	HT_ERR_BAD_NODE,  	/* Internal error: bad node */
	HT_TRY_NEXT,		/* Try searching next record */
	HT_DONE,		/* Done with operation */
	HT_ERR_HASH_OVERFLOW,	/* Too many hash collisions */
	HT_ERR_DUP,		/* Key already exits */
	HT_ERR_DIFFERENT,	/* Two things are different */
	FAILURE = -1 };


typedef struct Hrec_s {
	Key_t	key;
	Lump_s	val;
} Hrec_s;

typedef int	(*Apply_f)(Hrec_s rec, void *user);

typedef struct Audit_s {
	u64	leaves;
	u64	branches;
	u64	records;
	int	max_depth;
} Audit_s;

Htree_s *t_new(void);
void t_dump  (Htree_s *t);
int t_insert (Htree_s *t, Key_t key, Lump_s val);
int t_find   (Htree_s *t, Key_t key, Lump_s *val);
int t_delete (Htree_s *t, Key_t key);
int t_map    (Htree_s *t, Apply_f func, void *user);
int t_audit  (Htree_s *t, Audit_s *audit);
int t_next   (Htree_s *t, Key_t prev_key, Key_t *key, Lump_s *val);
int t_compare(Htree_s *a, Htree_s *b);
Stat_s t_get_stats(Htree_s *t);
void pr_all_records(Htree_s *t);
#endif
