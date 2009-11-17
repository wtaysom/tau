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

#define DEBUG 1

#include <linux/hdreg.h>
#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/version.h>

#include <tau/msg.h>
#define tMASK	tNET
#include <tau/debug.h>
#include <tau/net.h>
#include "eth.h"

MODULE_AUTHOR("Paul Taysom <Paul.Taysom@novell.com>");
MODULE_DESCRIPTION("tau raw ethernet interface");
MODULE_LICENSE("GPL v2");

enum { NUM_WORKERS = 4 };

typedef struct work_s {
	struct work_struct	w_work;
	struct sk_buff		*w_skb;
} work_s;

typedef struct worker_s {
	struct work_struct	wk_work;
	struct sk_buff		*wk_skb;
	struct workqueue_struct	*wk_que;
	unint			wk_inuse;
} worker_s;

typedef struct crew_s {
	unint		cw_num_refs;
	worker_s	cw_worker[NUM_WORKERS];
} crew_s;

struct workqueue_struct	*Work_queue[NUM_WORKERS];

struct {
	u64	send_alloc;
	u64	send_dest;
	u64	recv_alloc;
	u64	recv_dest;
} Cnt;

static node_s	*My_node;

struct workqueue_struct	*Net_workq;
struct workqueue_struct	*Lite_net_workq;

DEFINE_PER_CPU(crew_s, Crews);

static void destroy_net_workqs (void)
{
	if (Net_workq) {
		flush_workqueue(Net_workq);
		destroy_workqueue(Net_workq);
	}
	if (Lite_net_workq) {
		flush_workqueue(Lite_net_workq);
		destroy_workqueue(Lite_net_workq);
	}
}

static int init_net_workqs (void)
{
	Net_workq = create_workqueue("Net_workq");
	if (!Net_workq) {
		destroy_net_workqs();
		return -ENOMEM;
	}
	Lite_net_workq = create_workqueue("Lite_net_workq");
	if (!Lite_net_workq) {
		destroy_net_workqs();
		return -ENOMEM;
	}
	return 0;
}

static void cleanup_crews (void)
{
	int	i;

	for (i = 0; i < NUM_WORKERS; i++) {
		flush_workqueue(Work_queue[i]);
		destroy_workqueue(Work_queue[i]);
	}
}

static void net_receive (void *data);

static void init_worker (worker_s *worker, int cpu)
{
	worker->wk_skb   = NULL;
	worker->wk_que   = Work_queue[cpu];
	worker->wk_inuse = FALSE;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
	INIT_WORK( &worker->wk_work, net_receive, (void *)worker);
#else
	INIT_WORK( &worker->wk_work, net_receive);
#endif
}

static int init_crews (void)
{
	crew_s	*crew;
	char	name[20];
	int	cpu;
	int	i;

	for (i = 0; i < NUM_WORKERS; i++) {
		sprintf(name, "Net_workq/%d", i);
		Work_queue[i] = create_workqueue(name);
		if (!Work_queue[i]) {
			eprintk("create_workqueue %s", name);
			cleanup_crews();
			return -ENOMEM;
		}
	}
	for_each_cpu(cpu) {
		crew = &per_cpu(Crews, cpu);
		crew->cw_num_refs = 0;
		for (i = 0; i < NUM_WORKERS; i++) {
			init_worker( &crew->cw_worker[i], cpu);
		}
	}
	return 0;
}

static void send_destructor (struct sk_buff *skb)
{
	++Cnt.send_dest;
	if (Tau_debug_trace) printk("SEND DESTRUCTOR CALLED %lld %lld\n",
					Cnt.send_alloc, Cnt.send_dest);
	skb->destructor = NULL;
}

struct sk_buff *new_skb (ulong len)
{
	struct sk_buff	*skb;
	eth_hdr_s	*h;
FN;
	if (len < ETH_ZLEN) len = ETH_ZLEN;

