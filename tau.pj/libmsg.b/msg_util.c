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
#include <stdarg.h>

#include <eprintf.h>
#include <debug.h>

#include <tau/msg.h>
#include <tau/msg_util.h>

ki_t make_gate (
	void		*tag,
	unsigned	type)
{
	msg_s	msg;
	int	rc;

	msg.q.q_tag     = tag;
	msg.q.q_type    = type & ~(READ_DATA | WRITE_DATA);
	rc = create_gate_tau( &msg);
	if (rc) {
		eprintf("make_gate: create_gate_tau failed %d", rc);
	}
	return msg.q.q_passed_key;
}

ki_t make_datagate (
	void		*tag,
	unsigned	type,
	void		*start,
	unsigned	length)
{
	msg_s	msg;
	int	rc;

PRp(tag);
PRp(start);
	msg.q.q_tag     = tag;
	msg.q.q_type    = type;

	msg.q.q_start  = start;
	msg.q.q_length = length;

	rc = create_gate_tau( &msg);
	if (rc) {
		eprintf("make_gate: create_gate_tau failed %d", rc);
	}
	return msg.q.q_passed_key;
}

type_s *make_type (char *name, method_f destroy, ...)
{
	va_list		ap;
	unint		count = 0;
	type_s		*t;
	method_f	method;
	unint		i;

	// Count number of methods
	va_start(ap, destroy);
	for (;;) {
		method = va_arg(ap, method_f);
		if (!method) break;
		++count;
		if (count > 255) eprintf("Too many methods");
	}
	va_end(ap);

	t = ezalloc(sizeof(*t) + count*sizeof(method_f));
	t->ty_num_methods = count;
	t->ty_destroy     = destroy;

	// Assign methods to the type
	va_start(ap, destroy);
	for (i = 0; i < count; i++) {
		method = va_arg(ap, method_f);
		t->ty_method[i] = method;
	}
	va_end(ap);
	return t;
}
