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

#include <tau/msg.h>
#include <msg_debug.h>
#include <msg_internal.h>

/*
 * Key chaind is arranged so 1 is allocated first and 0 is never
 * allocated.
 */
void init_key_chain (avatar_s *avatar)
{
	key_chain_s	*chain = &avatar->av_chain;
	key_s		*key;
FN;
	zero(*chain);
	for (key = &chain->kc_chain[MAX_KEYS-1];
		key > &chain->kc_chain[STD_KEYS];
		key--)
	{
		key->k_id = 0;
		*(key_s **)key = chain->kc_top;
		chain->kc_top = key;
	}
}

ki_t new_key (avatar_s *avatar, key_s *passed_key)
{
	key_chain_s	*chain = &avatar->av_chain;
	key_s		*key;
FN;
	key = chain->kc_top;
	if (!key) {
		unint	i;
		dprintk("chain=%p top=%p", chain, chain->kc_top);
		for (i = 0; i < MAX_KEYS; i++) {
			dprintk("%ld. %llx", i, chain->kc_chain[i].k_id);
		}
		return 0;
	}
	chain->kc_top = *(key_s **)key;
	*key = *passed_key;

	return key - chain->kc_chain;
}

int assign_std_key (avatar_s *avatar, ki_t std_key_index, key_s *key)
{
	key_chain_s	*chain = &avatar->av_chain;
	key_s		*std_key;
FN;
	if (!std_key_index || (std_key_index > STD_KEYS)) return ENOTSTD;

	std_key = &chain->kc_chain[std_key_index];
	if (std_key->k_id) return EINUSE;

	*std_key = *key;

	return 0;
}

key_s *get_key (avatar_s *avatar, ki_t key_index)
{
	key_chain_s	*chain = &avatar->av_chain;
	key_s		*key;

	if (key_index >= MAX_KEYS) {
		return NULL;
	}
	key = &chain->kc_chain[key_index];
	if (key->k_id) {
FN;
		return key;
	}
	return NULL;
}

void delete_key (avatar_s *avatar, key_s *key)
{
	key_chain_s	*chain = &avatar->av_chain;
FN;
	key->k_id = 0;
	if (key > &chain->kc_chain[STD_KEYS]) {
		*(key_s **)key = chain->kc_top;
		chain->kc_top = key;
	}
}

int read_key (avatar_s *avatar, ki_t key_index, key_s *buf)
{
	key_s	*key;

	key = get_key(avatar, key_index);
	if (key) {
		*buf = *key;
		return 0;
	} else {
		return EBADKEY;
	}
}
