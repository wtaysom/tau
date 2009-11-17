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

#ifndef _DATA_H_
#define _DATA_H_ 1

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

enum { TST_READ_DATA, TST_WRITE_DATA };

typedef struct data_msg_s {
	sys_s	q;
	union {
		METHOD_S;
		body_u	b;

		struct {
			METHOD_S;
			u64	dt_num_bytes;
			u64	dt_seed;
		};
	};
} data_msg_s;

#endif
