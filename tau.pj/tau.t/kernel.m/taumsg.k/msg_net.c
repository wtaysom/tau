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

#include <linux/module.h>

#include <tau/msg.h>
#include <tau/net.h>

#include "msg_debug.h"

static int no_send (packet_s *packet)
{
	iprintk("net_send not plugged");
	return 0;
}

static int no_broadcast (packet_s *packet)
{
	iprintk("net_broadcast not plugged");
	return 0;
}

static int no_start_read (packet_s *packet)
{
	iprintk("net_start_read not plugged");
	return 0;
}

static void no_node_id (unint index, guid_t guid)
{
	iprintk("no_node_id not plugged");
}

static int no_read (key_s *key, data_s *data, copy_f copy, void *gate)
{
	iprintk("net_write not plugged");
	return 0;
}

static int no_write (key_s *key, data_s *data, copy_f copy)
{
	iprintk("net_write not plugged");
	return 0;
}

int	(*Net_send)(packet_s *packet)       = no_send;
int	(*Net_broadcast)(packet_s *packet)  = no_broadcast;
int	(*Net_start_read)(packet_s *packet) = no_start_read;
void	(*Net_node_id)(unint index, guid_t node_id)                    = no_node_id;
int	(*Net_read)(key_s *key, data_s *data, copy_f copy, void *gate) = no_read;
int	(*Net_write)(key_s *key, data_s *data, copy_f copy)            = no_write;

EXPORT_SYMBOL(Net_send);
EXPORT_SYMBOL(Net_broadcast);
EXPORT_SYMBOL(Net_start_read);
EXPORT_SYMBOL(Net_node_id);
EXPORT_SYMBOL(Net_read);
EXPORT_SYMBOL(Net_write);
