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
#include <string.h>
#include <assert.h>

#include <crc.h>
#include <debug.h>
#include <eprintf.h>

#include "crfs.h"
#include "ht.h"
#include "twins.h"

Volume_s	*VolA;
Volume_s	*VolB;
Htree_s	*TreeA;
Htree_s	*TreeB;

int delete_twins (Key_t key)
{
	int	rc;

	rc = t_delete(TreeA, key);
	if (rc) {
		printf("Failed delete key for TreeA \"%llx\" err=%d\n",
			(u64)key, rc);
	}
	rc = t_delete(TreeB, key);
	if (rc) {
		printf("Failed delete key for TreeB \"%llx\" err=%d\n",
			(u64)key, rc);
	}
	return rc;
}

int insert_twins (Key_t key, Lump_s val)
{
	int	rc;

	rc = t_insert(TreeA, key, val);
	if (rc) {
		printf("TreeA failed to insert \"%llx\" err=%d\n",
			(u64)key, rc);
		return rc;
	}
	rc = t_insert(TreeB, key, val);
	if (rc) {
		printf("TreeB failed to insert \"%llx\" err=%d\n",
			(u64)key, rc);
		return rc;
	}
	return rc;
}

int find_twins (Key_t key, Lump_s *val)
{
	int	rc_a;
	int	rc_b;

	rc_a = t_find(TreeA, key, val);
	rc_b = t_find(TreeB, key, val);

	if (rc_a != rc_b) {
		printf("find_twins \"%llx\" %d!=%d\n", (u64)key, rc_a, rc_b);
	}
	return rc_a;
}

int next_twins (
	Key_t	prev_key,
	Key_t	*key,
	Lump_s	*val)
{
	Key_t	key_a;
	Key_t	key_b;
	Lump_s	val_a;
	Lump_s	val_b;
	int	rc_a;
	int	rc_b;

	rc_a = t_next(TreeA, prev_key, &key_a, &val_a);
	rc_b = t_next(TreeB, prev_key, &key_b, &val_b);

	if (rc_a && (rc_a == rc_b)) {
		return rc_a;
	}
	if ((rc_a != rc_b)
		|| (key_a != key_b)
		|| (strcmp(val_a.d, val_b.d) != 0))
	{
		printf("Failed next_twins\n");
		if (rc_a) {
			printf("a: rc = %d\n", rc_a);
		} else {
			printf("a: key = %llx val = %s\n",
				(u64)key_b, (char *)val_b.d);
		}
		if (rc_b) {
			printf("b: rc = %d\n", rc_b);
		} else {
			printf("b: key = %llx val = %s\n",
				(u64)key_b, (char *)val_b.d);
		}
	}
	if (!rc_a) {
		*key = key_a;
		*val = val_a;
	}
	return rc_a;
}

void init_twins (char *devA, char *devB)
{
	VolA = crfs_create(devA);
	VolB = crfs_create(devB);
	TreeA = crfs_htree(VolA);
	TreeB = crfs_htree(VolB);	
//	blazy(TreeA.t_dev);
}