	skb = alloc_skb(len, GFP_ATOMIC);
	if (!skb) {
		iprintk("skb alloc failure\n");
		return NULL;
	}
	++Cnt.send_alloc;
	skb_reset_mac_header(skb);
	skb_reset_network_header(skb);
	skb->protocol = __constant_htons(ETH_P_NET);
	skb->priority = 0;
	skb_put(skb, len);
	memset(skb->head, 0, ETH_ZLEN);
	skb->next = skb->prev = NULL;

	skb->destructor = send_destructor;

	/*
	 * Tell the network layer not to perform IP checksums
	 * or to get the NIC to do it
	 */
	skb->ip_summed = CHECKSUM_NONE;

	/*
	 * Setup network information
	 */
	dev_hold(My_node->n_ifp);
	skb->dev = My_node->n_ifp;
	h = (eth_hdr_s *)skb_mac_header(skb);
	memset(h, 0, sizeof *h);
	h->h_type = __constant_cpu_to_be16(ETH_P_NET);

	memcpy(h->h_src, My_node->n_ifp->dev_addr, sizeof h->h_src);


	return skb;
}

static void pr_hdr (char *label, eth_hdr_s *h)
{
	u8	*d = h->h_dst;
	u8	*s = h->h_src;
	u8	*p = h->h_passed_mac;

	if (!Tau_debug_trace) return;

	printk("<2>" "### %s Hdr: "
		"%p\n"
		"dst=%.2x%.2x%.2x%.2x%.2x%.2x "
		"src=%.2x%.2x%.2x%.2x%.2x%.2x\n"
		"    type=%x sent=%llx recv=%llx cmd=%x "
		"key=%.2x%.2x%.2x%.2x%.2x%.2x\n",
		label, h,
		d[0], d[1], d[2], d[3], d[4], d[5],
		s[0], s[1], s[2], s[3], s[4], s[5],
		h->h_type, h->h_sent, h->h_recv, h->h_cmd,
		p[0], p[1], p[2], p[3], p[4], p[5]);

	tau_pr_packet(h+1);
}


static void set_counts (node_s *node, eth_hdr_s *h)
{
	h->h_sent = ++node->n_sent;
	h->h_recv = node->n_recv;
}		

static u8 *key_to_mac (key_s *key, void *dst)
{
	node_s	*node = Tau_node_slot[key->k_node];
FN;
	return memcpy(dst, node->n_netaddr, MAC_SIZE);
}

static void set_destination (key_s *key, eth_hdr_s *h)
{
	node_s	*node = Tau_node_slot[key->k_node];
FN;
	memcpy(h->h_dst, node->n_netaddr, MAC_SIZE);
	set_counts(node, h);
}

static bool is_broadcast (u8 *mac)
{
	unint	i;

	for (i = 0; i < MAC_SIZE; i++) {
		if (*mac++ != 0xff) return FALSE;
	}
	return TRUE;
}

static void set_key_node (key_s *key, u8 *mac)
{
	node_s	*node;
FN;
	if (!key->k_id) return;

	node = find_node(mac, MAC_SIZE);

	if (!node) {
		if (is_broadcast(mac)) return;
		node = new_node(mac, MAC_SIZE);
		assert(node);
	}
	key->k_node = node->n_slot;
}

