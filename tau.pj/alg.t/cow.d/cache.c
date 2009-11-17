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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <style.h>
#include <eprintf.h>
#include <debug.h>
#include <q.h>

#include "media.h"
#include "ant.h"
#include "cache.h"
#include "btree.h" // Doing this to have type system do some of my debugging

enum { DEFAULT_NUM_BUFS = 7 };

typedef struct buf_s	buf_s;
struct buf_s {
	void		*b_data;
	buf_s		*b_head;
	buf_s		*b_next;
	tree_s		*b_tree;	// Should be (void *)
	ant_s		b_ant;
	u64		b_blkno;
	snint		b_use;
};

static addr	Num_bufs;	/* Number of buffers			*/
static void	*Data;		/* Save pointer for freeing buffers	*/
static u8	*Base;		/* Block aligned base for buffers	*/
static buf_s	*Clock;		/* Clock for victim selection		*/
static buf_s	*Buf;		/* Buffer structures and buckets	*/
static bool	Lazy = FALSE;

void blazy (void)
{
	Lazy = TRUE;
}

static inline buf_s *addr2buf (void *data)
{
	addr	x;
	addr	y;

	assert(data);

	x = (addr)data - (addr)Base;
	y = x >> BLK_SHIFT;

	assert(y < Num_bufs);
	assert((y << BLK_SHIFT) == x);

	return &Buf[y];
}

ant_s *bant (void *data)
{
	buf_s	*buf;

	buf = addr2buf(data);

	return &buf->b_ant;
}

void *bdata (ant_s *ant)
{
	buf_s	*buf = container(ant, buf_s, b_ant);

	assert(buf >= Buf);
	assert(buf < &Buf[Num_bufs]);

	return buf->b_data;
}

static inline buf_s *hash (tree_s *tree, u64 blkno)
{
	return &Buf[((addr)tree + blkno) % Num_bufs];
}

static buf_s *lookup (tree_s *tree, u64 blkno)
{
	buf_s	*buf = hash(tree, blkno);

	for (buf = buf->b_head; buf; buf = buf->b_next) {
		if ((buf->b_tree == tree) && (buf->b_blkno == blkno)) {
			if (buf->b_use < 0) {
				buf->b_use = 1;
			} else {
				++buf->b_use;
			}
			return buf;
		}
	}
	return NULL;
}

static void add (buf_s *buf)
{
	buf_s	*head = hash(buf->b_tree, buf->b_blkno);

	buf->b_next  = head->b_head;
	head->b_head = buf;
}

static void rmv (buf_s *buf)
{
	buf_s	*prev = hash(buf->b_tree, buf->b_blkno);

	if (!prev->b_head) {
		return;
	}
	if (prev->b_head == buf) {
		prev->b_head = buf->b_next;
		buf->b_next  = NULL;
		return;
	}
	for (prev = prev->b_head; prev; prev = prev->b_next) {
		if (prev->b_next == buf) {
			prev->b_next = buf->b_next;
			buf->b_next  = NULL;
			return;
		}
	}
}

static inline void bflush (buf_s *buf)
{
FN;
	flush_ant( &buf->b_ant);
}

static buf_s *victim (void)
{
	buf_s	*buf;
	unint	wrap = 0;

	for (;;) {
		++Clock;
		if (Clock == &Buf[Num_bufs]) {
			Clock = Buf;
			if (++wrap > 2) {
				bdump();
				fatal("All buffers in use");
				return NULL;
			}
		}
		buf = Clock;
		if (buf->b_use < 0) {
			if (buf->b_ant.a_state == ANT_FLUSHING) continue;
			bflush(buf);
			assert(buf->b_ant.a_state != ANT_FLUSHING);
			assert(buf->b_ant.a_state != ANT_DIRTY);
			buf->b_use = 1;
			rmv(buf);
			return buf;
		}
		if (buf->b_use == 0) {
			buf->b_use = -1;
		}
	}
}

void bdump (void)
{
	buf_s	*buf;
	unint	i;

	for (i = 0; i < Num_bufs; i++) {
		printf("%6lu. ", i);
		for (buf = Buf[i].b_head; buf; buf = buf->b_next) {
			if (buf->b_tree) {
				printf("%3d:%3ld:%3llx:%3d ",
					(int)(buf - Buf),
					buf->b_use, buf->b_blkno,
					buf->b_ant.a_state);
			} else {
				printf("%d:free ", (int)(buf - Buf));
			}
		}
		printf("\n");
	}
}

