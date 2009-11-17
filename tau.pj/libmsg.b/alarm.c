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

#include <tau/switchboard.h>
#include <tau/alarm.h>

int cyclic_alarm (ki_t key, u32 msec, ki_t *ret_key)
{
	alarm_msg_s	m;
	unsigned	local_key;
	unint		alarm_key;
	int		rc;
	
	rc = sw_local("alarm", &local_key);
	if (rc) return rc;

	m.al_method = ALARM_CREATE;
	rc = call_tau(local_key, &m);
	destroy_key_tau(local_key);
	if (rc) {
		return rc;
	}
	alarm_key = m.q.q_passed_key;
	m.al_method = ALARM_START;
	m.al_msec   = msec;
	m.q.q_passed_key = key;
	rc = send_key_tau(alarm_key, &m);
	if (rc) {
		destroy_key_tau(alarm_key);
		return rc;
	}
	*ret_key = alarm_key;
	return 0;
}
