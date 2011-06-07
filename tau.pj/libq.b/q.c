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

#include "q.h"

/*
 * Stack
 */
bool _rmv_stk (stk_q *stk, qlink_t *link)
{
	qlink_t	*next;
	qlink_t	*prev;

	prev = (qlink_t *)&stk->top;
	for (next = prev->next; next; prev = next, next = next->next) {
		if (next == link) {
			prev->next = link->next;
			link->next = 0;
			return TRUE;
		}
	}
	return FALSE;
}

unint cnt_stk (stk_q *stk)
{
	unint	cnt = 0;
	qlink_t	*link;

	for (link = stk->top; link; link = link->next) {
		++cnt;
	}
	return cnt;
}

void append_stk (stk_q *dst, stk_q *src)
{
	qlink_t	*link;

	if (!src->top) return;

	for (link = (qlink_t *)&dst->top; link->next; link = link->next)
		;
	link->next = src->top;
	src->top = 0;
}

addr foreach_stk (stk_q *stk, qfunc f, void *args)
{
	qlink_t	*link;
	qlink_t	*next;
	addr	rc;

	for (link = stk->top; link; link = next) {
		next = link->next;
		rc = f(link, args);
		if (rc) return rc;
	}
	return 0;
}

/*
 * Ring buffer
 */
void init_ring (ring_q *ring, unint numObjs, void **ringBuffer)
{
	ring->enq = ring->deq = ring->start = ringBuffer;
	ring->end = ring->start + numObjs;
}

#ifndef __KERNEL__
#include <stdlib.h>

ring_q *new_ring (ring_q *ring, unint numObjs)
{
	ring->start = malloc(numObjs * sizeof(void *));
	if (ring->start == 0) return 0;

	ring->enq = ring->deq = ring->start;
	ring->end = ring->start + numObjs;
	return ring;
}
#endif

bool enq_ring (ring_q *ring, void *obj)
{
	void	**next;

	next = ring->enq + 1;
	if (next == ring->end) next = ring->start;
	if (next == ring->deq) return FALSE;

	*ring->enq = obj;
	ring->enq = next;
	return TRUE;
}

bool enq_front_ring (ring_q *ring, void *obj)
{
	void	**prev;

	prev = ring->deq;
	if (prev == ring->start) prev = ring->end;
	--prev;
	if (prev == ring->enq) return FALSE;

	*prev = obj;
	ring->deq = prev;
	return TRUE;
}

void *deq_ring (ring_q *ring)
{
	void	*obj;

	if (is_empty_ring(ring)) return 0;

	obj = *ring->deq++;
	if (ring->deq == ring->end) ring->deq = ring->start;

	return obj;
}

void *deq_back_ring (ring_q *ring)
{
	void	*obj;

	if (is_empty_ring(ring)) return 0;

	if (ring->enq == ring->start) ring->enq = ring->end;
	obj = *--ring->enq;

	return obj;
}

void *peek_ring (ring_q *ring)
{
	if (is_empty_ring(ring)) return 0;

	return *ring->deq;
}

void *rmv_ring (ring_q *ring, void *obj)
{
	void	**slot;
	void	**prev;

	for (slot = ring->deq; slot != ring->enq;) {
		if (*slot == obj) {
			if (slot == ring->start) prev = ring->end - 1;
			else prev = slot - 1;
			while (slot != ring->deq) {
				*slot = *prev;
				slot = prev;
				if (prev == ring->start) prev = ring->end - 1;
				else --prev;
			}
			if (++ring->deq == ring->end) ring->deq = ring->start;
			return obj;
		}
		if (++slot == ring->end) slot = ring->start;
	}
	return 0;
}

unint cnt_ring (ring_q *ring)
{
	if (ring->enq >= ring->deq) return ring->enq - ring->deq;
	else return (ring->end - ring->start) - (ring->deq - ring->enq);
}

bool append_ring (ring_q *dstq, ring_q *srcq)
{
	unint	max;
	void	*obj;

	max = dstq->end - dstq->start - 1;

	if (max < cnt_ring(dstq) + cnt_ring(srcq)) return FALSE;

	for (;;) {
		obj = deq_ring(srcq);
		if (obj == 0) break;
		enq_ring(dstq, obj);
	}
	return TRUE;
}

addr foreach_ring (ring_q *ring, qfunc f, void *args)
{
	void	**slot;
	addr	rc;

	for (slot = ring->deq; slot != ring->enq;) {
		rc = f(*slot, args);
		if (rc) return rc;
		if (++slot == ring->end) slot = ring->start;
	}
	return 0;
}


/*
 * Circularly Linked Queue
 */
bool _rmv_cir (cir_q *cir, qlink_t *link)
{
	qlink_t	*next;
	qlink_t	*prev;

	if (link->next == 0) return FALSE;
	if (is_empty_cir(cir)) return FALSE;

	prev = cir->last;
	for (next = prev->next; next != cir->last; prev = next, next = next->next)
	{
		if (link == next) {
			prev->next = link->next;
			link->next = 0;
			return TRUE;
		}
	}
	if (link == cir->last) {
		if (prev == link) {
			cir->last = 0;
		} else {
			prev->next = link->next;
			cir->last = prev;
		}
		link->next = 0;
		return TRUE;
	}
	return FALSE;
}

unint cnt_cir (cir_q *cir)
{
	unint	cnt;
	qlink_t	*link;

	if (is_empty_cir(cir)) return 0;

	link = cir->last->next;
	for (cnt = 1; link != cir->last; ++cnt) {
		link = link->next;
	}
	return cnt;
}

void append_cir (cir_q *dstq, cir_q *srcq)
{
	qlink_t	*link;

	if (is_empty_cir(srcq)) return;
	if (!is_empty_cir(dstq)) {
		link = srcq->last->next;
		srcq->last->next = dstq->last->next;
		dstq->last->next = link;
	}
	dstq->last = srcq->last;
	srcq->last = 0;
}

addr foreach_cir (cir_q *cir, qfunc f, void *args)
{
	qlink_t	*link;
	addr	rc;

	if (is_empty_cir(cir)) return 0;

	for (link = cir->last->next;; link = link->next) {
		rc = f(link, args);
		if (rc) return rc;

		if (link == cir->last) {
			return 0;
		}
	}
}

/*
 * Doublely Linked Queue
 */
unint cnt_dq (d_q *dq)
{
	unint		cnt = 0;
	dqlink_t	*link;

	for (link = dq->next; link != dq; link = link->next) {
		++cnt;
	}
	return cnt;
}

void append_dq (d_q *dstq, d_q *srcq)
{
	if (is_empty_dq(srcq)) return;

	srcq->prev->next = dstq;
	srcq->next->prev = dstq->prev;
	dstq->prev->next = srcq->next;
	dstq->prev = srcq->prev;
	srcq->next = srcq->prev = srcq;
}

addr foreach_dq (d_q *dq, qfunc f, void *args)
{
	dqlink_t	*link;
	dqlink_t	*next;
	addr		rc;

	for (link = dq->next; link != dq; link = next) {
		next = link->next;
		qassert(link->next->prev==link);
		qassert(link->prev->next==link);
		rc = f(link, args);
		if (rc) return rc;
	}
	return 0;
}
