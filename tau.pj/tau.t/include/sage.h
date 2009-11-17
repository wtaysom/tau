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

#ifndef _SAGE_H_
#define _SAGE_H_ 1

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

enum { SAGE_MKFS, SAGE_NEXT_FM, SAGE_OPS };

enum { SAGE_REGISTER, SAGE_REGISTER_OPS };

typedef struct sagemsg_s {
	sys_s	q;
	union {
		METHOD_S;
		body_u	b;
		struct {
			METHOD_S;
			guid_t	sg_guid_vol;
			guid_t	sg_guid_bag;
		};
		struct {
			METHOD_S;
			char	sg_name[TAU_NAME];
		};
		struct {
			METHOD_S;
			char	fm_name[TAU_NAME];
			guid_t	fm_guid;
			u64	fm_sequence;
		};
	};
} sagemsg_s;

#endif
