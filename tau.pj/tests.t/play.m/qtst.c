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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <mylib.h>

#include "q.h"

/*
 * Tests for queuing routines
 */

#undef assert
#define assert(_expr)	((void)((_expr) || Assert(WHERE " (" # _expr ")")))

#define prp(_p)	printf(#_p "=%p\n", _p)

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

addr fcount (void *obj, void *pcount)
{
	unint	*cnt = pcount;
	++(*cnt);
	return 0;
}

typedef struct StkObj_s
{
	int	a;
	int	b;
	qlink_t	link;
} StkObj_s;

StkObj_s	StkObj[NUM_OBJS];
stk_q	Astack;
stk_q	Bstack;

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

void report_stk (StkTest_s *t)
{
	printf("stk pushes=%ld pops=%ld peeks=%ld rmvs=%ld "
		"cnt=%ld appends=%ld foreaches=%ld\n",
		t->pushes, t->pops, t->peeks, t->rmvs,
		t->cnts, t->appends, t->foreaches);
}

void checkObjs (char *s)
{
#if 0
	unint i;

	for (i = 0; i < NUM_OBJS; ++i) {
		if (StkObj[i].link.next==0) {
			printf("%s NULL i=%ld\n", s, i);
			exit(1);
		}
	}
#endif
}

void test_stk (unint numTests)
{
	unint		i;
	StkObj_s	*obj;
	StkObj_s	*peeked;
	StkTest_s	t;

	zero(t);

	init_stk( &Astack);
	init_stk( &Bstack);
	for (i = 0; i < NUM_OBJS; ++i) {
		init_qlink(StkObj[i].link);
		push_stk( &Astack, &StkObj[i], link); ++t.pushes;
	}
	for (i = 0; i < numTests; ++i) {
		if (pick(20)) {
			pop_stk( &Astack, obj, link);
			if (obj) {
				++t.pops;
				push_stk( &Bstack, obj, link); ++t.pushes;
			}
		} else if (pick(20)) {
			pop_stk( &Bstack, obj, link);
			if (obj) {
				++t.pops;
				push_stk( &Astack, obj, link); ++t.pushes;
			}
		} else if (pick(20)) {
			peek_stk( &Bstack, peeked, link);
			pop_stk( &Bstack, obj, link);
			assert(peeked == obj);
			++t.peeks;
			if (obj) {
				++t.pops;
				push_stk( &Astack, obj, link); ++t.pushes;
			}
		} else if (pick(20)) {
			append_stk( &Astack, &Bstack); ++t.appends;
			assert(is_empty_stk( &Bstack));
			assert(cnt_stk( &Astack) == NUM_OBJS);
		}
		else if (pick(20)) {
			append_stk( &Bstack, &Astack); ++t.appends;
		} else if (pick(20)) {
			unint	i = urand(NUM_OBJS);

			if (rmv_stk( &Astack, &StkObj[i], link)) {
				++t.rmvs;
			} else if (rmv_stk( &Bstack, &StkObj[i], link)) {
				++t.rmvs;
			} else {
				printf("Cound't remove\n");
			}
			push_stk( &Astack, &StkObj[i], link); ++t.pushes;
		} else if (pick(20)) {
			unint	cnt = 0;

			foreach_stk( &Astack, fcount, &cnt);
			if (cnt != cnt_stk( &Astack)) {
				printf("foreach cout %ld != %ld\n", cnt, cnt_stk( &Astack));
			}
			++t.foreaches;
		}
		if (cnt_stk( &Astack) + cnt_stk( &Bstack) != NUM_OBJS) {
			printf("%ld + %ld != %d\n",
				cnt_stk( &Astack), cnt_stk( &Bstack), NUM_OBJS);
		}
		++t.cnts;
	}
	report_stk( &t);
}

/*====================================*/

typedef struct _ringObj_s {
	int		a;
	int		b;
} _ringObj_s;

_ringObj_s	_ringObj[NUM_OBJS];
ring_q	Aring;
ring_q	Bring;

typedef struct _ringTest_s {
	unint	enqs;
	unint	fronts;
	unint	deqs;
	unint	backs;
	unint	peeks;
	unint	rmvs;
	unint	cnts;
	unint	appends;
	unint	foreaches;
} _ringTest_s;

void report_ring (_ringTest_s *t)
{
	printf("ring enqs=%ld fronts=%ld deqs=%ld backs=%ld peeks=%ld"
		" rmvs=%ld cnts=%ld appends=%ld foreaches=%ld\n",
		t->enqs, t->fronts, t->deqs, t->backs, t->peeks,
		t->rmvs, t->cnts, t->appends, t->foreaches);
}

