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


#include <stdio.h>
#include <assert.h>
#include <que.h>

#define NSS_DEBUG DISABLE
#if NSS_DEBUG IS_ENABLED
int LBQ_QAssertError (char *msg, Link_t item)
{
	static char assertmsg[] = MSGNot("\tQASSERT ERROR: %s item=0x%x next=0x%x\n");

	printf(assertmsg, msg, item, item->next);

	EnterDebugger();
	return FALSE;
}
#endif

/************************************************************************/

void LBQ_STKinitElements (STKtop_t *stacktop, addr data, unint number, unint size)
{
	unint		i;
	STKtop_t	top;

	STK_INIT(top);
	for (i = 0; i < number; ++i)
	{
		NULLIFY(data);
		STK_PUSH(top, (STKlink_t)data, next);
		data += size;
	}
	*stacktop = top;
}

void LBQ_STKpush (STKtop_t *stacktop, STKlink_t item)
{
	if (IS_NULL(item))
	{
		item->next = (*stacktop);
		*stacktop  = item;
	}
}

addr LBQ_STKpop (STKtop_t *stacktop, unint offset)
{
	STKlink_t	next;

	next = *stacktop;
	if (next == NULL)
	{
		return 0;
	}
	else
	{
		*stacktop = next->next;
		NULLIFY(next);
		return ((addr)next) - offset;
	}
}

addr LBQ_STKpopNoCheck (STKtop_t *stacktop, unint offset)
{
	STKlink_t	next;

	next = *stacktop;
	assert(next != NULL);
	*stacktop = next->next;
	NULLIFY(next);
	return ((addr)next) - offset;
}

void LBQ_STKdrop (STKtop_t *stacktop)
{
	STKlink_t	next;

	next = *stacktop;
	assert(next != NULL);

	*stacktop = next->next;
	NULLIFY(next);
}

int LBQ_STKrmv (STKtop_t *top, STKlink_t item)
{
	STKlink_t	next, prev;

	prev = (STKlink_t)top;
	if (prev == NULL)
	{
		return FALSE;
	}
	for (next = prev->next; next != NULL; next = next->next)
	{
		if (next == item)
		{
			prev->next = next->next;
			NULLIFY(item);
			return TRUE;
		}
		prev = next;
	}
	return FALSE;
}

/************************************************************************/

void LBQ_SQinitElements (SQhead_t *sqhead, addr data, unint number, unint size)
{
	unint		i;

	SQ_INIT(sqhead);
	for (i = 0; i < number; ++i)
	{
		NULLIFY(data);
		SQ_ENQ(sqhead, (SQlink_t)data, next);
		data += size;
	}
}

void LBQ_SQenq (SQhead_t *head, SQlink_t item)
{
	if (IS_NULL(item))
	{
		head->last->next = item;
		head->last       = item;
	}
}

void LBQ_SQpush (SQhead_t *head, SQlink_t item)
{
	if (IS_NULL(item))
	{
		if (SQ_EMPTY(head))
		{
			head->last = item;
		}
		else
		{
			item->next = head->next;
		}
		head->next = item;
	}
}

addr LBQ_SQdeq (SQhead_t *head, unint offset)
{
	SQlink_t		item;
	SQlink_t		last;

	last = head->last;
	if (last == (SQlink_t)head)
	{
		return 0;
	}
	else
	{
		item = head->next;
		if (last == item)
		{
			head->last = (SQlink_t)head;
		}
		else
		{
			head->next = item->next;
		}
		NULLIFY(item);
		return ((addr)item) - offset;
	}
}

addr LBQ_SQdeqNoCheck (SQhead_t *head, unint offset)
{
	SQlink_t		item;
	SQlink_t		last;

	last = head->last;
	item = head->next;
	if (last == item)
	{
		head->last = (SQlink_t)head;
	}
	else
	{
		head->next = item->next;
	}
	NULLIFY(item);
	return ((addr)item) - offset;
}

void LBQ_SQdrop (SQhead_t *head)
{
	SQlink_t		item;
	SQlink_t		last;

	last = head->last;
	item = head->next;
	if (last == item)
	{
		head->last = (SQlink_t)head;
	}
	else
	{
		head->next = item->next;
	}
	NULLIFY(item);
}

