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

#ifndef _NETWORK_H_
#define _NETWORK_H_

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

typedef struct net_msg_s {
	sys_s	q;
	union {
		METHOD_S;
		body_u	b;

		struct {
			METHOD_S;
			guid_t	net_id;
		};

		struct {
			METHOD_S;
			guid_t	net_node_id;
		};

		struct {
			char	net_dev_name[BODY_SIZE];	/* Ignored */
		};
	};
} net_msg_s;

#endif
