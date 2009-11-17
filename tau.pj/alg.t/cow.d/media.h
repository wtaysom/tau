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

enum {	BLK_SHIFT = 8,
	BLK_SIZE = (1<<BLK_SHIFT),
	BLK_MASK = BLK_SIZE - 1 };

enum {	SUPER_BLOCK = 1,
	SUPER_MAGIC = 0xbabefeedfacebeefULL };

typedef enum { BAD = 0, SUPER, LEAF, BRANCH, END } block_t;

typedef struct head_s {
	u8	h_type;
	u8	h_reserved[1];
} head_s;

typedef struct super_s {
	head_s	sp_head;
	u16	sp_reserved[3];
	u64	sp_magic;
	u64	sp_root;
	u64	sp_last;
	u64	sp_next;
} super_s;

typedef struct key_s {
	u64	k_key;
	u64	k_block;
} key_s;

typedef struct rec_s {
	s16	r_start;
	s16	r_len;
	u16	r_reserved[2];
	u64	r_key;
} rec_s;

/* b0 k1 b1 k2 b2 k3 b3 */
typedef struct branch_s {
	head_s	br_head;
	s16	br_num;
	u16	br_reserved[2];
	u64	br_first;
	key_s	br_key[0];
} branch_s;

typedef struct leaf_s {
	head_s	l_head;
	s16	l_num;
	s16	l_end;
	s16	l_total;	// change to free later
			// Another way to make end work would be to use
			// &l_rec[l_num] as the index point of l_end
			// That is what I did in insert!!!
	rec_s	l_rec[0];
} leaf_s;

enum {
	KEYS_PER_BRANCH = (BLK_SIZE - sizeof(branch_s)) / sizeof(key_s),
	MAX_FREE = BLK_SIZE - sizeof(leaf_s)
};

static inline u8 type (void *data)
{
	head_s	*h = data;

	return h->h_type;
}

static inline void set_type (void *data, unsigned type)
{
	head_s	*h = data;

	h->h_type = type;
}
