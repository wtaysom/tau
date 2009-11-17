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
#include <string.h>
#include <uuid/uuid.h>

#include <q.h>
#include <eprintf.h>
#include <debug.h>

#include <tau/tag.h>
#include <tau/switchboard.h>
#include <tau/msg_util.h>
#include <tau/fsmsg.h>
#include <sage.h>

#include <sage_hdr.h>


typedef struct fm_s {
	dqlink_t	fm_link;
	unint		fm_key;
	char		fm_name[TAU_NAME];
	guid_t		fm_guid;
	u64		fm_sequence;
} fm_s;

d_q	Fm_list = static_init_dq(Fm_list);

#if 0
fm_s *lookup_fm (guid_t guid)
{
	fm_s	*fm;
FN;
	for (fm = NULL;;) {
		next_dq(fm, &Fm_list, fm_link);
		if (!fm) return NULL;
		if (uuid_compare(guid, fm->fm_guid) == 0) return fm;
	}
}

void have_fm (void *msg)
{
	static u64	sequence = 0;
FN;
	fsmsg_s	*m = msg;
	unint	key = m->q.q_passed_key;
	fm_s	*fm;
	int	rc;

	m->m_method = FM_WHO_AM_I;
	rc = call(key, m);
	if (rc) {
		weprintf("have_fm call %d", rc);
		destroy_key_tau(key);
		return;
	}

	fm = lookup_fm(m->fm_guid);
	if (fm) {
		destroy_key_tau(fm->fm_key);
	} else {
		fm = ezalloc(sizeof(*fm));
		fm->fm_sequence = ++sequence;
		uuid_copy(fm->fm_guid, m->fm_guid);
		enq_dq( &Fm_list, fm, fm_link);
	}
	fm->fm_key = key;
	strncpy(fm->fm_name, m->fm_name, sizeof(fm->fm_name));
}
#endif
		
void init_tau (void)
{
#if 0
	static type_s	*fm;

	ki_t	key;
	int	rc;
FN;
	fm  = make_type("sage_fm", NULL, have_fm, NULL);
	key = make_gate( &fm, RESOURCE | PASS_OTHER);
	
	rc = sw_request("fm", key);
	if (rc) eprintf("init_tau sw_request %d", rc);
#endif
}

void next_fm (void *msg)
{
	sagemsg_s	*m = msg;
	unint		reply = m->q.q_passed_key;
	fm_s		*fm;
	int		rc;
FN;
	for (fm = NULL;;) {
		next_dq(fm, &Fm_list, fm_link);
		if (!fm) {
			destroy_key_tau(reply);
			return;
		}
		if (m->fm_sequence < fm->fm_sequence) {
			rc = duplicate_key_tau(fm->fm_key, m);
			if (rc) eprintf("next_fm duplicate_key_tau %d", rc);
			strncpy(m->fm_name, fm->fm_name, sizeof(m->fm_name));
			uuid_copy(m->fm_guid, fm->fm_guid);
			m->fm_sequence = fm->fm_sequence;
			rc = send_key_tau(reply, m);
			if (rc) eprintf("next_fm send_key_tau %d", rc);
			return;
		}
	}
}
