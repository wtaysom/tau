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

#ifndef _MSG_DEBUG_H_
#define _MSG_DEBUG_H_ 1

#ifndef _TAU_DEBUG_H_
#include <tau/debug.h>
#endif

#ifndef _MSG_DEV_H_
#include "msg_internal.h"
#endif

int pr_msg   (void *msg);
int pr_key   (char *s, void *key);
int pr_body  (void *body);
void pr_mb   (void *m);
void pr_mac  (void *mac);
void pr_gate (datagate_s *gate);

void in_call  (avatar_s *avatar, ki_t key_index, msgbuf_s *mb);
void out_call (avatar_s *avatar, msgbuf_s *mb, int rc);

void in_change_index  (avatar_s *avatar, ki_t key_index, ki_t std_index);
void out_change_index (avatar_s *avatar, int rc);

void in_create_gate  (avatar_s *avatar, msg_s *msg);
void out_create_gate (avatar_s *avatar, ki_t key_index, u64 gate_id, int rc);

void in_destroy_gate  (avatar_s *avatar, u64 id);
void out_destroy_gate (avatar_s *avatar, int rc);

void in_destroy_key  (avatar_s *avatar, ki_t key_index);
void out_destroy_key (avatar_s *avatar, int rc);

void in_duplicate_key  (avatar_s *avatar, ki_t key_index);
void out_duplicate_key (avatar_s *avatar, ki_t key_index, int rc);

void in_node_died  (avatar_s *avatar, u64 node_no);
void out_node_died (avatar_s *avatar);

void in_plug_key  (avatar_s *avatar, ki_t plug, msgbuf_s *mb);
void out_plug_key (avatar_s *avatar, int rc);

void in_receive  (avatar_s *avatar);
void out_receive (avatar_s *avatar, msgbuf_s *mb, int rc);

void in_read_data  (avatar_s *avatar, ki_t key_index, void *data);
void out_read_data (avatar_s *avatar, int rc);

void in_send  (avatar_s *avatar, ki_t key_index, msgbuf_s *mb);
void out_send (avatar_s *avatar, int rc);

void in_stat_key  (avatar_s *avatar, ki_t key_index);
void out_stat_key (avatar_s *avatar, msg_s *msg, int rc);

void in_write_data  (avatar_s *avatar, ki_t key_index, void *data);
void out_write_data (avatar_s *avatar, int rc);

#endif
