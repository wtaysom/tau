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

#include <eprintf.h>
#include <debug.h>

#include <tau/switchboard.h>

#include <sw.h>

typedef struct key_s {
	unsigned	k_key;
	unsigned	k_type;
	name_s		*k_name;
	void		*k_owner;
} key_s;

static key_s	Key[MAX_KEYS] = { { 0, 0, NULL, NULL } };
static key_s	*NextKey = &Key[1];

void key_dump (void)
{
	key_s	*k;
FN;
	for (k = Key; k != NextKey; k++) {
		printf("key: %u type %u owner %p %s\n",
			k->k_key, k->k_type, k->k_owner, k->k_name->nm_name);
	}
}

void key_add (unsigned key, name_s *name, void *owner, unsigned type)
{
	key_s	*k;

	k = NextKey;
	if (k == &Key[MAX_KEYS]) {
		eprintf("Too many keys");
	}
	++NextKey;
	k->k_key   = key;
	k->k_type  = type;
	k->k_owner = owner;
	k->k_name  = name;
}

void key_delete (void *owner)
{
	key_s	*k;
FN;
	for (k = Key; k != NextKey; k++) {
		if (k->k_owner == owner) {
			destroy_key_tau(k->k_key);
			zero(*k);
		}
	}
}

int dup_send_key (unsigned to_key, unsigned key, name_s *name, u64 sequence)
{
	sw_msg_s	msg;
	int		rc;
FN;
	rc = duplicate_key_tau(key, &msg);
	if (rc) {
		eprintf("dup_send_key duplicate_key_tau %s %d\n", name, rc);
		return rc;
	}
	msg.sw_method = SW_KEY_SUPPLIED;
	msg.sw_sequence = sequence;
	memcpy( &msg.sw_name, name, name->nm_length+1);
	rc = send_key_tau(to_key, &msg);
	if (rc) {
		destroy_key_tau(msg.q.q_passed_key);
		eprintf("dup_send_key send_key_tau %s %d\n", name, rc);
		return rc;
	}
	return 0;
}

int key_send (unsigned to_key, name_s *name, unsigned type)
{
	key_s	*k;
	int	rc;
	int	cnt = 0;
FN;
	for (k = Key; k != NextKey; k++) {
		if ((k->k_name == name) && (k->k_type & type)) {
			rc = dup_send_key(to_key, k->k_key, name, 0);
			if (!rc) {
				++cnt;
			}
		}
	}
	return cnt;
}

int key_send_any (unsigned to_key, name_s *name, unsigned type)
{
	key_s	*k;
	int	rc;
FN;
	for (k = Key; k != NextKey; k++) {
		if ((k->k_name == name) && (k->k_type & type)) {
			rc = dup_send_key(to_key, k->k_key, name, 0);
			return rc;
		}
	}
	destroy_key_tau(to_key);
	return 0;
}

int key_send_all (unsigned to_key, unsigned type)
{
	key_s	*k;
	int	rc;
	int	cnt = 0;
FN;
	// Could use wild carding on name :-)
	for (k = Key; k != NextKey; k++) {
		if (k->k_type & type) {
			rc = dup_send_key(to_key, k->k_key, k->k_name, 0);
			if (!rc) {
				++cnt;
			}
		}
	}
	return cnt;
}

int key_next (unsigned to_key, name_s *name, unsigned type, u64 sequence)
{
	key_s	*k;
	int	rc;
FN;
	if (++sequence >= MAX_KEYS) {
		destroy_key_tau(to_key);
		return 0;
	}
PRu(sequence);
	for (k = &Key[sequence]; k < NextKey; k++) {
		if ((k->k_name == name) && (k->k_type & type)) {
			rc = dup_send_key(to_key, k->k_key, name, k - Key);
			return rc;
		}
	}
	destroy_key_tau(to_key);
	return 0;
}
