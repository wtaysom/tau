/****************************************************************************
 |  (C) Copyright 2009 Novell, Inc. All Rights Reserved.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <style.h>
#include <tau/fsmsg.h>
#include <mystdlib.h>
#include <eprintf.h>
#include <debug.h>

#include <bagio.h>

enum { DEFAULT_NUM_BUFS = 3 };

typedef struct buf_s	buf_s;
struct buf_s {
	void	*b_data;
	buf_s	*b_head;
	buf_s	*b_next;
	u64	b_blkno;
	snint	b_use;
	bool	b_dirty;
	ki_t	b_key;
};

static unint	Num_bufs;	/* Number of buffers			*/
static void	*Data;		/* Save pointer for freeing buffers	*/
static u8	*Base;		/* Block aligned base for buffers	*/
static buf_s	*Clock;		/* Clock for victim selection		*/
static buf_s	*Buf;		/* Buffer structures and buckets	*/

buf_s *data2buf (void *data)
{
	buf_s	*buf;
	unint	i;

	i = ((addr)data - (addr)Base) >> BLK_SHIFT;
	if (i >= Num_bufs) eprintf("bad block pointer %p", data);
	buf = &Buf[i];
	if (buf->b_data != data) eprintf("block ponters don't match %p!=%p",
					buf->b_data, data);
	return buf;
}

static inline buf_s *hash (ki_t key, u64 blkno)
{
	return &Buf[(key * blkno) % Num_bufs];
}

static buf_s *lookup (ki_t key, u64 blkno)
{
	buf_s	*buf = hash(key, blkno);

	for (buf = buf->b_head; buf; buf = buf->b_next) {
		if ((buf->b_key == key) && (buf->b_blkno == blkno)) {
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
	buf_s	*head = hash(buf->b_key, buf->b_blkno);

	buf->b_next  = head->b_head;
	head->b_head = buf;
}

static void rmv (buf_s *buf)
{
	buf_s	*prev = hash(buf->b_key, buf->b_blkno);

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

static void bflush (buf_s *buf)
{
	fsmsg_s	m;
	int	rc;

	if (buf->b_dirty) {
		m.m_method = WRITE_BAG;
		m.io_blkno = buf->b_blkno;
		rc = putdata_tau(buf->b_key, &m, BLK_SIZE, buf->b_data);
		if (rc) {
			eprintf("putdata_tau at %ld:%lld failed = %d",
				buf->b_key, buf->b_blkno, rc);
			return;
		}
		buf->b_dirty = FALSE;
	}
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
				eprintf("All buffers in use");
				return NULL;
			}
		}
		buf = Clock;
		if (buf->b_use < 0) {
			bflush(buf);
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
			if (buf->b_key) {
				printf("%d:%ld:%llx ", (int)(buf - Buf),
					buf->b_use, buf->b_blkno);
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
				printf("inuse %d:%ld:%llx\n", (int)(buf - Buf),
					buf->b_use, buf->b_blkno);
				exit(2);
			}
		}
	}
	//printf("\n");
}
#endif

void binit (int num_bufs)
{
	unint	i;
	unint	n;

	if (Buf) {
		fprintf(stderr, "block cache already initialized\n");
		return;
	}
	if (num_bufs == 0) num_bufs = DEFAULT_NUM_BUFS;
	n = findprime(num_bufs);
	if (!n) eprintf("too many buffers %d", num_bufs);

	Clock = Buf = ezalloc(sizeof(buf_s) * num_bufs);
	Data = emalloc((n + 1) * BLK_SIZE);

	Base = (u8 *)(((addr)Data + BLK_MASK) & ~BLK_MASK);
	for (i = 0; i < n; i++) {
		Buf[i].b_data = &Base[i * BLK_SIZE];
	}
	Num_bufs = n;
}

void bdirty (void *data)
{
	buf_s	*buf;

	buf = data2buf(data);
	buf->b_dirty = TRUE;
}

void bput (void *data)
{
	buf_s	*buf;

	buf = data2buf(data);
	assert(buf->b_use > 0);
	bflush(buf);
	--buf->b_use;
}

void *bget (ki_t key, u64 blkno)
{
	fsmsg_s	m;
	buf_s	*buf;
	int	rc;

	if (!blkno) return NULL;

	buf = lookup(key, blkno);
	if (buf) {
		return buf->b_data;
	}
	buf = victim();
	buf->b_key = key;
	buf->b_blkno = blkno;
	add(buf);

	m.m_method = READ_BAG;
	m.io_blkno = blkno;
	rc = getdata_tau(key, &m, BLK_SIZE, buf->b_data);
	if (rc) {
		eprintf("getdata_tau %ld at %lld failed =%d",
			key, blkno, rc);
		return NULL;
	}
	buf->b_dirty = FALSE;
	return buf->b_data;
}

void *bnew (ki_t key, u64 blkno)
{
	buf_s	*buf;

	//if (!blkno) return NULL;

	buf = lookup(key, blkno);
	if (!buf) {
		buf = victim();
		buf->b_key = key;
		buf->b_blkno = blkno;
		add(buf);
	}
	memset(buf->b_data, 0, BLK_SIZE);
	buf->b_dirty = TRUE;
	return buf->b_data;
}
