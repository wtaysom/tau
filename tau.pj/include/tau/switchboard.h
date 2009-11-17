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

#ifndef _TAU_SWITCHBOARD_H_
#define _TAU_SWITCHBOARD_H_ 1

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

/*
 * Names are case sensitive, null terminated, utf-8 strings
 * with no slashes, '/' because in the future, they may be
 * file names.
 */

enum { SW_KEY = 1 };

enum { SW_ERR_ALREADY_REGISTERED = 2000 };

enum { SW_REGISTER, SW_REGISTER_MAX };
enum { SW_POST, SW_GET, SW_NEXT, SW_OPS };
enum { SW_KEY_SUPPLIED, SW_REPLY_MAX };

enum {	SW_LOCAL    = 0x1,
	SW_REMOTE   = 0x2,
	SW_ALL      = SW_LOCAL | SW_REMOTE };

typedef struct name_s {
	u8	nm_length;
	u8	nm_name[TAU_NAME];
} name_s;

typedef struct sw_msg_s {
	sys_s	q;
	union {
		body_u	b;

		struct {
			u8	sw_method;
			u8	sw_request;
			name_s	sw_name;
			u64	sw_sequence;
		};
	};
} sw_msg_s;

int sw_register  (const char *name);
int sw_post      (const char *name, ki_t key);
int sw_request   (const char *name, ki_t key);
int sw_remote    (const char *name, ki_t key);
int sw_local     (const char *name, ki_t *key);
int sw_any       (const char *name, ki_t *key);
int sw_next      (const char *name, ki_t *key, u64 start, u64 *next);
int sw_post_v    (unint len, const void *name, ki_t key);
int sw_request_v (unint len, const void *name, ki_t key);
int sw_remote_v  (unint len, const void *name, ki_t key);
int sw_any_v     (unint len, const void *name, ki_t *key);

#endif
