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
#include <string.h>

#include <eprintf.h>
#include <debug.h>

#include <tau/msg.h>

#include <sw.h>

typedef struct request_s {
	name_s		*r_name;
	client_s	*r_client;
	unsigned	r_key;
	unsigned	r_type;
} request_s;

static request_s	Request[MAX_REQUESTS];
static request_s	*NextRequest = Request;

void request_add (unsigned key, name_s *name, client_s *client, unsigned type)
{
	request_s	*r;
FN;
	for (r = Request; r < NextRequest; r++) {
		if (!r->r_key) goto found;
	}
	if (NextRequest == &Request[MAX_REQUESTS]) {
		eprintf("Too many requests");
	}
	r = NextRequest++;
found:
	r->r_key    = key;
	r->r_client = client;
	r->r_name   = name;
	r->r_type   = type;
}

void request_delete (client_s *client)
{
	request_s	*r;
FN;
	for (r = Request; r < NextRequest; r++) {
		if (r->r_client == client) {
			destroy_key_tau(r->r_key);
			zero(*r);
		}
	}
}

void request_honor (unsigned key, name_s *name, unsigned type)
{
	request_s	*r;
FN;
	for (r = Request; r < NextRequest; r++) {
		if ((r->r_name == name) && (r->r_type & type) && r->r_key) {
			dup_send_key(r->r_key, key, name, 0);
			if (r->r_type & JUST_ONE) {
				zero(*r);
			}
		}
	}
}