void test_ring (unint numTests)
{
	unint		i;
	_ringObj_s	*obj;
	_ringObj_s	*peeked;
	_ringTest_s	t;

	zero(t);

	new_ring( &Aring, NUM_OBJS + 1);
	new_ring( &Bring, NUM_OBJS + 1);
	for (i = 0; i < NUM_OBJS; ++i) {
		enq_ring( &Aring, &_ringObj[i]); ++t.enqs;
	}
	for (i = 0; i < numTests; ++i) {
		if (pick(20)) {
			obj = deq_ring( &Aring);
			if (obj) {
				++t.deqs;
				if (pick(50)) {
					enq_ring( &Bring, obj); ++t.enqs;
				} else {
					enq_front_ring( &Bring, obj); ++t.fronts;
				}
			}
		} else if (pick(20)) {
			if (pick(50)) {
				obj = deq_ring( &Bring);
				if (obj) ++t.deqs;
			} else {
				obj = deq_back_ring( &Bring);
				if (obj) ++t.backs;
			}
			if (obj) {
				enq_ring( &Aring, obj); ++t.enqs;
			}
		} else if (pick(20)) {
			peeked = peek_ring( &Bring);
			obj = deq_ring( &Bring);
			assert(peeked == obj);
			++t.peeks;
			if (obj) {
				++t.deqs;
				enq_ring( &Aring, obj); ++t.enqs;
			}
		} else if (pick(20)) {
			append_ring( &Aring, &Bring); ++t.appends;
			assert(is_empty_ring( &Bring));
			assert(cnt_ring( &Aring) == NUM_OBJS);
		} else if (pick(20)) {
			append_ring( &Bring, &Aring); ++t.appends;
		} else if (pick(20)) {
			unint	i = urand(NUM_OBJS);

			if (rmv_ring( &Aring, &_ringObj[i])) {
				++t.rmvs;
			} else if (rmv_ring( &Bring, &_ringObj[i])) {
				++t.rmvs;
			} else {
				printf("Cound't remove\n");
				exit(1);
			}
			enq_ring( &Aring, &_ringObj[i]); ++t.enqs;
		} else if (pick(20)) {
			unint	cnt = 0;

			foreach_ring( &Aring, fcount, &cnt);
			if (cnt != cnt_ring( &Aring)) {
				printf("foreach cout %ld != %ld\n", cnt, cnt_ring( &Aring));
			}
			++t.foreaches;
		}
		if (cnt_ring( &Aring) + cnt_ring( &Bring) != NUM_OBJS) {
			printf("%ld + %ld != %d\n",
				cnt_ring( &Aring), cnt_ring( &Bring), NUM_OBJS);
			exit(1);
		}
		++t.cnts;
	}
	report_ring( &t);
}
/*================================================*/

typedef struct CirObj_s {
	int		a;
	int		b;
	qlink_t	link;
} CirObj_s;

CirObj_s	CirObj[NUM_OBJS];
cir_q	Acir;
cir_q	Bcir;

typedef struct CirTest_s {
	unint	enqs;
	unint	fronts;
	unint	deqs;
	unint	peeks;
	unint	rmvs;
	unint	cnts;
	unint	appends;
	unint	foreaches;
} CirTest_s;

void report_cir (CirTest_s *t)
{
	printf("cir enqs=%ld fronts=%ld deqs=%ld peeks=%ld"
		" rmvs=%ld cnt=%ld appends=%ld foreaches=%ld\n",
		t->enqs, t->fronts, t->deqs, t->peeks,
		t->rmvs, t->cnts, t->appends, t->foreaches);
}

void test_cir (unint numTests)
{
	unint		i;
	CirObj_s	*obj;
	CirObj_s	*peeked;
	CirTest_s	t;

	zero(t);

	init_cir( &Acir);
	init_cir( &Bcir);
	for (i = 0; i < NUM_OBJS; ++i) {
		init_qlink(CirObj[i].link);
		enq_cir( &Acir, &CirObj[i], link); ++t.enqs;
	}
	for (i = 0; i < numTests; ++i) {
		if (pick(20)) {
			deq_cir( &Acir, obj, link);
			if (obj) {
				++t.deqs;
				enq_cir( &Bcir, obj, link); ++t.enqs;
			}
		} else if (pick(20)) {
			deq_cir( &Bcir, obj, link);
			if (obj) {
				++t.deqs;
				if (pick(50)) {
					enq_cir( &Acir, obj, link); ++t.enqs;
				} else {
					enq_front_cir( &Acir, obj, link); ++t.fronts;
				}
			}
		} else if (pick(20)) {
			peek_cir( &Bcir, peeked, link);
			deq_cir( &Bcir, obj, link);
			assert(peeked == obj);
			++t.peeks;
			if (obj) {
				++t.deqs;
				enq_cir( &Acir, obj, link); ++t.enqs;
			}
		} else if (pick(20)) {
			append_cir( &Acir, &Bcir); ++t.appends;
			assert(is_empty_cir( &Bcir));
			assert(cnt_cir( &Acir) == NUM_OBJS);
		} else if (pick(20)) {
			append_cir( &Bcir, &Acir); ++t.appends;
		} else if (pick(20)) {
			unint	i = urand(NUM_OBJS);

			if (rmv_cir( &Acir, &CirObj[i], link)) {
				++t.rmvs;
			} else if (rmv_cir( &Bcir, &CirObj[i], link)) {
				++t.rmvs;
			} else {
				printf("Cound't remove\n");
			}
			enq_cir( &Acir, &CirObj[i], link); ++t.enqs;
		} else if (pick(20)) {
			unint	cnt = 0;

			foreach_cir( &Acir, fcount, &cnt);
			if (cnt != cnt_cir( &Acir)) {
				printf("foreach cout %ld != %ld\n", cnt, cnt_cir( &Acir));
			}
			++t.foreaches;
		}
		if (cnt_cir( &Acir) + cnt_cir( &Bcir) != NUM_OBJS) {
			printf("%ld + %ld != %d\n",
				cnt_cir( &Acir), cnt_cir( &Bcir), NUM_OBJS);
		}
		++t.cnts;
	}
	report_cir( &t);
}
/*======================================================================*/

