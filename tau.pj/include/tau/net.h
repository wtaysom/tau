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
 * Defines the constants and data structures used by the network.
 */
#ifndef _TAU_NET_H_
#define _TAU_NET_H_ 1

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

enum {	MAX_NODES      = 10,
	NET_ADDR_SIZE  = 16 };	/* Max network address size (IPv6) */

typedef struct key_s {
	u32	k_length;
	u16	k_node;
	u8	k_type;
	u8	k_reserved;
	u64	k_id;
} key_s;

enum packet_flags {
	ALLOCATED = 0x1,
	NETWORK   = 0x2,
	LAST      = 0x4};

typedef struct packet_s {
	s32		pk_error;
	u32		pk_flags;
	key_s		pk_key;
	key_s		pk_passed_key;
	sys_s		pk_sys;
	u8		pk_body[0];
} packet_s;

typedef struct data_s {	// Mirrors the data area in sys_s in msg.h
	void	*q_start;
	PADDING(start);
	u64	q_length;
	u64	q_offset;
} data_s;

typedef struct node_s	node_s;
struct node_s {
	node_s			*n_next;
	u8			n_netaddr[NET_ADDR_SIZE];
	u16			n_slot;
	u64			n_sent;
	u64			n_recv;
	struct net_device	*n_ifp;
};

typedef int	(*copy_f)(void *destination, void *source, u64 offset, unint length);

int write_to_net (
	key_s		*key,
	data_s		*data,
	copy_f		copy);

int read_to_net (
	key_s	*key,
	data_s	*data,
	copy_f	copy,
	void	*gate);

int copy_to_avatar_tau (packet_s *p);
int read_from_avatar_tau (packet_s *p);

int  msg_plug_network (packet_s *packet);
int  msg_init_net (char *net_name);
void msg_exit_net (void);

extern node_s	*Tau_node_slot[];
node_s *find_node (u8 *netaddr, unint size);
node_s *new_node  (u8 *netaddr, unint size);

void net_deliver_tau   (packet_s *p);
void net_go_public_tau (packet_s *p);

extern int	(*Net_send)(packet_s *packet);
extern int	(*Net_broadcast)(packet_s *packet);
extern int	(*Net_start_read)(packet_s *packet);
extern int	(*Net_read)(key_s *key, data_s *data, copy_f copy, void *gate);
extern int	(*Net_write)(key_s *key, data_s *data, copy_f copy);
extern void	(*Net_node_id)(unint index, guid_t node_id);

#endif