static void check_counts (eth_hdr_s *h)
{
	node_s	*node;
	u64	recvd;

	node = find_node(h->h_src, MAC_SIZE);
	if (!node) {
		if (Tau_debug_trace) printk("No node found\n");
		return;
	}
	recvd = h->h_sent - 1;
	if (node->n_recv < recvd) {
		eprintk("Missing a message %lld < %lld\n",
			node->n_recv, recvd);
	} else if (node->n_recv > recvd) {
		eprintk("Recieved more than sent %lld > %lld\n",
			node->n_recv, recvd);
	} else {
		if (Tau_debug_trace) printk("OK: Next msg received %lld == %lld\n",
						node->n_recv, recvd);
	}
	node->n_recv = h->h_sent;

	if (node->n_sent < h->h_recv) {
		eprintk("Received more than sent %lld < %lld\n",
			node->n_sent, h->h_recv);
	} else if (node->n_sent > h->h_recv) {
		if (Tau_debug_trace)  printk("OK: Hasn't received all"
						" that were sent %lld > %lld\n",
						node->n_sent, h->h_recv);
	} else {
		if (Tau_debug_trace) printk("OK: Received all that"
						" were sent %lld == %lld\n",
						node->n_sent, h->h_recv);
	}
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
static void net_receive (void *data)
{
	work_s		*work = data;
#else
static void net_receive (struct work_struct *w)
{
	work_s		*work = container(w, work_s, w_work);
#endif
	struct sk_buff	*skb = work->w_skb;
	eth_hdr_s	*h;
	packet_s	*p;
FN;
	h = (eth_hdr_s *)skb_mac_header(skb);
	check_counts(h);
	p = (packet_s *)(h + 1);
	set_key_node( &p->pk_key, h->h_dst);
	set_key_node( &p->pk_passed_key, h->h_passed_mac);

	switch (h->h_cmd) {

	case CMD_INIT:
		net_go_public_tau(p);
		break;

	case CMD_SEND:
		net_deliver_tau(p);
		break;

	case CMD_START_READ:
		read_from_avatar_tau(p);
		break;

	case CMD_COPY_TO:
		copy_to_avatar_tau(p);
		break;

	default:
		printk("Bad cmd %d\n", h->h_cmd);
		break;
	}
	dev_kfree_skb(skb);
	kfree(work);
}

static void finish_send (struct sk_buff *skb)
{
	int	rc;
FN;
	rc = dev_queue_xmit(skb);
	dev_put(My_node->n_ifp);
}

static int write_chunk (
	key_s		*key,
	char		*start,
	unint		length,
	u64		offset,
	copy_f		copy)
{
	eth_hdr_s	*h;
	packet_s	*p;
	struct sk_buff	*skb;
	int		rc;
FN;

	skb = new_skb(sizeof(*h) + sizeof(packet_s) + length);
	if (!skb) return -ENOMEM;

	h = (eth_hdr_s *)skb_mac_header(skb);
	h->h_cmd = CMD_COPY_TO;

	set_destination(key, h);

	p                     = (packet_s *)(h + 1);
	p->pk_key             = *key;
	p->pk_passed_key.k_id = 0;

	p->pk_sys.q_offset = offset;
	p->pk_sys.q_length = length;

	rc = copy(p->pk_body, start, 0, length);
	if (rc) {
		dev_kfree_skb(skb);
		return rc;
	}
	finish_send(skb);
	return 0;
}

static int read_chunk (
	key_s		*key,
	void		*gate,
	unint		length,
	u64		offset,
	copy_f		copy,
	bool		is_last)
{
	eth_hdr_s	*h;
	packet_s	*p;
	struct sk_buff	*skb;
	int		rc;
FN;
	skb = new_skb(sizeof(*h) + sizeof(packet_s) + length);
	if (!skb) return -ENOMEM;

	h = (eth_hdr_s *)skb_mac_header(skb);
	h->h_cmd = CMD_COPY_TO;

	set_destination(key, h);

	p                     = (packet_s *)(h + 1);
	p->pk_error           = 0;
	p->pk_flags           = (is_last ? LAST : 0);
	p->pk_key             = *key;
	p->pk_passed_key.k_id = 0;

	p->pk_sys.q_offset = offset;
	p->pk_sys.q_length = length;

	rc = copy(p->pk_body, gate, offset, length);
	if (rc) {
		dev_kfree_skb(skb);
		return rc;
	}
	finish_send(skb);
	return 0;
}

static int net_write (
	key_s		*key,
	data_s		*data,
	copy_f		copy)
{
	u8	*start = data->q_start;
	u64	length = data->q_length;
	u64	offset = data->q_offset;
	unint	size = CHUNK_SIZE;	
	int	rc;
FN;
	for (;;) {
		if (size > length) {
			size = length;
		} else {
			// Last packet.
		}
		rc = write_chunk(key, start, size, offset, copy);
		if (rc) return rc;
		length -= size;
		if (length == 0) {
			return 0;
		}
		start  += size;
		offset += size;
	}
}

static int net_read (
	key_s	*key,
	data_s	*data,
	copy_f	copy,
	void	*gate)
{
	u64	length = data->q_length;
	u64	offset = data->q_offset;
	unint	size = CHUNK_SIZE;
	bool	is_last = FALSE;	
	int	rc;
FN;
	for (;;) {
		if (size >= length) {
			size = length;
			is_last = TRUE;
		}
		rc = read_chunk(key, gate, size, offset, copy, is_last);
		if (rc) return rc;	//XXX: need to do more here.
		length -= size;
		if (length == 0) {
			return 0;
		}
		offset += size;
	}
}

static void __net_send (int cmd, packet_s *p, unint length, bool broadcast)
{
	eth_hdr_s	*h;
	struct sk_buff	*skb;
FN;
	skb = new_skb(sizeof(*h) + sizeof(*p) + length);
	if (!skb)  return;

	h = (eth_hdr_s *)skb_mac_header(skb);
	h->h_cmd = cmd;

	if (broadcast) {
		memset(h->h_dst, 0xff, sizeof h->h_dst);
	} else {
		set_destination( &p->pk_key, h);
	}
	if (p->pk_passed_key.k_id) {
		key_to_mac( &p->pk_passed_key, h->h_passed_mac);
	}
	memcpy(h+1, p, sizeof(*p) + length);

pr_hdr("__net_send", h);
	finish_send(skb);
}

static int net_send (packet_s *p)
{
	__net_send(CMD_SEND, p, BODY_SIZE, FALSE);
	return 0;
}

static int net_broadcast (packet_s *p)
{
	__net_send(CMD_INIT, p, BODY_SIZE, TRUE);
	return 0;
}

static int net_start_read (packet_s *p)
{
	__net_send(CMD_START_READ, p, 0, FALSE);
	return 0;
}

static void net_node_id (unint index, guid_t node_id)
{
	node_s	*node;

	if (index >= MAX_NODES) {
		eprintk("%ld>=%d", index, MAX_NODES);
		memset(node_id, 0, sizeof(guid_t));
		return;
	}
	node = Tau_node_slot[index];
	if (!node) {
		eprintk("no node at slot %ld", index);
		memset(node_id, 0, sizeof(guid_t));
		return;
	}
	memcpy(node_id, node->n_netaddr, sizeof(guid_t));
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
static void net_deliver (void *data)
{
	worker_s	*worker = data;
#else
static void net_deliver (struct work_struct *w)
{
	worker_s	*worker = container(w, worker_s, wk_work);
#endif
	struct sk_buff	*skb = worker->wk_skb;
	eth_hdr_s	*h;
	packet_s	*p;
FN;
	h = (eth_hdr_s *)skb_mac_header(skb);
	check_counts(h);
	p = (packet_s *)(h + 1);
	set_key_node( &p->pk_key, h->h_dst);
	set_key_node( &p->pk_passed_key, h->h_passed_mac);

	switch (h->h_cmd) {

	case CMD_SEND:
		net_deliver_tau(p);
		break;

	default:
		printk("Bad cmd %d\n", h->h_cmd);
		break;
	}
	dev_kfree_skb(skb);
	worker->wk_inuse = FALSE;
}

static void assign_work (struct sk_buff *skb)
{
	crew_s		*crew;
	worker_s	*w;
	int		i;

	// Could use __get_cpu_var here because kernel preemption is
	// already turned off.
	crew = &get_cpu_var(Crews);
	++crew->cw_num_refs;

	for (i = 0; i < NUM_WORKERS; i++) {
		w = &crew->cw_worker[i];
		if (w->wk_inuse) continue;
		w->wk_inuse = TRUE;
		w->wk_skb = skb;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
		INIT_WORK( &w->wk_work, net_deliver, (void *)w);
#else
		INIT_WORK( &w->wk_work, net_deliver);
#endif
		queue_work(w->wk_que, &w->wk_work);
		put_cpu_var(Crews);
		return;
	}
	put_cpu_var(Crews);
	/*
	 * Method of last resort
	 */
	{
		work_s		*work;

		++Cnt.recv_alloc;
		work = kmalloc(sizeof(*work), GFP_ATOMIC);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
		INIT_WORK( &work->w_work, net_receive, (void *)work);
#else
		INIT_WORK( &work->w_work, net_receive);
#endif
		work->w_skb = skb;

		queue_work(Net_workq, &work->w_work);
	}
}

static void recv_destructor (struct sk_buff *skb)
{
	++Cnt.recv_dest;
	if (Tau_debug_trace) printk("RECEIVE DESTRUCTOR CALLED %lld %lld\n",
				Cnt.recv_alloc, Cnt.recv_dest);
	skb->destructor = NULL;
}

static int net_rcv (
	struct sk_buff		*skb,
	struct net_device	*ifp,
	struct packet_type	*pt
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,6))
	, struct net_device	*orig_dev
#endif
)
{
	eth_hdr_s	*h;
	work_s		*work;
FN;
if (!skb->destructor) {
	skb->destructor = recv_destructor;
}
	h = (eth_hdr_s *)skb_mac_header(skb);
	if (h->h_cmd == CMD_SEND) {
		assign_work(skb);
		return 0;
	}
	++Cnt.recv_alloc;
	work = kmalloc(sizeof(*work), GFP_ATOMIC);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
	INIT_WORK( &work->w_work, net_receive, (void *)work);
#else
	INIT_WORK( &work->w_work, net_receive);
#endif
	work->w_skb = skb;
	queue_work(Lite_net_workq, &work->w_work);

	return 0;
}

static struct packet_type net_pt = {
	.type = __constant_htons(ETH_P_NET),
	.func = net_rcv,
};

char	*Device = "eth0";
module_param(Device, charp, 0444);
MODULE_PARM_DESC(Device, "ethernet device used for tau network");

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))

