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

#ifndef _TAU_ALARM_H_
#define _TAU_ALARM_H_ 1

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

enum { ALARM_CREATE, ALARM_SW_NUM };
enum { ALARM_START, ALARM_NUM };
enum { ALARM_POP };

typedef struct alarm_msg_s {
	sys_s   q;
	union {
		body_u  b;

		struct {
			u8      al_method;
			u32	al_msec;
		};
	};
} alarm_msg_s;

int cyclic_alarm (ki_t key, u32 msec, ki_t *ret_key);

#endif