#if 0
void binuse (void)
{
	buf_s	*buf;
	unint	i;

	for (i = 0; i < Num_bufs; i++) {
		printf("%6lu. ", i);
		for (buf = Buf[i].b_head; buf; buf = buf->b_next) {
			if (buf->b_use > 0) {
				printf("%d:%ld:%llx ", (int)(buf - Buf),
					buf->b_use, buf->b_blkno);
			}
		}
		printf("\n");
	}
}
#else
void binuse (void)
{
	buf_s	*buf;
	unint	i;

	for (i = 0; i < Num_bufs; i++) {
		for (buf = Buf[i].b_head; buf; buf = buf->b_next) {
			if (buf->b_use > 0) {
				printf("inuse %d:%ld:%llx:%d\n",
					(int)(buf - Buf),
					buf->b_use, buf->b_blkno,
					buf->b_ant.a_state);
				exit(2);
			}
		}
	}
	//printf("\n");
}
#endif

void binit (int num_bufs)
{
	buf_s	*buf;
	u8	*b;

	if (Buf) {
		fprintf(stderr, "block cache already initialized\n");
		return;
	}
	if (num_bufs == 0) num_bufs = DEFAULT_NUM_BUFS;
	num_bufs |= 1; /* make sure it is at least odd */

	Clock = Buf = ezalloc(sizeof(buf_s) * num_bufs);
	Data = emalloc((num_bufs + 1) * BLK_SIZE);

	b = Base = (u8 *)(((addr)Data + BLK_MASK) & ~BLK_MASK);
	for (buf = Buf; buf < &Buf[num_bufs]; buf++) {
		init_ant( &buf->b_ant, NULL);
		buf->b_data = b;
		b += BLK_SIZE;
	}
	Num_bufs = num_bufs;
}

void bsync (tree_s *tree)
{
	buf_s	*buf;

	for (buf = Buf; buf < &Buf[Num_bufs]; buf++) {
		if (buf->b_tree == tree) {
			bflush(buf);
		}
	}
}

void bput (void *data)
{
	buf_s	*buf;

	buf = addr2buf(data);
	assert(buf->b_use > 0);
	if (!Lazy) {
		bflush(buf);
	}
	assert(buf->b_use > 0);
	--buf->b_use;
}

void bdirty (void *data)
{
	buf_s	*buf;

	buf = addr2buf(data);
	assert(buf->b_ant.a_state != ANT_FLUSHING);
	buf->b_ant.a_state = ANT_DIRTY;
}

u64 bblkno (void *data)
{
	buf_s	*buf;

	buf = addr2buf(data);
	return buf->b_blkno;
}

void *bcontext (void *data)
{
	buf_s	*buf;

	buf = addr2buf(data);
	return buf->b_tree;
}

static inline buf_s *alloc_buf (tree_s *tree, u64 blkno)
{
	buf_s	*buf;

	buf = victim();
	buf->b_tree = tree;
	buf->b_blkno = blkno;
	add(buf);

	return buf;
}

void *bget (tree_s *tree, u64 blkno)
{
	buf_s	*buf;

	if (!blkno) return NULL;

	buf = lookup(tree, blkno);
	if (buf) {
		return buf->b_data;
	}
	buf = alloc_buf(tree, blkno);

	bread(tree, buf->b_blkno, buf->b_data);

	buf->b_ant.a_state = ANT_CLEAN;
	return buf->b_data;
}

void *bnew (tree_s *tree, u64 blkno)
{
	buf_s	*buf;

	if (!blkno) return NULL;

	buf = lookup(tree, blkno);
	if (!buf) {
		buf = alloc_buf(tree, blkno);
	}
	memset(buf->b_data, 0, BLK_SIZE);
	buf->b_ant.a_state = ANT_DIRTY;
	return buf->b_data;
}

void bdepends_on (void *parent, void *child)
{
	buf_s	*pbuf;
	buf_s	*cbuf;

	pbuf = addr2buf(parent);
	cbuf = addr2buf(child);

	queen_ant( &pbuf->b_ant, &cbuf->b_ant);
}

void bchange_blkno (void *data, u64 blkno)
{
	buf_s	*buf;

	buf = addr2buf(data);
printf("change %llx -> %llx\n", buf->b_blkno, blkno);
	rmv(buf);
	buf->b_blkno = blkno;
	add(buf);
}

void bclear_dependency (void *data)
{
	buf_s	*buf;

	buf = addr2buf(data);
	clear_queen( &buf->b_ant);
}
