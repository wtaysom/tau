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

/*
 *	_stk	- Linked list stack
 *	_ring	- Fixed size ring buffer
 *	_cir	- Singlely linked circular queue
 *	_dq	- Doublely linked circular queue
 *
 * Note:
 *	1. the _* routines don't check for the empty case because
 *		the macros have to.
 */
#ifndef _Q_H_
#define _Q_H_

#ifdef __KERNEL__
	int      tau_assert(const char *func, const char *what);
	#define qassert(_expr)		\
		((void)((_expr) ||	\
			tau_assert(__func__, "Q: " WHERE " (" # _expr ")")))
#else
	#ifndef assert
	#include <assert.h>
	#endif
	#define qassert	assert
#endif

#ifndef _STYLE_H_
#include <style.h>
#endif

typedef struct qlink_t {
	struct qlink_t	*next;
} qlink_t;

typedef struct dqlink_t {
	struct dqlink_t	*next;
	struct dqlink_t	*prev;
} dqlink_t;

typedef struct stk_q {
	qlink_t	*top;
} stk_q;

typedef struct ring_q {
	void	**enq;
	void	**deq;
	void	**start;
	void	**end;
} ring_q;

typedef struct cir_q {
	qlink_t	*last;
} cir_q;

typedef struct dqlink_t	d_q;

typedef addr (*qfunc)(void *obj, void *data);

#define is_qmember(_x)		((_x).next)
#define init_qlink(_x)		((_x).next = 0)

/*
 * Stack
 */
#define is_empty_stk(_stk)	((_stk)->top == 0)

static inline void init_stk (stk_q *stk)
{
	stk->top = 0;
}

static inline void _push_stk (stk_q *stk, qlink_t *link)
{
	qassert(!link->next);
	link->next = stk->top;
	stk->top = link;
}

static inline addr _pop_stk (stk_q *stk)
{
	qlink_t	*link;
	
	link = stk->top;
	stk->top   = link->next;
	link->next = 0;
	return (addr)link;
}

static inline addr _peek_stk (stk_q *stk)
{
	return (addr)stk->top;
}

bool _rmv_stk (stk_q *stk, qlink_t *link);
void append_stk(stk_q *dst, stk_q *src);
unint cnt_stk(stk_q *stk);
addr foreach_stk(stk_q *stk, qfunc f, void *args);
#define push_stk(_s, _x, _l)	_push_stk(_s, &(_x)->_l)
#define pop_stk(_s, _x, _l)	(_x = (is_empty_stk(_s) ? 0	\
				    : (void *)(_pop_stk(_s)-field(_x, _l))))
#define peek_stk(_s, _x, _l)	(_x = (is_empty_stk(_s) ? 0	\
				    : (void *)(_peek_stk(_s)-field(_x, _l))))
#define rmv_stk(_s, _x, _l)	_rmv_stk(_s, &(_x)->_l)

/*
 * Ring buffer
 */
#define is_empty_ring(_ring)	((_ring)->enq == (_ring)->deq)	
void init_ring(ring_q *ring, unint numObjs, void **ringBuffer);
ring_q *new_ring(ring_q *ring, unint numObjs);
bool enq_ring(ring_q *ring, void *obj);
bool enq_front_ring(ring_q *ring, void *obj);
void *deq_ring(ring_q *ring);
void *deq_back_ring(ring_q *ring);
void *peek_ring(ring_q *ring);
void *rmv_ring(ring_q *ring, void *obj);
unint cnt_ring(ring_q *ring);
bool append_ring(ring_q *dstq, ring_q *srcq);
addr foreach_ring(ring_q *ring, qfunc f, void *args);

/*
 * Circular singlely linked list
 */
#define is_empty_cir(_cir)	((_cir)->last == 0)	

static inline void init_cir (cir_q *cir)
{
	cir->last = NULL;
}

static inline void _enq_cir (cir_q *cir, qlink_t *link)
{
	qassert(!link->next);
	if (is_empty_cir(cir)) {
		link->next = link;
	} else {
		link->next = cir->last->next;
		cir->last->next = link;
	}
	cir->last = link;
}

static inline void _enq_front_cir (cir_q *cir, qlink_t *link)
{
	qassert(!link->next);
	if (is_empty_cir(cir)) {
		link->next = link;
		cir->last = link;
	} else {
		link->next = cir->last->next;
		cir->last->next = link;
	}
}

static inline addr _deq_cir (cir_q *cir)
{
	qlink_t	*link;

	link = cir->last->next;
	if (link == cir->last) {
		cir->last = 0;
	} else {
		cir->last->next = link->next;
	}
	link->next = 0;
	return (addr)link;
}