struct net_device *find_net_device (void)
{
	struct net_device	*ifp;
FN;
	for (ifp = dev_base; ifp; dev_put(ifp), ifp = ifp->next) {
		dev_hold(ifp);
		if (strcmp(ifp->name, Device) == 0) {
			return ifp;
		}
	}
	return NULL;
}

#else

struct net_device *find_net_device (void)
{
	struct net_device	*ifp;
FN;
	read_lock( &dev_base_lock);
	for_each_netdev( &init_net, ifp) {
		dev_hold(ifp);
		if (strcmp(ifp->name, Device) == 0) {
			read_unlock( &dev_base_lock);
			return ifp;
		}
		dev_put(ifp);
	}
	read_unlock( &dev_base_lock);
	return NULL;
}

#endif

static int eth_init (void)
{
	struct net_device	*ifp;
FN;
	ifp = find_net_device();
	if (!ifp) return ENODEV;

	My_node = new_node(ifp->dev_addr, MAC_SIZE);
	if (!My_node) {
		dev_put(ifp);
		return ENOMEM;
	}
	My_node->n_ifp = ifp;
	dev_put(ifp);

	init_net_workqs();
	init_crews();
	dev_add_pack( &net_pt);

	Net_send       = net_send;
	Net_broadcast  = net_broadcast;
	Net_start_read = net_start_read;
	Net_read       = net_read;
	Net_write      = net_write;
	Net_node_id    = net_node_id;

	return 0;
}

static void eth_exit (void)
{
FN;
	dev_remove_pack( &net_pt);
	cleanup_crews();
	destroy_net_workqs();
}

module_init(eth_init)
module_exit(eth_exit)

