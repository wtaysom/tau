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
 *	Stk		- Linked list stack
 *	Ring	- Fixed size ring buffer
 *	Cir		- Singlely linked circular queue
 *	Dq		- Doublely linked circular queue
 */
#ifndef _Q_H_
#define _Q_H_

#ifndef _STYLE_H_
#include <style.h>
#endif

typedef struct qlink_t
{
	struct qlink_t	*next;
} qlink_t;

typedef struct dqlink_t
{
	struct dqlink_t	*next;
	struct dqlink_t	*prev;
} dqlink_t;

typedef struct Stk_q
{
	qlink_t	*top;
	unint	offset;
} Stk_q;

typedef struct Ring_q
{
	void	**enq;
	void	**deq;
	void	**start;
	void	**end;
} Ring_q;

typedef struct Cir_q
{
	qlink_t	*last;
	unint	offset;
} Cir_q;

typedef struct D_q
{
	dqlink_t	head;
	unint		offset;
} D_q;

typedef bool (*qfunc)(void *obj, void *data);

#define isMember(_x)	((_x).next)
#define initLink(_x)	((_x).next = 0)

#define isEmptyStk(_stk)	((_stk)->top == 0)
void initStk(Stk_q *stk, unint offset);
void pushStk(Stk_q *stk, void *obj);
void *popStk(Stk_q *stk);
void *peekStk(Stk_q *stk);
void appendStk(Stk_q *dst, Stk_q *src);
unint cntStk(Stk_q *stk);
bool foreachStk(Stk_q *stk, qfunc f, void *args);

#define isEmptyRing(_ring)	((_ring)->enq == (_ring)->deq)	
void initRing(Ring_q *ring, unint numObjs, void **ringBuffer);
Ring_q *newRing(Ring_q *ring, unint numObjs);
bool enqRing(Ring_q *ring, void *obj);
bool enqFrontRing(Ring_q *ring, void *obj);
void *deqRing(Ring_q *ring);
void *deqBackRing(Ring_q *ring);
void *peekRing(Ring_q *ring);
void *rmvRing(Ring_q *ring, void *obj);
unint cntRing(Ring_q *ring);
bool appendRing(Ring_q *dstq, Ring_q *srcq);
bool foreachRing(Ring_q *ring, qfunc f, void *args);

#define isEmptyCir(_cir)	((_cir)->last == 0)	
void initCir(Cir_q *cir, unint offset);
void enqCir(Cir_q *cir, void *obj);
void enqFrontCir(Cir_q *cir, void *obj);
void *deqCir(Cir_q *cir);
void *peekCir(Cir_q *cir);
void *rmvCir(Cir_q *cir, void *obj);
void appendCir(Cir_q *dstq, Cir_q *srcq);
unint cntCir(Cir_q *cir);
bool foreachCir(Cir_q *cir, qfunc f, void *args);

#define isEmptyDq(_dq)	((_dq)->head.next == &(_dq)->head)	
void initDq(D_q *dq, unint offset);
void enqDq(D_q *dq, void *obj);
void enqFrontDq(D_q *dq, void *obj);
void *deqDq(D_q *dq);
void *deqBackDq(D_q *dq);
void *peekDq(D_q *dq);
void rmvDq(dqlink_t *link);
void appendDq(D_q *dstq, D_q *srcq);
unint cntDq(D_q *dq);
bool foreachDq(D_q *dq, qfunc f, void *args);

#endif