static inline addr _peek_cir (cir_q *cir)
{
	qlink_t	*link;

	link = cir->last->next;
	return (addr)link;
}

void init_cir(cir_q *cir);
bool _rmv_cir(cir_q *cir, qlink_t *link);
void append_cir(cir_q *dstq, cir_q *srcq);
unint cnt_cir(cir_q *cir);
addr foreach_cir(cir_q *cir, qfunc f, void *args);
#define enq_cir(_q, _x, _l)	_enq_cir(_q, &(_x)->_l)
#define enq_front_cir(_q, _x, _l) _enq_front_cir(_q, &(_x)->_l)
#define deq_cir(_q, _x, _l)	(_x = (is_empty_cir(_q) ? 0	\
				    : (void *)(_deq_cir(_q)-field(_x, _l))))
#define peek_cir(_q, _x, _l)	(_x = (is_empty_cir(_q) ? 0	\
				    : (void *)(_peek_cir(_q)-field(_x, _l))))
#define rmv_cir(_q, _x, _l)	_rmv_cir(_q, &(_x)->_l)

/*
 * Doublely linked list
 */
#define is_empty_dq(_dq)	((_dq)->next == (_dq))	
#define static_init_dq(_dq)	{ &(_dq), &(_dq) }

static inline void init_dq (d_q *dq)
{
	dq->next = dq->prev = dq;
}

static inline void _enq_dq (d_q *dq, dqlink_t *link)
{
	qassert(!link->next);
	qassert(dq->next->prev==dq);
	qassert(dq->prev->next==dq);
	link->prev       = dq->prev;
	link->next       = dq;
	link->prev->next = link;
	dq->prev         = link;
}

static inline void _enq_front_dq (d_q *dq, dqlink_t *link)
{
	qassert(!link->next);
	qassert(dq->next->prev==dq);
	qassert(dq->prev->next==dq);
	link->prev       = dq;
	link->next       = dq->next;
	link->next->prev = link;
	dq->next         = link;
}

static inline addr _deq_dq (d_q *dq)
{
	dqlink_t	*link;

	qassert(!is_empty_dq(dq));
	link = dq->next;
	qassert(link->next->prev==link);
	qassert(link->prev->next==link);
	dq->next       = link->next;
	dq->next->prev = dq;
	link->next     = 0;
	return (addr)link;
}

static inline addr _deq_back_dq (d_q *dq)
{
	dqlink_t	*link;

	qassert(!is_empty_dq(dq));
	link = dq->prev;
	qassert(link->next->prev==link);
	qassert(link->prev->next==link);
	dq->prev       = link->prev;
	dq->prev->next = dq;
	link->next     = 0;
	return (addr)link;
}

static inline addr _peek_dq (d_q *dq)
{
	qassert(!is_empty_dq(dq));
	return (addr)dq->next;
}

static inline void rmv_dq (dqlink_t *link)
{
	if (link->next) {
		qassert(link->next->prev==link);
		qassert(link->prev->next==link);
		link->prev->next = link->next;
		link->next->prev = link->prev;
		link->next = 0;
	}
}

static inline void *_next_dq (void *x, d_q *dq, int offset)
{
	dqlink_t	*link;
	dqlink_t	*next;

	if (x) {
		link = (dqlink_t *)((addr)x + offset);
		next = link->next;
	} else {
		next = dq->next;
	}
	if (next == dq) return NULL;
	return (void *)((addr)next - offset);
}

void append_dq(d_q *dstq, d_q *srcq);
unint cnt_dq(d_q *dq);
addr foreach_dq(d_q *dq, qfunc f, void *args);
#define enq_dq(_q, _x, _l)	_enq_dq(_q, &(_x)->_l)
#define enq_front_dq(_q, _x, _l) _enq_front_dq(_q, &(_x)->_l)
#define deq_dq(_q, _x, _l)	(_x = (is_empty_dq(_q) ? 0	\
				    : (void *)(_deq_dq(_q)-field(_x, _l))))
#define deq_back_dq(_q, _x, _l)	(_x = (is_empty_dq(_q) ? 0	\
				    : (void *)(_deq_back_dq(_q)-field(_x, _l))))
#define peek_dq(_q, _x, _l)	(_x = (is_empty_dq(_q) ? 0	\
				    : (void *)(_peek_dq(_q)-field(_x, _l))))		
#define next_dq(_x, _q, _l)	(_x = _next_dq(_x, _q, field(_x, _l)))

#endif
