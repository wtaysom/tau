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

#ifndef _TAU_BICHO_H_
#define _TAU_BICHO_H_ 1

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

enum { BICHO_WHO_AM_I, BICHO_PS, BICHO_NEXT_PID, BICHO_AVATAR, BICHO_GATE, BICHO_SW_NUM };
enum { BICHO_AVATAR_STAT, BICHO_AVATAR_KEYS, BICHO_AVATAR_GATES, BICHO_AVATAR_NUM };

typedef struct bicho_msg_s {
	sys_s   q;
	union {
		body_u  b;
		METHOD_S;

		struct {
			METHOD_S;
			char	bi_name[TAU_NAME];
			guid_t	bi_id;
		};
		struct {
			METHOD_S;
			char	ps_name[TAU_NAME];
			u64	ps_id;
			u32	ps_num_waiters;
			u32	ps_num_msgs;
			u8	ps_dieing;
		};
		struct {
			METHOD_S;
			u64	key_index;
			u32	key_length;
			u16	key_node_index;
			u8	key_type;
			u8	key_reserved;
			u64	key_id;
			guid_t	key_node_id;
		};
		struct {
			METHOD_S;
			u64	gate_id;
			u64	gate_tag;
			u64	gate_start;
			u64	gate_length;
			u64	gate_avatar;
			u8	gate_type;
		};
	};
} bicho_msg_s;

enum { BICHO_MSG_ASSERT = 1 / (sizeof(bicho_msg_s) == sizeof(msg_s)) };

int cyclic_bicho (ki_t key, u32 msec, ki_t *ret_key);

#endif
