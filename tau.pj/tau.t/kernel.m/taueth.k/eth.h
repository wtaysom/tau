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

#ifndef _ETH_H_
#define _ETH_H_ 1

/*
 * For compatibility, so that this driver builds for kernels with
 * or without MoE already in them.  I've just picked a number.
 */
#ifndef ETH_P_NET
#define ETH_P_NET 0x8979
#endif


enum { MAC_SIZE = 6 };

/*
 * Max ethernet packet is 1518 octets and a jumbo is 9000 octets.
 * Want to make sure header and other information is packed in so that
 * I can send a 4K page in  3 packets: 1366, 1365, 1365
 */
enum {	CHUNK_SIZE = 4096 / 3 + 1 };

enum {	CMD_INIT,
	CMD_SEND,
	CMD_DESTROY,
	CMD_START_READ,
	CMD_COPY_TO};

typedef struct eth_hdr_s {
	u8	h_dst[MAC_SIZE];
	u8	h_src[MAC_SIZE];
	u16	h_type;
	u16	h_version;

	u64	h_sent;
	u64	h_recv;
	u8	h_cmd;
	u8	h_passed_mac[MAC_SIZE];
	u8	h_reserved[sizeof(u64) - MAC_SIZE - 1];
} eth_hdr_s;

enum { CHECK_PACKET_SIZE = 1 / ((sizeof(eth_hdr_s)+sizeof(packet_s)+CHUNK_SIZE)
				<= 1518) };

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))

/* For compatibility with SLES 11 */

static inline void skb_reset_mac_header (struct sk_buff *skb)
{
	skb->mac.raw = skb->data;
}

static inline void skb_reset_network_header (struct sk_buff *skb)
{
	skb->nh.raw = skb->data;
}

static inline unsigned char *skb_mac_header (struct sk_buff *skb)
{
	return skb->mac.raw;
}

#endif

#endif

