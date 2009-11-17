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

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "q_offset.h"

	/*
	 * Stack
	 */
void errorStk (Stk_q *stk, void *obj, qlink_t *link)
{
	printf("Stack Error: stk=%p obj=%p link=%p\n",
			stk, obj, link);
}

void initStk (Stk_q *stk, unint offset)
{
	stk->top    = 0;
	stk->offset = offset;
}

void pushStk (Stk_q *stk, void *obj)
{
	qlink_t	*link = (qlink_t *)((addr)obj + stk->offset);

	if (link->next) {
		errorStk(stk, obj, link->next);
		return;
	}
	link->next = stk->top;
	stk->top = link;
}

void *popStk (Stk_q *stk)
{
	qlink_t	*link;
	
	link = stk->top;
	if (!link) return 0;

	stk->top   = link->next;
	link->next = 0;
	return (void *)((addr)link - stk->offset);
}

void *peekStk (Stk_q *stk)
{
	qlink_t	*link;
	
	link = stk->top;
	if (!link) return 0;

	return (void *)((addr)link - stk->offset);
}

unint cntStk (Stk_q *stk)
{
	unint	cnt = 0;
	qlink_t	*link;
		
	for (link = stk->top; link; link = link->next) {
		++cnt;
	}
	return cnt;
}

void *rmvStk (Stk_q *stk, void *obj)
{
	qlink_t	*link = (qlink_t *)((addr)obj + stk->offset);
	qlink_t	*current;
	qlink_t	*prev;

	prev = (qlink_t *)&stk->top;
	for (current = prev->next; current; prev = current, current = current->next) {
		if (current == link) {
			prev->next = link->next;
			link->next = 0;
			return obj;
		}
	}
	return 0;
}

void appendStk (Stk_q *dst, Stk_q *src)
{
	qlink_t	*link;

	if (!src->top) return;

	for (link = (qlink_t *)&dst->top; link->next; link = link->next)
		;
	link->next = src->top;
	src->top = 0;
}

bool foreachStk (Stk_q *stk, qfunc f, void *args)
{
	qlink_t	*link;
		
	for (link = stk->top; link; link = link->next) {
		if (!f((void *)((addr)link - stk->offset), args)) {
			return FALSE;
		}
	}
	return TRUE;
}

	/*
	 * Ring buffer
	 */
unint prRing(Ring_q *ring)
{
	void	**slot;

	printf("\tring=%p enq=%p deq=%p start=%p end=%p cnt=%ld\n",
		ring, ring->enq, ring->deq, ring->start, ring->end,
		cntRing(ring));
	for (slot = ring->deq; slot != ring->enq; ) {
		printf("\t\t%p:%p\n", slot, *slot);
		if (++slot == ring->end) slot = ring->start;
	}
	return TRUE;
}

void initRing (Ring_q *ring, unint numObjs, void **ringBuffer)
{
	ring->enq = ring->deq = ring->start = ringBuffer;
	ring->end = ring->start + numObjs;
}

Ring_q *newRing (Ring_q *ring, unint numObjs)
{
	ring->start = malloc(numObjs * sizeof(void *));
	if (ring->start == 0) return 0;

	ring->enq = ring->deq = ring->start;
	ring->end = ring->start + numObjs;
	return ring;
}

bool enqRing (Ring_q *ring, void *obj)
{
	void	**next;

	next = ring->enq + 1;
	if (next == ring->end) next = ring->start;
	if (next == ring->deq) return FALSE;

	*ring->enq = obj;
	ring->enq = next;
	return TRUE;
}

bool enqFrontRing (Ring_q *ring, void *obj)
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

void *deqRing (Ring_q *ring)
{
	void	*obj;

	if (isEmptyRing(ring)) return 0;

	obj = *ring->deq++;
	if (ring->deq == ring->end) ring->deq = ring->start;

	return obj;
}

void *deqBackRing (Ring_q *ring)
{
	void	*obj;

	if (isEmptyRing(ring)) return 0;

	if (ring->enq == ring->start) ring->enq = ring->end;
	obj = *--ring->enq;

	return obj;
}

void *peekRing (Ring_q *ring)
{
	if (isEmptyRing(ring)) return 0;

	return *ring->deq;
}

