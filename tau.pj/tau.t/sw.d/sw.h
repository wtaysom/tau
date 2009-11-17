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

#ifndef _SW_H_
#define _SW_H_ 1

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

#ifndef _TAU_SWITCHBOARD_H_
#include <tau/switchboard.h>
#endif

enum {	MAX_NAMES	= 1047,
	MAX_CLIENTS	= 100,
	MAX_KEYS	= 1000,
	MAX_REQUESTS	= 77,
	MAX_ID		= 16,
	JUST_ONE	= 0x4 };	// See SW_ALL

typedef struct client_s		client_s;

int dup_send_key (ki_t to_key, unsigned key, name_s *name, u64 sequence);

void key_add      (ki_t key, name_s *name, void *owner, unsigned type);
void key_delete   (void *owner);
int  key_send     (ki_t to_key, name_s *name, unsigned type);
int  key_send_any (ki_t to_key, name_s *name, unsigned type);
int  key_send_all (ki_t to_key, unsigned type);
int  key_next     (ki_t to_key, name_s *name, unsigned type, u64 sequence);

void node_send (ki_t key, name_s *name);

void request_add    (ki_t key, name_s *name, client_s *client, unsigned type);
void request_delete (client_s *client);
void request_honor  (ki_t key, name_s *name, unsigned type);

name_s *name_find (name_s *name);

int init_net (char *net_dev);
int init_client (void);

#endif