int LBQ_SQrmv (SQhead_t *head, SQlink_t item)
{
	SQlink_t	next, prev;
	SQlink_t	last;

	last = head->last;
	if (last != (SQlink_t)head)
	{
		prev = (SQlink_t)head;
		for (next = head->next; next != last; next = next->next)
		{
			if (next == item)
			{
				prev->next = next->next;
				NULLIFY(item);
				return TRUE;
			}
			prev = next;
		}
		if (item == last)
		{
			head->last = prev;
			NULLIFY(item);
		}
		return TRUE;
	}
	return FALSE;
}

void LBQ_SQappend (SQhead_t *head, SQhead_t *tail)
{
	if (SQ_NOT_EMPTY(tail))
	{
		head->last->next = tail->next;
		head->last       = tail->last;
		SQ_INIT(tail);
	}
}

void LBQ_SQprepend (SQhead_t *head, SQhead_t *tail)
{
	if (SQ_NOT_EMPTY(tail))
	{
		if (SQ_EMPTY(head))
		{
			head->last = tail->last;
		}
		else
		{
			tail->last->next = head->next;
		}
		head->next = tail->next;
		SQ_INIT(tail);
	}
}

int LBQ_SQfind (SQhead_t *head, SQlink_t item)
{
	SQlink_t	next;
	SQlink_t	last;

	last = head->last;
	if (last != (SQlink_t)head)
	{
		next = (SQlink_t)head;
		do
		{
			next = (SQlink_t)NEXT(next);
			if (next == item)
			{
				return TRUE;
			}
		} while (next != last);
	}
	return FALSE;
}

int LBQ_SQcnt (SQhead_t *head)
{
	SQlink_t	next;
	SQlink_t	last;
	int		count = 0;

	last = head->last;
	if (last != (SQlink_t)head)
	{
		next = (SQlink_t)head;
		do
		{
			++count;
			next = (SQlink_t)NEXT(next);
		} while (next != last);
	}
	return count;
}

/************************************************************************/

void LBQ_CIRinitElements (CIRhead_t *cirhead, addr data, unint number, unint size)
{
	unint		i;
	CIRhead_t	head;

	CIR_INIT(head);
	for (i = 0; i < number; ++i)
	{
		NULLIFY(data);
		CIR_ENQ(head, (SQlink_t)data, next);
		data += size;
	}
	*cirhead = head;
}

void LBQ_CIRenq (CIRhead_t *cirhead, CIRlink_t item)
{
	CIRhead_t	head = *cirhead;

	if (IS_NULL(item))
	{
		if (CIR_EMPTY(head))
		{
			item->next = item;
		}
		else
		{
			item->next = head->next;
			head->next = item;
		}
		*cirhead = item;
	}
}

void LBQ_CIRpush (CIRhead_t *cirhead, CIRlink_t item)
{
	CIRhead_t	head = *cirhead;

	if (IS_NULL(item))
	{
		if (CIR_EMPTY(head))
		{
			item->next = item;
			*cirhead = item;
		}
		else
		{
			item->next = head->next;
			head->next = item;
		}
	}
}

addr LBQ_CIRdeq (CIRhead_t *cirhead, unint offset)
{
	CIRhead_t	head = *cirhead;
	CIRlink_t	item;

	if (CIR_EMPTY(head))
	{
		return 0;
	}
	else
	{
		item = head->next;
		if (item == head)
		{
			*cirhead = NULL;
		}
		else
		{
			head->next = item->next;
		}
		NULLIFY(item);
		return ((addr)item) - offset;
	}
}

addr LBQ_CIRdeqNoCheck (CIRhead_t *cirhead, unint offset)
{
	CIRhead_t	head = *cirhead;
	CIRlink_t	item;

	item = head->next;
	if (item == head)
	{
		*cirhead = NULL;
	}
	else
	{
		head->next = item->next;
	}
	NULLIFY(item);
	return ((addr)item) - offset;
}

void LBQ_CIRdrop (CIRhead_t *cirhead)
{
	CIRhead_t	head = *cirhead;
	CIRlink_t	item;

	item = head->next;
	if (item == head)
	{
		*cirhead = NULL;
	}
	else
	{
		head->next = item->next;
	}
	NULLIFY(item);
}

int LBQ_CIRrmv (CIRhead_t *cirhead, CIRlink_t item)
{
	CIRhead_t	head = *cirhead;
	CIRlink_t	prev;
	CIRlink_t	next;

	if (CIR_EMPTY(head))
	{
		return FALSE;
	}
	prev = head;
	for (next = (CIRlink_t)NEXT(head);
		next != head;
		next = (CIRlink_t)NEXT(next))
	{
		if (next == item)
		{
			NEXT(prev) = NEXT(next);
			NULLIFY(item);
			return TRUE;
		}
		prev = next;
	}
	if (item == head)
	{
		if (prev == item)
		{
			*cirhead = NULL;
		}
		else
		{
			NEXT(prev) = NEXT(item);
			*cirhead = prev;
		}
		NULLIFY(item);
		return TRUE;
	}
	return FALSE;
}