unint cntRing (Ring_q *ring)
{
	if (ring->enq >= ring->deq) return ring->enq - ring->deq;
	else return (ring->end - ring->start) - (ring->deq - ring->enq);
}

void *rmvRing (Ring_q *ring, void *obj)
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

bool appendRing (Ring_q *dstq, Ring_q *srcq)
{
	unint	max;
	void	*obj;

	max = dstq->end - dstq->start - 1;

	if (max < cntRing(dstq) + cntRing(srcq)) return FALSE;

	for (;;) {
		obj = deqRing(srcq);
		if (obj == 0) break;
		enqRing(dstq, obj);
	}
	return TRUE;
}

bool foreachRing (Ring_q *ring, qfunc f, void *args)
{
	void	**slot;

	for (slot = ring->deq; slot != ring->enq;) {
		if (!f( *slot, args)) return FALSE;
		if (++slot == ring->end) slot = ring->start;
	}
	return TRUE;
}


	/*
	 * Circularly Linked Queue
	 */
void errorCir (Cir_q *cir, void *obj, qlink_t *next)
{
	printf("Cir Error: cir=%p obj=%p link->next=%p\n",
			cir, obj, next);
}

void initCir (Cir_q *cir, unint offset)
{
	cir->last = NULL;
	cir->offset = offset;
}

void enqCir (Cir_q *cir, void *obj)
{
	qlink_t	*link = (qlink_t *)((addr)obj + cir->offset);

	if (link->next) {
		errorCir(cir, obj, link->next);
		return;
	}
	if (isEmptyCir(cir)) {
		link->next = link;
	} else {
		link->next = cir->last->next;
		cir->last->next = link;
	}
	cir->last = link;
}

void enqFrontCir (Cir_q *cir, void *obj)
{
	qlink_t	*link = (qlink_t *)((addr)obj + cir->offset);

	if (link->next) {
		errorCir(cir, obj, link->next);
		return;
	}
	if (isEmptyCir(cir)) {
		link->next = link;
		cir->last = link;
	} else {
		link->next = cir->last->next;
		cir->last->next = link;
	}
}

void *deqCir (Cir_q *cir)
{
	qlink_t	*link;

	if (isEmptyCir(cir)) return 0;

	link = cir->last->next;
	if (link == cir->last) {
		cir->last = 0;
	} else {
		cir->last->next = link->next;
	}
	link->next = 0;
	return (void *)((addr)link - cir->offset);
}

void *peekCir (Cir_q *cir)
{
	qlink_t	*link;

	if (isEmptyCir(cir)) return 0;

	link = cir->last->next;
	return (void *)((addr)link - cir->offset);
}

unint cntCir (Cir_q *cir)
{
	unint	cnt;
	qlink_t	*link;

	if (isEmptyCir(cir)) return 0;

	link = cir->last->next;
	for (cnt = 1; link != cir->last; ++cnt) {
		link = link->next;
	}
	return cnt;
}

