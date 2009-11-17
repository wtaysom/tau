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
#ifndef _TAU_YY_H_
#define _TAU_YY_H_ 1

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

enum { LOGIN, LOGIN_METHODS };

enum { LOOKUP, FS_METHODS };

enum { SEND_TST, READ_TST, WRITE_TST, TEST_METHODS };

enum { MAX_PATH = 4097 };

typedef struct yy_msg_s {
	sys_s	q;
	union {
		METHOD_S;
		body_u	b;

		struct {
			u8	yy_method;
			u8	yy_reserved[7];
			u64	yy_id;
			u64	yy_num;
			u64	yy_crc;
		};
	};
} yy_msg_s;

#endif