void LBQ_CIRappend (CIRhead_t *cirhead, CIRhead_t *cirtail)
{
	CIRhead_t	head = *cirhead;
	CIRhead_t	tail = *cirtail;
	CIRlink_t		temp;

	if (CIR_NOT_EMPTY(tail))
	{
		if (CIR_NOT_EMPTY(head))
		{
			temp = tail->next;
			tail->next = head->next;
			head->next = temp;
		}
		*cirhead = tail;
		CIR_INIT(*cirtail);
	}
}

void LBQ_CIRprepend (CIRhead_t *cirhead, CIRhead_t *cirtail)
{
	CIRhead_t	head = *cirhead;
	CIRhead_t	tail = *cirtail;
	CIRlink_t		temp;

	if (CIR_NOT_EMPTY(tail))
	{
		if (CIR_NOT_EMPTY(head))
		{
			temp = tail->next;
			tail->next = head->next;
			head->next = temp;
		}
		else
		{
			*cirhead = tail;
		}
		CIR_INIT(*cirtail);
	}
}

int LBQ_CIRfind (CIRhead_t cirhead, CIRlink_t item)
{
	CIRlink_t	next;

	if (CIR_NOT_EMPTY(cirhead))
	{
		next = cirhead;
		do
		{
			if (next == item)
			{
				return TRUE;
			}
			next = (CIRlink_t)NEXT(next);
		} while (next != cirhead);
	}
	return FALSE;
}

int LBQ_CIRcnt (CIRhead_t cirhead)
{
	CIRlink_t	next;
	int			count = 0;

	if (CIR_NOT_EMPTY(cirhead))
	{
		next = cirhead;
		do
		{
			++count;
			next = (CIRlink_t)NEXT(next);
		} while (next != cirhead);
	}
	return count;
}

/************************************************************************/

typedef struct DQdummy_s
{
	DQlink_t	link;
} DQdummy_s;

void LBQ_DQinitElements (DQhead_t *head, addr data, unint number, unint size)
{
	unint		i;

	DQ_INIT(head);
	for (i = 0; i < number; ++i)
	{
		NULLIFY(data);
		DQ_ENQ(head, (DQdummy_s *)data, link);
		data += size;
	}
}

void LBQ_DQenq (DQhead_t *head, DQlink_t *item)
{
	if (IS_NULL(item))
	{
		item->prev = head->prev;
		item->next = head;
		head->prev->next = item;
		head->prev = item;
	}
}

void LBQ_DQpush (DQhead_t *head, DQlink_t *item)
{
	if (IS_NULL(item))
	{
		item->next = head->next;
		item->prev = head;
		head->next->prev = item;
		head->next = item;
	}
}

addr LBQ_DQdeq (DQhead_t *head, unint offset)
{
	DQlink_t		*item;

	if (DQ_EMPTY(head))
	{
		return 0;
	}
	else
	{
		item = head->next;
		head->next = item->next;
		head->next->prev = head;
		NULLIFY(item);
		return ((addr)item) - offset;
	}
}

addr LBQ_DQdeqNoCheck (DQhead_t *head, unint offset)
{
	DQlink_t		*item;

	item = head->next;
	head->next = item->next;
	head->next->prev = head;
	NULLIFY(item);
	return ((addr)item) - offset;
}

addr LBQ_DQtake (DQhead_t *head, unint offset)
{
	DQlink_t		*item;

	if (DQ_EMPTY(head))
	{
		return 0;
	}
	else
	{
		item = head->prev;
		head->prev = item->prev;
		head->prev->next = head;
		NULLIFY(item);
		return ((addr)item) - offset;
	}
}

void LBQ_DQdrop (DQhead_t *head)
{
	DQlink_t		*item;

	item = head->next;
	head->next = item->next;
	head->next->prev = head;
	NULLIFY(item);
}

void LBQ_DQrmv (DQlink_t *item)
{
	assert(QMEMBER(item));

	item->next->prev = item->prev;
	item->prev->next = item->next;
	NULLIFY(item);
}