void *rmvCir (Cir_q *cir, void *obj)
{
	qlink_t	*link = (qlink_t *)((addr)obj + cir->offset);
	qlink_t	*current;
	qlink_t	*prev;

	if (link->next == 0) return obj;
	if (isEmptyCir(cir)) return 0;

	prev = cir->last;
	for (current = prev->next;
			current != cir->last;
			prev = current, current = current->next)
	{
		if (link == current) {
			prev->next = link->next;
			link->next = 0;
			return obj;
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
		return obj;
	}
	return 0;
}

void appendCir (Cir_q *dstq, Cir_q *srcq)
{
	qlink_t	*link;

	if (isEmptyCir(srcq)) return;
	if (!isEmptyCir(dstq)) {
		link = srcq->last->next;
		srcq->last->next = dstq->last->next;
		dstq->last->next = link;
	}
	dstq->last = srcq->last;
	srcq->last = 0;
}

bool foreachCir (Cir_q *cir, qfunc f, void *args)
{
	qlink_t	*link;
		
	if (isEmptyCir(cir)) return TRUE;

	for (link = cir->last->next;; link = link->next) {
		if (!f((void *)((addr)link - cir->offset), args)) {
			return FALSE;
		}
		if (link == cir->last) {
			return TRUE;
		}
	}
	return TRUE;
}

	/*
	 * Doublely Linked Queue
	 */
void errorDq (D_q *dq, void *obj, dqlink_t *link)
{
	printf("Dq Error: dq=%p obj=%p link=%p\n",
			dq, obj, link);
}

void initDq (D_q *dq, unint offset)
{
	dq->head.next = dq->head.prev = &dq->head;
	dq->offset = offset;
}

void enqDq (D_q *dq, void *obj)
{
	dqlink_t	*link = (dqlink_t *)((addr)obj + dq->offset);

	if (link->next) {
		errorDq(dq, obj, link->next);
		return;
	}
	link->prev = dq->head.prev;
	link->next = &dq->head;
	link->prev->next = link;
	dq->head.prev    = link;
}

void enqFrontDq (D_q *dq, void *obj)
{
	dqlink_t	*link = (dqlink_t *)((addr)obj + dq->offset);

	if (link->next) {
		errorDq(dq, obj, link->next);
		return;
	}
	link->prev = &dq->head;
	link->next = dq->head.next;
	link->next->prev = link;
	dq->head.next    = link;
}

void *deqDq (D_q *dq)
{
	dqlink_t	*link;

	if (dq->head.next == &dq->head) return 0;

	link                = dq->head.next;
	dq->head.next       = link->next;
	dq->head.next->prev = &dq->head;
	link->next          = 0;
	return (void *)((addr)link - dq->offset);
}

void *deqBackDq (D_q *dq)
{
	dqlink_t	*link;

	if (dq->head.next == &dq->head) return 0;

	link                = dq->head.prev;
	dq->head.prev       = link->prev;
	dq->head.prev->next = &dq->head;
	link->next          = 0;
	return (void *)((addr)link - dq->offset);
}

void *peekDq (D_q *dq)
{
	dqlink_t	*link;

	if (dq->head.next == &dq->head) return 0;

	link = dq->head.next;
	return (void *)((addr)link - dq->offset);
}

unint cntDq (D_q *dq)
{
	unint		cnt = 0;
	dqlink_t	*link;
		
	for (link = dq->head.next; link != &dq->head; link = link->next) {
		++cnt;
	}
	return cnt;
}

void rmvDq (dqlink_t *link)
{
	if (link->next) {
		link->prev->next = link->next;
		link->next->prev = link->prev;
		link->next = 0;
	}
}

void appendDq (D_q *dstq, D_q *srcq)
{
	if (isEmptyDq(srcq)) return;

	srcq->head.prev->next = &dstq->head;
	srcq->head.next->prev = dstq->head.prev;
	dstq->head.prev->next = srcq->head.next;
	dstq->head.prev = srcq->head.prev;
	srcq->head.next = srcq->head.prev = &srcq->head;
}

bool foreachDq (D_q *dq, qfunc f, void *args)
{
	dqlink_t	*link;
		
	for (link = dq->head.next; link != &dq->head; link = link->next) {
		if (!f((void *)((addr)link - dq->offset), args)) {
			return FALSE;
		}
	}
	return TRUE;
}

#if 0

/*
 * Tests for queuing routines
 */

#undef assert
#define assert(_expr)	((void)((_expr) || Assert(WHERE " (" # _expr ")")))

enum { NUM_OBJS = 10 };

bool Assert (char *s)
{
	printf("ASSERT:%s\n", s);
	exit(1);
	return TRUE;
}

bool pick (unint percent)
{
	return (random() % 100) < percent;
}

unint range (unint limit)
{
	return random() % limit;
}

bool fcount (void *obj, unint *cnt)
{
	++(*cnt);
	return 0;
}

typedef struct StkObj_s
{
	int		a;
	int		b;
	qlink_t	link;
} StkObj_s;

StkObj_s	StkObj[NUM_OBJS];
Stk_q	Astack;
Stk_q	Bstack;

typedef struct StkTest_s
{
	unint	pushes;
	unint	pops;
	unint	peeks;
	unint	rmvs;
	unint	cnts;
	unint	appends;
	unint	foreaches;
} StkTest_s;

void reportStk (StkTest_s *t)
{
	printf("Stk pushes=%d pops=%d peeks=%d rmvs=%d "
			"cnt=%d appends=%d foreaches=%d\n",
			*t);
}

void checkObjs (char *s)
{
#if 0
	unint i;

	for (i = 0; i < NUM_OBJS; ++i) {
		if (StkObj[i].link.next==0) {
			printf("%s NULL i=%d\n", s, i);
			exit(1);
		}
	}
#endif
}

void testStk (unint numTests)
{
	unint		i;
	StkObj_s	*obj;
	StkObj_s	*peeked;
	StkTest_s	t;
	
	bzero( &t, sizeof(StkTest_s));
	
	initStk( &Astack, offsetof(StkObj_s, link));
	initStk( &Bstack, offsetof(StkObj_s, link));
	for (i = 0; i < NUM_OBJS; ++i) {
		initLink(StkObj[i].link);
		pushStk( &Astack, &StkObj[i]); ++t.pushes;
	}
	for (i = 0; i < numTests; ++i) {
		if (pick(20)) {
			obj = popStk( &Astack);
			if (obj) {
				++t.pops;
				pushStk( &Bstack, obj); ++t.pushes;
			}
		} else if (pick(20)) {
			obj = popStk( &Bstack);
			if (obj) {
				++t.pops;
				pushStk( &Astack, obj); ++t.pushes;
			}
		} else if (pick(20)) {
			peeked = peekStk( &Bstack);
			obj = popStk( &Bstack);
			assert(peeked == obj);
			++t.peeks;
			if (obj) {
				++t.pops;
				pushStk( &Astack, obj); ++t.pushes;
			}
		} else if (pick(20)) {
			appendStk( &Astack, &Bstack); ++t.appends;
			assert(isEmptyStk( &Bstack));
			assert(cntStk( &Astack) == NUM_OBJS);
		}
		else if (pick(20)) {
			appendStk( &Bstack, &Astack); ++t.appends;
		} else if (pick(20)) {
			unint	i = range(NUM_OBJS);

			if (rmvStk( &Astack, &StkObj[i])) {
				++t.rmvs;
			} else if (rmvStk( &Bstack, &StkObj[i])) {
				++t.rmvs;
			} else {
				printf("Cound't remove\n");
			}
			pushStk( &Astack, &StkObj[i]); ++t.pushes;
		} else if (pick(20)) {
			unint	cnt = 0;

			foreachStk( &Astack, fcount, &cnt);
			if (cnt != cntStk( &Astack)) {
				printf("foreach cout %d != %d\n", cnt, cntStk( &Astack));
			}
			++t.foreaches;
		}
		if (cntStk( &Astack) + cntStk( &Bstack) != NUM_OBJS) {
			printf("%d + %d != %d\n",
				cntStk( &Astack), cntStk( &Bstack), NUM_OBJS);
		}
		++t.cnts;
	}
	reportStk( &t);
}

/*====================================*/

typedef struct RingObj_s {
	int		a;
	int		b;
} RingObj_s;

RingObj_s	RingObj[NUM_OBJS];
Ring_q	Aring;
Ring_q	Bring;

typedef struct RingTest_s {
	unint	enqs;
	unint	fronts;
	unint	deqs;
	unint	backs;
	unint	peeks;
	unint	rmvs;
	unint	cnts;
	unint	appends;
	unint	foreaches;
} RingTest_s;

void reportRing (RingTest_s *t)
{
	printf("Ring enqs=%d fronts=%d deqs=%d backs=%d "
			"peeks=%d rmvs=%d cnt=%d appends=%d foreaches=%d\n",
			*t);
}

void testRing (unint numTests)
{
	unint		i;
	RingObj_s	*obj;
	RingObj_s	*peeked;
	RingTest_s	t;
	
	bzero( &t, sizeof(RingTest_s));
	
	newRing( &Aring, NUM_OBJS + 1);
	newRing( &Bring, NUM_OBJS + 1);
	for (i = 0; i < NUM_OBJS; ++i) {
		enqRing( &Aring, &RingObj[i]); ++t.enqs;
	}
	for (i = 0; i < numTests; ++i) {
		if (pick(20)) {
			obj = deqRing( &Aring);
			if (obj) {
				++t.deqs;
				if (pick(50)) {
					enqRing( &Bring, obj); ++t.enqs;
				} else {
					enqFrontRing( &Bring, obj); ++t.fronts;
				}
			}
		} else if (pick(20)) {
			if (pick(50)) {
				obj = deqRing( &Bring);
				if (obj) ++t.deqs;
			} else {
				obj = deqBackRing( &Bring);
				if (obj) ++t.backs;
			}
			if (obj) {
				enqRing( &Aring, obj); ++t.enqs;
			}
		} else if (pick(20)) {
			peeked = peekRing( &Bring);
			obj = deqRing( &Bring);
			assert(peeked == obj);
			++t.peeks;
			if (obj) {
				++t.deqs;
				enqRing( &Aring, obj); ++t.enqs;
			}
		} else if (pick(20)) {
			appendRing( &Aring, &Bring); ++t.appends;
			assert(isEmptyRing( &Bring));
			assert(cntRing( &Aring) == NUM_OBJS);
		} else if (pick(20)) {
			appendRing( &Bring, &Aring); ++t.appends;
		} else if (pick(20)) {
			unint	i = range(NUM_OBJS);

			if (rmvRing( &Aring, &RingObj[i])) {
				++t.rmvs;
			} else if (rmvRing( &Bring, &RingObj[i])) {
				++t.rmvs;
			} else {
				printf("Cound't remove\n");
				exit(1);
			}
			enqRing( &Aring, &RingObj[i]); ++t.enqs;
		} else if (pick(20)) {
			unint	cnt = 0;

			foreachRing( &Aring, fcount, &cnt);
			if (cnt != cntRing( &Aring)) {
				printf("foreach cout %d != %d\n", cnt, cntRing( &Aring));
			}
			++t.foreaches;
		}
		if (cntRing( &Aring) + cntRing( &Bring) != NUM_OBJS) {
			printf("%d + %d != %d\n",
				cntRing( &Aring), cntRing( &Bring), NUM_OBJS);
			exit(1);
		}
		++t.cnts;
	}
	reportRing( &t);
}
/*================================================*/

typedef struct CirObj_s {
	int		a;
	int		b;
	qlink_t	link;
} CirObj_s;

CirObj_s	CirObj[NUM_OBJS];
Cir_q	Acir;
Cir_q	Bcir;

typedef struct CirTest_s {
	unint	enqs;
	unint	enqsFront;
	unint	deqs;
	unint	peeks;
	unint	rmvs;
	unint	cnts;
	unint	appends;
	unint	foreaches;
} CirTest_s;

void reportCir (CirTest_s *t)
{
	printf("Cir enqs=%d enqsFront=%d deqs=%d peeks=%d "
			"rmvs=%d cnt=%d appends=%d foreaches=%d\n",
			*t);
}

void testCir (unint numTests)
{
	unint		i;
	CirObj_s	*obj;
	CirObj_s	*peeked;
	CirTest_s	t;
	
	bzero( &t, sizeof(CirTest_s));
	
	initCir( &Acir, offsetof(CirObj_s, link));
	initCir( &Bcir, offsetof(CirObj_s, link));
	for (i = 0; i < NUM_OBJS; ++i) {
		initLink(CirObj[i].link);
		enqCir( &Acir, &CirObj[i]); ++t.enqs;
	}
	for (i = 0; i < numTests; ++i) {
		if (pick(20)) {
			obj = deqCir( &Acir);
			if (obj) {
				++t.deqs;
				enqCir( &Bcir, obj); ++t.enqs;
			}
		} else if (pick(20)) {
			obj = deqCir( &Bcir);
			if (obj) {
				++t.deqs;
				if (pick(50)) {
					enqCir( &Acir, obj); ++t.enqs;
				} else {
					enqFrontCir( &Acir, obj); ++t.enqsFront;
				}
			}
		} else if (pick(20)) {
			peeked = peekCir( &Bcir);
			obj = deqCir( &Bcir);
			assert(peeked == obj);
			++t.peeks;
			if (obj) {
				++t.deqs;
				enqCir( &Acir, obj); ++t.enqs;
			}
		} else if (pick(20)) {
			appendCir( &Acir, &Bcir); ++t.appends;
			assert(isEmptyCir( &Bcir));
			assert(cntCir( &Acir) == NUM_OBJS);
		} else if (pick(20)) {
			appendCir( &Bcir, &Acir); ++t.appends;
		} else if (pick(20)) {
			unint	i = range(NUM_OBJS);

			if (rmvCir( &Acir, &CirObj[i])) {
				++t.rmvs;
			} else if (rmvCir( &Bcir, &CirObj[i])) {
				++t.rmvs;
			} else {
				printf("Cound't remove\n");
			}
			enqCir( &Acir, &CirObj[i]); ++t.enqs;
		} else if (pick(20)) {
			unint	cnt = 0;

			foreachCir( &Acir, fcount, &cnt);
			if (cnt != cntCir( &Acir)) {
				printf("foreach cout %d != %d\n", cnt, cntCir( &Acir));
			}
			++t.foreaches;
		}
		if (cntCir( &Acir) + cntCir( &Bcir) != NUM_OBJS) {
			printf("%d + %d != %d\n",
				cntCir( &Acir), cntCir( &Bcir), NUM_OBJS);
		}
		++t.cnts;
	}
	reportCir( &t);
}
/*======================================================================*/

typedef struct DqObj_s {
	int			a;
	int			b;
	dqlink_t	link;
} DqObj_s;

DqObj_s	DqObj[NUM_OBJS];
D_q	Adq;
D_q	Bdq;

typedef struct DqTest_s {
	unint	enqs;
	unint	fronts;
	unint	deqs;
	unint	backs;
	unint	peeks;
	unint	rmvs;
	unint	cnts;
	unint	appends;
	unint	foreaches;
} DqTest_s;

void reportDq (DqTest_s *t)
{
	printf("Dq enqs=%d fronts=%d deqs=%d backs=%d "
			"peeks=%d rmvs=%d cnt=%d appends=%d foreaches=%d\n",
			*t);
}

void testDq (unint numTests)
{
	unint		i;
	DqObj_s		*obj;
	DqObj_s		*peeked;
	DqTest_s	t;
	
	bzero( &t, sizeof(DqTest_s));
	
	initDq( &Adq, offsetof(DqObj_s, link));
	initDq( &Bdq, offsetof(DqObj_s, link));
	for (i = 0; i < NUM_OBJS; ++i) {
		initLink(DqObj[i].link);
		enqDq( &Adq, &DqObj[i]); ++t.enqs;
	}
	for (i = 0; i < numTests; ++i) {
		if (pick(20)) {
			obj = deqDq( &Adq);
			if (obj) {
				++t.deqs;
				if (pick(50)) {
					enqDq( &Bdq, obj); ++t.enqs;
				} else {
					enqFrontDq( &Bdq, obj); ++t.fronts;
				}
			}
		} else if (pick(20)) {
			if (pick(50)) {
				obj = deqDq( &Bdq);
				if (obj) ++t.deqs;
			} else {
				obj = deqBackDq( &Bdq);
				if (obj) ++t.backs;
			}
			if (obj) {
				enqDq( &Adq, obj); ++t.enqs;
			}
		} else if (pick(20)) {
			peeked = peekDq( &Bdq);
			obj = deqDq( &Bdq);
			assert(peeked == obj);
			++t.peeks;
			if (obj) {
				++t.deqs;
				enqDq( &Adq, obj); ++t.enqs;
			}
		} else if (pick(20)) {
			appendDq( &Adq, &Bdq); ++t.appends;
			assert(isEmptyDq( &Bdq));
			assert(cntDq( &Adq) == NUM_OBJS);
		} else if (pick(20)) {
			appendDq( &Bdq, &Adq); ++t.appends;
		} else if (pick(20)) {
			unint	i = range(NUM_OBJS);

			rmvDq( &DqObj[i].link); ++t.rmvs;
			enqDq( &Adq, &DqObj[i]); ++t.enqs;
		} else if (pick(20)) {
			unint	cnt = 0;

			foreachDq( &Adq, fcount, &cnt);
			if (cnt != cntDq( &Adq)) {
				printf("foreach cout %d != %d\n", cnt, cntDq( &Adq));
			}
			++t.foreaches;
		}
		if (cntDq( &Adq) + cntDq( &Bdq) != NUM_OBJS) {
			printf("%d + %d != %d\n",
				cntDq( &Adq), cntDq( &Bdq), NUM_OBJS);
		}
		++t.cnts;
	}
	reportDq( &t);
}

int main (int argc, char *argv[])
{
	unint		numTests = 100000;
	
	if (argc > 1) numTests = atoi(argv[1]);

	testStk(numTests);
	testStk(numTests);
	testRing(numTests);
	testRing(numTests);
	testCir(numTests);
	testCir(numTests);
	testDq(numTests);
	testDq(numTests);
	
	return 0;
}

#endif