typedef struct DqObj_s {
	int		a;
	int		b;
	dqlink_t	link;
} DqObj_s;

DqObj_s	DqObj[NUM_OBJS];
d_q	Adq;
d_q	Bdq;

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
	unint	nexts;
} DqTest_s;

void report_dq (DqTest_s *t)
{
	printf("dq enqs=%ld fronts=%ld deqs=%ld backs=%ld peeks=%ld"
		" rmvs=%ld cnts=%ld appends=%ld foreaches=%ld nexts=%ld\n",
		t->enqs, t->fronts, t->deqs, t->backs, t->peeks,
		t->rmvs, t->cnts, t->appends, t->foreaches, t->nexts);
}

void test_dq (unint numTests)
{
	unint		i;
	DqObj_s		*obj;
	DqObj_s		*peeked;
	DqTest_s	t;

	zero(t);

	init_dq( &Adq);
	init_dq( &Bdq);
	for (i = 0; i < NUM_OBJS; ++i) {
		init_qlink(DqObj[i].link);
		enq_dq( &Adq, &DqObj[i], link); ++t.enqs;
	}
	for (i = 0; i < numTests; i++) {
		if (pick(20)) {
			deq_dq( &Adq, obj, link);
			if (obj) {
				++t.deqs;
				if (pick(50)) {
					enq_dq( &Bdq, obj, link); ++t.enqs;
				} else {
					enq_front_dq( &Bdq, obj, link); ++t.fronts;
				}
			}
		} else if (pick(20)) {
			if (pick(50)) {
				deq_dq( &Bdq, obj, link);
				if (obj) ++t.deqs;
			} else {
				deq_back_dq( &Bdq, obj, link);
				if (obj) ++t.backs;
			}
			if (obj) {
				enq_dq( &Adq, obj, link); ++t.enqs;
			}
		} else if (pick(20)) {
			peek_dq( &Bdq, peeked, link);
			deq_dq( &Bdq, obj, link);
			assert(peeked == obj);
			++t.peeks;
			if (obj) {
				++t.deqs;
				enq_dq( &Adq, obj, link); ++t.enqs;
			}
		} else if (pick(20)) {
			append_dq( &Adq, &Bdq); ++t.appends;
			assert(is_empty_dq( &Bdq));
			assert(cnt_dq( &Adq) == NUM_OBJS);
		} else if (pick(20)) {
			append_dq( &Bdq, &Adq); ++t.appends;
		} else if (pick(20)) {
			unint	i = urand(NUM_OBJS);

			rmv_dq( &DqObj[i].link); ++t.rmvs;
			enq_dq( &Adq, &DqObj[i], link); ++t.enqs;
		} else if (pick(20)) {
			unint	cnt = 0;

			foreach_dq( &Adq, fcount, &cnt);
			if (cnt != cnt_dq( &Adq)) {
				printf("foreach count %ld != %ld\n", cnt, cnt_dq( &Adq));
			}
			++t.foreaches;
		} else if (pick(20)) {
			unint	cnt = 0;

			for (obj = NULL;; cnt++) {
				next_dq(obj, &Adq, link);
				if (!obj) break;
			}
			if (cnt != cnt_dq( &Adq)) {
				printf("next count %ld != %ld\n", cnt, cnt_dq( &Adq));
			}
			++t.nexts;
		}
		if (cnt_dq( &Adq) + cnt_dq( &Bdq) != NUM_OBJS) {
			printf("%ld + %ld != %d\n",
				cnt_dq( &Adq), cnt_dq( &Bdq), NUM_OBJS);
		}
		++t.cnts;
	}
	report_dq( &t);
}

int main (int argc, char *argv[])
{
	unint		numTests = 100000;

	if (argc > 1) numTests = atoi(argv[1]);

	test_stk(numTests);
	test_stk(numTests);
	test_ring(numTests);
	test_ring(numTests);
	test_cir(numTests);
	test_cir(numTests);
	test_dq(numTests);
	test_dq(numTests);

	return 0;
}
