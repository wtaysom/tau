/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _UTIL_H_
#define _UTIL_H_ 1

#include <lump.h>

typedef void (*krecFunc)(u64 key, void *user);

typedef void (*recFunc)(Lump_s key, Lump_s val, void *user);

void Pause(void);

char *rndstring(unint n);
Lump_s rnd_lump(void);
Lump_s fixed_lump(unint n);
Lump_s seq_lump(void);

void k_init(void);
void k_seed(u64 seed);
void k_add (u64 key);
void k_for_each(krecFunc f, void *user);
snint k_rand_index (void);
u64 k_get_rand(void);
u64 k_delete_rand(void);
int k_should_delete(s64 count, s64 level);
u64 k_rand_key(void);
bool k_rand_percent(unint percent);

void r_add(Rec_s rec);
snint r_rand_index(void);
void r_for_each(recFunc f, void *user);
Rec_s r_get_rand(void);
Lump_s r_delete_rand(void);

#endif