void LBQ_DQappend (DQhead_t *head, DQhead_t *tail)
{
	if (DQ_NOT_EMPTY(tail))
	{
		tail->prev->next = head;
		tail->next->prev = head->prev;
		head->prev->next = tail->next;
		head->prev = tail->prev;
		DQ_INIT(tail);
	}
}

void LBQ_DQprepend (DQhead_t *head, DQhead_t *tail)
{
	if (DQ_NOT_EMPTY(tail))
	{
		tail->prev->next = head->next;
		head->next->prev = tail->prev;
		tail->next->prev = head;
		head->next = tail->next;
		DQ_INIT(tail);
	}
}

int LBQ_DQfind (DQhead_t *head, DQlink_t *item)
{
	DQlink_t	*next;

	DQ_FOREACH(head, next, DQlink_t, next)
	{
		if (next == item)
		{
			return TRUE;
		}
	}
	return FALSE;
}

int LBQ_DQcnt (DQhead_t *head)
{
	DQlink_t	*next;
	int		count = 0;

	DQ_FOREACH(head, next, DQlink_t, next)
	{
		++count;
	}
	return count;
}

/************************************************************************/

typedef struct SETdummy_s
{
	SETlink_t	link;
} SETdummy_s;

void LBQ_SETinitElements (SEThead_t *head, addr data, unint number, unint size)
{
	unint		i;

	SET_INIT(head);
	for (i = 0; i < number; ++i)
	{
		NULLIFY(data);
		SET_ENQ(head, (SETdummy_s *)data, link);
		data += size;
	}
}

void LBQ_SETenq (SEThead_t *head, SETlink_t *item)
{
	if (IS_NULL(item))
	{
		item->setNum = head->setNum++;
		item->prev = head->prev;
		item->next = head;
		head->prev->next = item;
		head->prev = item;
	}
}

void LBQ_SETpush (SEThead_t *head, SETlink_t *item)
{
	if (IS_NULL(item))
	{
		item->setNum = head->prev->setNum - 1;
		item->next = head->next;
		item->prev = head;
		head->next->prev = item;
		head->next = item;
	}
}

addr LBQ_SETdeq (SEThead_t *head, unint offset)
{
	SETlink_t		*item;

	if (SET_EMPTY(head))
	{
		return 0;
	}
	else
	{
		item = head->next;
		head->next = item->next;
		head->next->prev = head;
		NULLIFY(item);
		return ((addr)item) - offset;
	}
}

addr LBQ_SETdeqNoCheck (SEThead_t *head, unint offset)
{
	SETlink_t		*item;

	item = head->next;
	head->next = item->next;
	head->next->prev = head;
	NULLIFY(item);
	return ((addr)item) - offset;
}

addr LBQ_SETtake (SEThead_t *head, unint offset)
{
	SETlink_t		*item;

	if (SET_EMPTY(head))
	{
		return 0;
	}
	else
	{
		item = head->prev;
		head->prev = item->prev;
		head->prev->next = head;
		NULLIFY(item);
		return ((addr)item) - offset;
	}
}

void LBQ_SETdrop (SEThead_t *head)
{
	SETlink_t		*item;

	item = head->next;
	head->next = item->next;
	head->next->prev = head;
	NULLIFY(item);
}

void LBQ_SETrmv (SETlink_t *item)
{
	assert(QMEMBER(item));

	item->next->prev = item->prev;
	item->prev->next = item->next;
	NULLIFY(item);
}

void LBQ_SETappend (SEThead_t *head, SEThead_t *tail)
{
	if (SET_NOT_EMPTY(tail))
	{
		tail->prev->next = head;
		tail->next->prev = head->prev;
		head->prev->next = tail->next;
		head->prev = tail->prev;
		SET_INIT(tail);
	}
}

void LBQ_SETprepend (SEThead_t *head, SEThead_t *tail)
{
	if (SET_NOT_EMPTY(tail))
	{
		tail->prev->next = head->next;
		head->next->prev = tail->prev;
		tail->next->prev = head;
		head->next = tail->next;
		SET_INIT(tail);
	}
}

int LBQ_SETfind (SEThead_t *head, SETlink_t *item)
{
	SETlink_t	*next;

	SET_FOREACH(head, next, SETlink_t, next)
	{
		if (next == item)
		{
			return TRUE;
		}
	}
	return FALSE;
}

int LBQ_SETcnt (SEThead_t *head)
{
	SETlink_t	*next;
	int		count = 0;

	SET_FOREACH(head, next, SETlink_t, next)
	{
		++count;
	}
	return count;
}
