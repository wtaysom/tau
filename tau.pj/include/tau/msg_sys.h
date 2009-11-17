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

/*
 * Defines the interface between user space and
 * the kernel character device for message passing.
 */
#ifndef _TAU_MSG_SYS_H_
#define _TAU_MSG_SYS_H_ 1

/* May want to split this into write/read cases (send_t) */

#define DEV_NAME	"msg"
#define FULL_DEV_NAME	"/dev/" DEV_NAME

#define ARG_SHIFT	8
#define ARG_MASK	((1 << ARG_SHIFT) - 1)

#define MAKE_ARG(_x, _r)	((_x << ARG_SHIFT) | (_r))
#define ARG_REQUEST(_x)		((_x) & ARG_MASK)
#define ARG_RCV_MASK(_x)	((_x) >> ARG_SHIFT)
#define ARG_KEY(_x)		((_x) >> ARG_SHIFT)
#define ARG_ID(_x)		((_x) >> ARG_SHIFT)

typedef enum read_t {
	MSG_CALL,
	MSG_CHANGE_INDEX,
	MSG_CREATE_GATE,
	MSG_DESTROY_GATE,
	MSG_DESTROY_KEY,
	MSG_DUPLICATE_KEY,
	MSG_NAME,
	MSG_NODE_DIED,
	MSG_NODE_ID,
	MSG_PLUG_KEY,
	MSG_READ_DATA,
	MSG_RECEIVE,
	MSG_SEND,
	MSG_STAT_KEY,
	MSG_WRITE_DATA,
	MSG_LAST
} read_t;

#endif

