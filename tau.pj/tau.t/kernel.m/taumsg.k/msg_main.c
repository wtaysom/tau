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
 * Once a gate is created, is its state immutable?
 *
 * Data gates are variable length, may be I just reserve a fixed size
 * then have a larger area for large data moves. 
 *
 */

#define DEBUG 1

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/kallsyms.h>
#include <asm/uaccess.h>

#include <tau/msg.h>
#include <tau/msg_sys.h>

#include "msg_debug.h"
#include "msg_internal.h"
#include "network.h"

MODULE_AUTHOR("Paul Taysom <Paul.Taysom@novell.com>");
MODULE_DESCRIPTION("msg character device");
MODULE_LICENSE("GPL v2");

#define WARNING	"<1>"


Inst_s	Inst;	// expose through sysfs

struct workqueue_struct	*Msg_workqueue;

key_s	Client_key;
key_s	Network_key;

msg_spinlock_s	Msg_spinlock;

d_q	Avatars = static_init_dq(Avatars);

#if 0
	#define LOCK_AVATAR(_av)	\
		(msg_spinlock( &(_av)->av_spinlock, WHERE))

	#define UNLOCK_AVATAR(_ch)	\
		(msg_spinunlock( &(_av)->av_spinlock, WHERE))
#else
	#define LOCK_AVATAR(_av)	((void)0)
	#define UNLOCK_AVATAR(_av)	((void)0)
#endif


static int kreceive (
	avatar_s	*avatar,
	char __user	*msg);

static int msg_destroy_key (
	avatar_s	*avatar,
	unsigned	key_index);


/*
 * Utilities
 */

void msg_init_spinlock (msg_spinlock_s *lock, char *name)
{
	spin_lock_init( &lock->msg_spinlock);

	lock->msg_process = 0;
	lock->msg_where = WHERE;
	lock->msg_name  = name;
	lock->msg_flags = 0;
}

#define SPINLOCK_TIMEOUT ENABLE
#if SPINLOCK_TIMEOUT IS_ENABLED
void show_stack (struct task_struct *task, unsigned long *sp)
{
	static void (*fp)(struct task_struct *task, unsigned long *sp) = NULL;
	char *symName = "show_stack";

	if (!fp) {
		fp = (void *)kallsyms_lookup_name(symName);
		if (!fp) {
			printk(KERN_ALERT "ERR: Symbol[%s] unavailable.\n", symName);
			return;
		}
	}
	fp(task, sp);
}
#endif

void msg_spinlock (msg_spinlock_s *lock, char *where)
{
	unsigned long	flags;

	if (lock->msg_process == current) {
		printk("<1>"
			"Current<%p> already has spinlock %s at %s\n",
			current, lock->msg_name, lock->msg_where);
		BUG();
	}
#if SPINLOCK_TIMEOUT IS_ENABLED
	/*
	 * To use this code
	 *  1. change the 0 to a 1, recompile
	 *  2. Don't have to do this test anymore:
	 *	goto /usr/src/linux/arch/i386/kernel/traps.c
	 *	and EXPORT_SYMBOL(show_stack)
	 *  3. recompile kernel
	 */
	{
		static int dead = FALSE;
		int n = 20*1000*1000;

		if (dead) bug("=====taumsg spinlock problem already detected====");
		for (;;) {
			if (spin_trylock_irqsave( &lock->msg_spinlock, flags)) {
				break;
			}
			if (!n--) {
				dead = TRUE;
				printk(KERN_EMERG "---------------- "
					"Stack dump %p ----------------\n",
					lock->msg_process);
				show_stack((struct task_struct *)lock->msg_process,
						NULL);
				printk(KERN_EMERG "---------------- "
					"Stack end  %p ----------------\n",
					lock->msg_process);
				BUG();
			}
		}
	}
#else
	spin_lock_irqsave( &lock->msg_spinlock, flags);
#endif
	lock->msg_process = current;
	lock->msg_where   = where;
	lock->msg_flags   = flags;
}

void msg_locked (msg_spinlock_s *lock, char *where)
{
	if (lock->msg_process != current) {
		printk("<1>"
			"Current<%p> doesn't hold spinlock %s at %s\n",
			current, lock->msg_name, lock->msg_where);
		BUG();
	}
}

void msg_unlocked (msg_spinlock_s *lock, char *where)
{
	if (lock->msg_process == current) {
		printk("<1>"
			"Current<%p> shouldn't be holding spinlock %s at %s\n",
			current, lock->msg_name, lock->msg_where);
		BUG();
	}
}

void msg_spinunlock (msg_spinlock_s *lock, char *where)
{
	if (lock->msg_process != current) {
		printk("<1>"
			"Current<%p> tried to unlock spinlock %s at %s\n",
			current, lock->msg_name, where);
		BUG();
	}
	lock->msg_process = 0;
	lock->msg_where   = where;
	spin_unlock_irqrestore( &lock->msg_spinlock, lock->msg_flags);
}

void mod_put (void)
{
	module_put(THIS_MODULE);
}

void mod_get (void)
{
	int	rc;

	rc = try_module_get(THIS_MODULE);
	if (!rc) {
		eprintk("try_module_get %d", rc);
	}
}

/*
 * Msg work queue
 *	1. Data move operations
 *	2. Channel clean-up
 */
static int init_msg_workqueue (void)
{
	Msg_workqueue = create_workqueue("copy");
	if (!Msg_workqueue) {
		return -ENOMEM;
	}
	return 0;
}

static void destroy_msg_workqueue (void)
{
	if (Msg_workqueue) {
		flush_workqueue(Msg_workqueue);
		destroy_workqueue(Msg_workqueue);
	}
}

void net_send (msgbuf_s *mb)
{
FN;
	UNLOCK_MSG;
	Net_send( &mb->mb_packet);
	LOCK_MSG;
	free_mb(mb);
}

static void broadcast (msgbuf_s *mb)
{
FN;
	UNLOCK_MSG;
	Net_broadcast( &mb->mb_packet);
	LOCK_MSG;
	free_mb(mb);
}

/*
 * Avatar
 */
static kmem_cache_t	*Avatar_pool;

static void init_avatar_once (
	void		*data,
	kmem_cache_t	*cachep,
	unsigned long	flags)
{
	avatar_s *avatar = data;

	zero(*avatar);
}

static int init_avatar_pool (void)
{
FN;
	Avatar_pool = kmem_cache_create("avatar_pool",
				sizeof(avatar_s), 0,
				SLAB_HWCACHE_ALIGN|SLAB_RECLAIM_ACCOUNT,
				init_avatar_once, NULL);
	if (!Avatar_pool) return -ENOMEM;
	return 0;
}

static void destroy_avatar_pool (void)
{
FN;
	if (Avatar_pool && kmem_cache_destroy(Avatar_pool)) {
		eprintk("not all messages were freed\n");
	}
}

static avatar_s *msg_alloc_avatar (void)
{
	avatar_s	*avatar;
FN;
	UNLOCK_MSG;
	avatar = kmem_cache_alloc(Avatar_pool, SLAB_KERNEL);
	LOCK_MSG;
	if (!avatar) return NULL;

	return  avatar;
}

static void msg_destroy_gates (avatar_s *avatar)
{
	datagate_s	*gate;
FN;
	for (;;) {
		deq_dq( &avatar->av_gates, gate, gt_siblings);
		if (!gate) {
			break;
		}
		++gate->gt_usecnt;
		release_gate(gate);
	}
}

static void msg_destroy_messages (avatar_s *avatar)
{
	msg_s	msg;
FN;
	while (!is_empty_dq( &avatar->av_msgq)) {
		kreceive(avatar, (char *)&msg);
		// may want destroy any keys received here so
		// I can't overflow key chain.  What do I want to
		// do in general about this problem - could give
		// an error on receive.
	}
}

static void msg_destroy_key_chain (avatar_s *avatar)
{
	unsigned	i;
FN;
	for (i = 0; i < MAX_KEYS; i++) {
		msg_destroy_key(avatar, i);
	}
}

static void msg_destroy_avatar (void *work)
{
	avatar_s	*avatar = work;
FN;
	if (Tau_debug_trace) printk("%s died\n", avatar->av_name);
WARN_ON(irqs_disabled());
	LOCK_MSG;
	msg_destroy_gates(avatar);
	msg_destroy_messages(avatar);
	msg_destroy_key_chain(avatar);

	rmv_dq( &avatar->av_link);
	UNLOCK_MSG;
	// This is why in O/P we did this in the process but
	// we have to do it in the kernel.
	kmem_cache_free(Avatar_pool, avatar);
	mod_put();
}

static int init_avatar (
	avatar_s	*avatar,
	const char	*name,
	receive_f	recvfn)
{
	static u64	id = 0;
FN;
	zero(*avatar);
	avatar->av_magic   = AVATAR_MAGIC;
	avatar->av_id      = ++id;
	avatar->av_dieing  = FALSE;
	init_dq( &avatar->av_gates);
	msg_init_spinlock( &avatar->av_spinlock, "Avatar");
	init_dq( &avatar->av_waiters);
	init_dq( &avatar->av_msgq);
	init_key_chain(avatar);
	if (Client_key.k_id) {
		assign_std_key(avatar, SW_STD_KEY, &Client_key);
	}
	strlcpy(avatar->av_name, name, sizeof(avatar->av_name));
	avatar->av_recvfn = recvfn;
	enq_dq( &Avatars, avatar, av_link);
	mod_get();
	return 0;
}

/*
 * Msg pool
 */
static kmem_cache_t	*Msg_pool;

static int init_msg_pool (void)
{
FN;
	Msg_pool = kmem_cache_create("msg_pool", sizeof(msgbuf_s), 0,
				SLAB_HWCACHE_ALIGN|SLAB_RECLAIM_ACCOUNT,
				NULL, NULL);
	if (!Msg_pool) return -ENOMEM;
	return 0;
}

static void destroy_msg_pool (void)
{
FN;
	if (Msg_pool && kmem_cache_destroy(Msg_pool)) {
		eprintk("not all messages were freed");
	}
}

msgbuf_s *alloc_mb (void)
{
	msgbuf_s	*mb;
FN;
//XXX: May want to use a cache pool because we can now block.
//	mb = kmem_cache_alloc(Msg_pool, SLAB_KERNEL);
	mb = kmalloc(sizeof(*mb), GFP_ATOMIC);
	if (!mb) {
		eprintk("no momory for message buffers");
		return NULL;
	}
	init_qlink(mb->mb_avatar);
	mb->mb_packet.pk_error = 0;
	mb->mb_packet.pk_flags = ALLOCATED;
	mb->mb_packet.pk_key.k_id = 0;
	mb->mb_packet.pk_passed_key.k_id = 0;
++Inst.msg_alloc;
	return mb;
}

void free_mb (msgbuf_s *mb)
{
FN;
	if (mb && (mb->mb_packet.pk_flags & ALLOCATED)) {
++Inst.msg_free;
		kfree(mb);
//		kmem_cache_free(Msg_pool, mb);
	}
//printk("MSG %lld %lld\n", Inst.msg_alloc, Inst.msg_free);
}

typedef struct waiter_s {
	dqlink_t		wt_wait;
	struct task_struct	*wt_task;
} waiter_s;

void kick (avatar_s *avatar)
{
	waiter_s	*waiter;
FN;
	deq_dq( &avatar->av_waiters, waiter, wt_wait);
	if (waiter) {
		wake_up_process(waiter->wt_task);
	}
}

int wait (avatar_s *avatar)
{
	struct task_struct	*task = current;
	waiter_s		waiter;
	int			rc = 0;
FN;
	init_qlink(waiter.wt_wait);
	waiter.wt_task = task;
	enq_dq( &avatar->av_waiters, &waiter, wt_wait);

	task->state = TASK_INTERRUPTIBLE;
	for (;;) {
		if (signal_pending(current)) {
			rc = -EINTR;
			break;
		}
		if (!is_empty_dq( &avatar->av_msgq)) {
			break;
		}
		UNLOCK_MSG;
		schedule();
		LOCK_MSG;
		task->state = TASK_INTERRUPTIBLE;
	}
	task->state = TASK_RUNNING; 
	rmv_dq( &waiter.wt_wait);
	return rc;
}

static int copy_data (char __user *msg, data_s *data)
{
	msg_s __user	*m;

	m = (msg_s __user *)msg;
	return copy_from_user(data, &m->q.q_start, sizeof(data_s));
}

static int copy_msg (char __user *msg, msgbuf_s **p_mb)
{
	msgbuf_s	*mb;
	int		rc;

	mb = alloc_mb();
	if (!mb) {
		return ENOMSGS;
	}
	rc = copy_from_user( &mb->mb_packet.pk_sys, msg, sizeof(msg_s));
	if (rc) {
		free_mb(mb);
		return rc;
	}
	*p_mb = mb;
	return 0;
}

static int copy_kmsg (msg_s *msg, msgbuf_s **p_mb)
{
	msgbuf_s	*mb;

	mb = alloc_mb();
	if (!mb) {
		return ENOMSGS;
	}
	memcpy( &mb->mb_packet.pk_sys, msg, sizeof(msg_s));
	*p_mb = mb;
	return 0;
}

static int kreceive (
	avatar_s	*avatar,
	char __user	*msg)
{
	int		rc;
	msgbuf_s	*mb;
	packet_s	*p;
FN;
	for (;;) {
		deq_dq( &avatar->av_msgq, mb, mb_avatar);
		if (mb) {
			p = &mb->mb_packet;
			if (p->pk_passed_key.k_id) {
				p->pk_sys.q_passed_key = new_key(avatar,
							&p->pk_passed_key);
			} else {
				p->pk_sys.q_passed_key = 0;
			}
			rc = copy_to_user(msg, &p->pk_sys,
						sizeof(msg_s));
			if (!rc) {
				rc = p->pk_error;
			}
out_receive(avatar, mb, rc);
			free_mb(mb);
			return rc;
		} else {
			/* No message waiting, block */
			if (avatar->av_dieing) {
				return 0;
			}
			rc = wait(avatar);
			if (rc) {
				return rc;
			}
		}
	}
}

static int cant_pass (key_s *dst, key_s *pass)
{
	if (pass->k_type & ONCE) {
		if (dst->k_type & PASS_REPLY) {
			return 0;
		}
	} else {
		if (dst->k_type & PASS_OTHER) {
			return 0;
		}
	}
	return ECANTPASS;
}

static void broken_gate (msgbuf_s *mb)
{
	packet_s	*p = &mb->mb_packet;
	key_s		*key = &p->pk_passed_key;
FN;
	if (!key->k_id) return;
	if (REQUEST_TYPE(key->k_type)) return;
	p->pk_key = *key;
	key->k_id = 0;
	p->pk_error = DESTROYED;
	deliver(mb);
}

static void send_destroyed (
	datagate_s	*gate,
	msgbuf_s	*mb)
{
	packet_s	*p = &mb->mb_packet;
	avatar_s	*avatar;
	avatar_s	*save_avatar;
FN;
	p->pk_sys.q_tag = gate->gt_tag;
	p->pk_error = DESTROYED;

// This is where we have to call the gate function
	// Queue message
	avatar = gate->gt_avatar;
HERE;
	if (gate->gt_type & KERNEL_GATE) {
HERE;
		save_avatar = peek_avatar();
		enter_tau(avatar);
		out_receive(avatar, mb, p->pk_error);
		UNLOCK_MSG;
		avatar->av_recvfn(p->pk_error, &p->pk_sys);
		LOCK_MSG;
		enter_tau(save_avatar);
HERE;
	} else {
HERE;
		enq_dq( &avatar->av_msgq, mb, mb_avatar);
		kick(avatar);
	}
	free_gate(gate);
}

/*
=============================================================================

_X_ Put mb in reply structure
_X_ wake up reply
___ What to do about key chain - hash is an interesting idea. Could just use
	gates. Radix tree.
*/

int wait_reply (reply_s *reply)
{
	struct task_struct	*task = current;
	int			rc = 0;
FN;
	if (reply->rp_state == REPLY_FINISHED) return rc;
	reply->rp_state = REPLY_WAITING;

	reply->rp_task = task;

	task->state = TASK_INTERRUPTIBLE;
	UNLOCK_MSG;
	schedule();
	LOCK_MSG;

	if (signal_pending(current)) {
		rc = -EINTR;
	}
	task->state = TASK_RUNNING; 
	return rc;
}

void wake_reply (reply_s *reply)
{
FN;
	if (reply->rp_state == REPLY_WAITING) {
		wake_up_process(reply->rp_task);
	}
	reply->rp_state = REPLY_FINISHED;
}

void deliver (msgbuf_s *mb)
{
	packet_s	*p = &mb->mb_packet;
	datagate_s	*gate;
	avatar_s	*avatar;
	avatar_s	*save_avatar;
	s32		error;
FN;
	if (p->pk_key.k_node) {
		net_send(mb);
		return;
	}
	gate = get_gate( &p->pk_key);
	if (!gate) {
		broken_gate(mb);
		return;
	}
	p->pk_sys.q_tag = gate->gt_tag;

	if ((gate->gt_type & ONCE) && (gate->gt_type & PERM)) {
		reply_s	*reply;

		reply = gate->gt_tag;
		reply->rp_mb = mb;
		wake_reply(reply);
		return;
	}
	avatar = gate->gt_avatar;
	error = p->pk_error;
	if (gate->gt_type & KERNEL_GATE) {
		if (p->pk_passed_key.k_id) {
			p->pk_sys.q_passed_key = new_key(avatar,
							&p->pk_passed_key);
			// What if we fail? treat like broken? Maybe
			// .
		}
		save_avatar = peek_avatar();
		enter_tau(avatar);
		for (;;) {
			UNLOCK_MSG;
	out_receive(avatar, mb, p->pk_error);
			avatar->av_recvfn(p->pk_error, &p->pk_sys);
			LOCK_MSG;
			
			free_mb(mb);
			deq_dq( &avatar->av_msgq, mb, mb_avatar);
			if (!mb) break;
			p = &mb->mb_packet;
		}
		enter_tau(save_avatar);
	} else {
		enq_dq( &avatar->av_msgq, mb, mb_avatar);

		kick(avatar);
	}
	if ((gate->gt_type & ONCE) || (error == DESTROYED)) {
		free_gate(gate);
	} else {
		release_gate(gate);
	}
}

u64	Tau_remote_key;

static void track_key (key_s *key, key_s *passed_key)
{
	datagate_s	*gate;

	/* Don't need to track PERM keys */
	if (passed_key->k_type & PERM) return;

	/* Staying on node, no need to track */
	if (!key->k_node) return;

	// May want to put this in net_send
	if (passed_key->k_node) {
		/* Gate is on remote node, notify it that key
		 * is being sent to another node. If sending to
		 * same node this is simpler.
		 */
		++Tau_remote_key;
PRd(Tau_remote_key);
	} else {
		gate = get_gate(passed_key);
		if (!gate) return;

		gate->gt_node = key->k_node;
		release_gate(gate);
	}
}

static int ksend (
	avatar_s	*avatar,
	unsigned	key_index,
	msgbuf_s	*mb)
{
	packet_s	*p = &mb->mb_packet;
	key_s		*key;
	key_s		*passed_key;
FN;
in_send(avatar, key_index, mb);
	key = get_key(avatar, key_index);
	if (!key) return EBADKEY;

	if (p->pk_sys.q_passed_key) {
		if (key_index == p->pk_sys.q_passed_key) return ECANTPASS;
		passed_key = get_key(avatar, p->pk_sys.q_passed_key);
		if (!passed_key) return EBADKEY;
		if (cant_pass(key, passed_key)) return ECANTPASS;
		p->pk_passed_key = *passed_key;
		p->pk_sys.q_type = passed_key->k_type;
		if (passed_key->k_type & DATA_GATE) {
			p->pk_sys.q_length = passed_key->k_length;
		} else {
			p->pk_sys.q_length = passed_key->k_length;
		}
		delete_key(avatar, passed_key);
		track_key(key, &p->pk_passed_key);
	}
	p->pk_key = *key;
	if (key->k_type & ONCE) {
		delete_key(avatar, key);
	}
	deliver(mb);
	return 0;
}

static int user_send (
	avatar_s	*avatar,
	unsigned	key_index,
	char __user	*msg)
{
	msgbuf_s	*mb = NULL;
	int		rc;
FN;
	rc = copy_msg(msg, &mb);
	if (rc) return rc;

	return ksend(avatar, key_index, mb);
}

static int kernel_send (
	avatar_s	*avatar,
	unsigned	key_index,
	msg_s		*m)
{
	msgbuf_s	*mb = NULL;
	int		rc;
FN;
	rc = copy_kmsg(m, &mb);
	if (rc) return rc;

	return ksend(avatar, key_index, mb);
}

static void clear_mb (msgbuf_s *mb)
{
	packet_s	*p = &mb->mb_packet;

	mb->mb_avatar.next   = 0;
	p->pk_error           = 0;
	p->pk_flags           = 0;
	p->pk_passed_key.k_id = 0;
}

static int kcall (
	avatar_s	*avatar,
	unsigned	key_index,
	msgbuf_s	*mb,
	reply_s		*reply)
{
	packet_s	*p = &mb->mb_packet;
	key_s		*key;
	int		rc;
FN;
	in_call(avatar, key_index, mb);

	key = get_key(avatar, key_index);
	if (!key) return EBADKEY;

	if (!key->k_type & PASS_REPLY) return ECANTPASS;
	p->pk_key = *key;

	rc = init_reply_gate( &p->pk_sys, avatar,
			(datagate_s *)&reply->rp_gate, reply);
	if (rc) {
		eprintk("init_reply_gate = %d", rc);
		return rc;
	}
	if (key->k_type & ONCE) delete_key(avatar, key);

	MAKE_KEY(p->pk_passed_key, &reply->rp_gate);
	reply->rp_gate.gt_node = key->k_node;	/* track key */

	p->pk_sys.q_type = reply->rp_gate.gt_type;
	if (reply->rp_gate.gt_type & DATA_GATE) {
		p->pk_sys.q_length = reply->rp_gate.dg_length;
	} else {
		p->pk_sys.q_length = 0;
	}

	deliver(mb);

	rc = wait_reply(reply);
	if (rc) {
		rmv_dq( &mb->mb_avatar);
		rmv_dq( &reply->rp_gate.gt_siblings);
		release_gate((datagate_s *)&reply->rp_gate);
	} else {
		mb = reply->rp_mb;
		p = &mb->mb_packet;
		if (p->pk_passed_key.k_id) {
			p->pk_sys.q_passed_key = new_key(avatar,
							&p->pk_passed_key);
		} else {
			p->pk_sys.q_passed_key = 0;
		}
		rc = mb->mb_packet.pk_error;
	}
	out_call(avatar, reply->rp_mb, rc);
	return rc;
}

static int user_call (
	avatar_s	*avatar,
	unsigned	key_index,
	char __user	*msg)
{
	reply_s		reply;
	msgbuf_s	*mb = &reply.rp_msgbuf;
	packet_s	*p  = &mb->mb_packet;
	int		rc;
FN;
	reply.rp_state = REPLY_STARTING;
	clear_mb(mb);
	rc = copy_from_user( &p->pk_sys, msg, sizeof(msg_s));
	if (rc) return rc;

	rc = kcall(avatar, key_index, mb, &reply);
	if (!rc) {
		rc = copy_to_user(msg, &reply.rp_mb->mb_packet.pk_sys,
					sizeof(msg_s));
		free_mb(reply.rp_mb);
	}
	return rc;
}

static int kernel_call (
	avatar_s	*avatar,
	unsigned	key_index,
	msg_s		*m)
{
	reply_s		reply;
	msgbuf_s	*mb = &reply.rp_msgbuf;
	int		rc;
FN;
	reply.rp_state = REPLY_STARTING;
	clear_mb(mb);
	memcpy( &mb->mb_packet.pk_sys, m, sizeof(msg_s));

	rc = kcall(avatar, key_index, mb, &reply);
	if (!rc) {
		memcpy(m, &reply.rp_mb->mb_packet.pk_sys, sizeof(msg_s));
		free_mb(reply.rp_mb);
	}
	return rc;
}

/*
==============================================================================
*/

/*
 * message operations
 */
static int msg_change_index (
	avatar_s	*avatar,
	ki_t		key_index,
	ki_t		std_index)
{
	key_s		*key;
	int		rc;
FN;
in_change_index(avatar, key_index, std_index);
	key = get_key(avatar, key_index);
	if (!key) {
		rc = EBADKEY;
		goto error;
	}
	rc = assign_std_key(avatar, std_index, key);
	if (rc) goto error;
	delete_key(avatar, key);
error:
out_change_index(avatar, rc);
	return rc;
}

static int user_change_index (
	avatar_s	*avatar,
	unsigned	key_index,
	char __user	*m)
{
	msg_s __user	*msg = (msg_s __user *)m;
	ki_t		std_index;
	int		rc;
FN;
	rc = copy_from_user( &std_index, &msg->q.q_passed_key,
				sizeof(std_index));
	if (rc) {
		return EBADADDR;
	}
	return msg_change_index(avatar, key_index, std_index);
}

static int create_gate (
	avatar_s	*avatar,
	msg_s		*msg)
{
	datagate_s	*gate = NULL;
	key_s		key;
	u32		key_index = 0;
	int		rc;
FN;
in_create_gate(avatar, msg);
	/* Make sure it is all correct */
	if ((msg->q.q_type & VALID_GATE) != msg->q.q_type) {
		rc = EBADGATE;
		goto error;
	}
	if ((msg->q.q_type & (PERM | ONCE)) == (PERM | ONCE)) {
		rc = EBADGATE;
		goto error;
	}
	rc = new_gate(avatar, msg->q.q_type, msg->q.q_tag,
				msg->q.q_start, msg->q.q_length,
				&gate);
	if (rc) {
		goto error;
	}
	MAKE_KEY(key, gate);

	key_index = new_key(avatar, &key);
	if (!key_index) {
		rc = -ENOMEM;
		goto error;
	}
	msg->cr_id = gate->gt_id;
	release_gate(gate);
	// Should I return key_index as return value - avoid copy
	msg->q.q_passed_key = key_index;
	out_create_gate(avatar, key_index, msg->cr_id, rc);
	
	return 0;

error:
	out_create_gate(avatar, key_index, gate ? gate->gt_id : 0, rc);
	if (gate) {
		release_gate(gate);
	}
	return rc;
}

static int user_create_gate (
	avatar_s	*avatar,
	char __user	*umsg)
{
	msg_s	msg;
	int	rc;
FN;
	rc = copy_from_user( &msg, umsg, sizeof(msg));
	if (rc) return rc;

	if (msg.q.q_type & KERNEL_GATE) {
		return EBADGATE;
	}
	rc = create_gate(avatar, &msg);
	if (rc) return rc;

	rc = copy_to_user(umsg+offsetof(msg_s, cr_id), &msg.cr_id,
			sizeof(msg.cr_id));
	if (rc) {
		goto error;
	}
	// Should I return key_index as return value - avoid copy
	rc = copy_to_user(umsg+offsetof(msg_s, q.q_passed_key), &msg.q.q_passed_key,
		sizeof(msg.q.q_passed_key));
	if (rc) {
		goto error;
	}	
	return 0;

error:
//XXX	destroy_gate(msg.cr_id);//XXX: don't want to get destruction notification
	return rc;
}

static int destroy_gate_by_id (
	avatar_s	*avatar,
	u64		id)
{
	datagate_s	*gate;
	int		rc = 0;
FN;
in_destroy_gate(avatar, id);
	gate = find_gate(id);
	if (!gate) {
		return EBADID;
	}
	if (gate->gt_avatar != avatar) {
		return EBADID;
	}
	if (REPLY_TYPE(gate->gt_type) || RESOURCE_TYPE(gate->gt_type)) {
		msgbuf_s	*mb;
LABEL(RightType);
		mb = alloc_mb();
		if (!mb) {
			return ENOMSGS;
		}
		send_destroyed(gate, mb);
	} else {
LABEL(WrongType);
		free_gate(gate);
	}		
	return rc;
}

static int msg_destroy_gate (
	avatar_s	*avatar,
	msg_s __user	*msg)
{
	u64		id;
	int		rc = 0;
FN;
	rc = copy_from_user( &id, &msg->cr_id, sizeof(msg->cr_id));
	if (rc) {
		return rc;
	}
	return destroy_gate_by_id(avatar, id);
}

static int msg_destroy_key (
	avatar_s	*avatar,
	unsigned	key_index)
{
	msgbuf_s	*mb;
	key_s		*key;
	int		rc = 0;

in_destroy_key(avatar, key_index);
	key = get_key(avatar, key_index);
	if (!key) {
		rc = EBADKEY;
		goto exit;
	}
	if (REQUEST_TYPE(key->k_type)) {
		delete_key(avatar, key);
		goto exit;
	}
	mb = alloc_mb();
	if (!mb) {
		rc = ENOMSGS;
		goto exit;
	}
	mb->mb_packet.pk_error = DESTROYED;
	mb->mb_packet.pk_key = *key;
	delete_key(avatar, key);

	// We can unlock avatar here
	deliver(mb);

exit:		
	return rc;
}

static int msg_duplicate_key (
	avatar_s	*avatar,
	unsigned	key_index,
	char __user	*m)
{
	msg_s __user	*msg = (msg_s __user *)m;
	key_s		*key;
	unsigned	dup_index;
	int		rc;
FN;
in_duplicate_key(avatar, key_index);
	// Get gate, is key valid?
	key = get_key(avatar, key_index);
	if (!key) {
		rc = EBADKEY;
		goto error;
	}
	if (!REQUEST_TYPE(key->k_type)) {
		rc = ENODUP;
		goto error;
	}
	dup_index = new_key(avatar, key);
	if (!dup_index) {
		rc = -ENOMEM;
		goto error;
	}
	rc = copy_to_user( &msg->q.q_passed_key, &dup_index, sizeof(dup_index));
	if (rc) {
		key = get_key(avatar, dup_index);
		if (key) { // this should never, ever fail
			rc = EREALLYBAD;
			delete_key(avatar, key);
		}
		goto error;
	}
error:
out_duplicate_key(avatar, dup_index, rc);
	return rc;
}

static int msg_name (
	avatar_s	*avatar,
	char __user	*msg)
{
	msgbuf_s	*mb = NULL;
	msg_s		*m;
	int		rc;
FN;
	rc = copy_msg(msg, &mb);
	if (rc) {
		return rc;
	}
	m = (msg_s *)&mb->mb_packet.pk_sys;
	strlcpy(avatar->av_name, m->my_name,
		sizeof(avatar->av_name));
	free_mb(mb);
	return 0;
}

static int msg_read_data (
	avatar_s	*avatar,
	unsigned	key_index,
	char __user	*msg)
{
	data_s		data;
	int		rc;
FN;
	rc = copy_data(msg, &data);
	if (rc) {
		goto exit;
	}
in_read_data(avatar, key_index, &data);
	rc = rw_data(USER_RW | READ_RW, avatar, key_index, &data);

exit:
out_read_data(avatar, rc);
	return rc;
}

static int msg_write_data (
	avatar_s	*avatar,
	ki_t		key,
	char __user	*msg)
{
	data_s		data;
	int		rc;
FN;
	rc = copy_data(msg, &data);
	if (rc) {
		goto exit;
	}
in_write_data(avatar, key, &data);
	rc = rw_data(USER_RW | WRITE_RW, avatar, key, &data);
exit:
out_write_data(avatar, rc);
	return rc;
}

static int msg_node_id (
	avatar_s	*avatar,
	char __user	*msg)
{
	msgbuf_s	*mb = NULL;
	net_msg_s	*m;
	int		rc;
FN;
	if (current->fsuid) {
		return -EPERM;
	}
	rc = copy_msg(msg, &mb);
	if (rc) {
		return rc;
	}
	m = (net_msg_s *)&mb->mb_packet.pk_sys;
	Net_node_id(0, m->net_node_id);
	rc = copy_to_user(msg, m, sizeof(msg_s));
	free_mb(mb);
	return rc;
}

static addr destroy_gate (void *obj, void *node_no)
{
	datagate_s	*g = container(obj, datagate_s, gt_siblings);
	u64		node = (addr)node_no;
	msgbuf_s	*mb;
FN;
	if (g->gt_node != node) return 0;
	++g->gt_usecnt;
// Refractor with code in msg_destroy_gate
	mb = alloc_mb();
	if (!mb) {
		eprintk("Out of message buffers");
		return 0;
	}	
	send_destroyed(g, mb);
	return 0;
}

static int msg_node_died (
	avatar_s	*avatar,
	u64		node_no)
{
	avatar_s	*av;
FN;
in_node_died(avatar, node_no);
	if (current->fsuid) {
		return -EPERM;
	}
	for (av = NULL;;) {
		next_dq(av, &Avatars, av_link);
		if (!av) {
			out_node_died(avatar);
			return 0;
		}
		foreach_dq( &av->av_gates, destroy_gate, (void *)(addr)node_no);
	}
}

static int msg_plug_key (
	avatar_s	*avatar,
	unsigned	plug,
	char __user	*msg)
{
	extern void init_alarm (void);
	extern void init_bicho (void);

	msgbuf_s	*mb = NULL;
	packet_s	*p;
	key_s		*key;
	int		rc = 0;
FN;
	switch (plug) {
	case PLUG_CLIENT:
	case PLUG_NETWORK:
		break;
	default:
		return -EINVAL;
	}
	rc = copy_msg(msg, &mb);
	if (rc) {
		return rc;
	}
	p = &mb->mb_packet;
in_plug_key(avatar, plug, mb);
	key = get_key(avatar, p->pk_sys.q_passed_key);
	if (!key) {
		rc = EBADKEY;
		goto error;
	}
	if (!REQUEST_TYPE(key->k_type)) {
		rc = EBADTYPE;
		goto error;
	}
	if (current->fsuid) {
		rc = EPERM;
		goto error;
	}
	switch (plug) {

	case PLUG_CLIENT:
		Client_key = *key;
		free_mb(mb);

		UNLOCK_MSG;
		init_alarm();
		init_bicho();
		LOCK_MSG;

		break;

	case PLUG_NETWORK:
		Network_key = *key;
		p->pk_passed_key = *key;
		broadcast(mb);
		break;
	}
	return 0;
error:
	if (mb) {
		free_mb(mb);
	}
	return rc;
}

static int msg_stat_key (
	avatar_s	*avatar,
	unsigned	key_index,
	char __user	*umsg)
{
	msg_s		m;
	key_s		*key;
	int		rc;
FN;
in_stat_key(avatar, key_index);
	key = get_key(avatar, key_index);
	if (!key) {
		rc = EBADKEY;
		goto error;
	}
	m.sk_gate_id = key->k_id;
	m.sk_node_no = key->k_node;
	m.sk_length  = key->k_length;
	m.sk_type    = key->k_type;
	rc = copy_to_user(umsg, &m, sizeof(msg_s));
	if (rc) {
		goto error;
	}
error:
out_stat_key(avatar, &m, rc);
	return rc;
}

static int msg_receive (
	avatar_s	*avatar,
	char __user	*msg)
{
in_receive(avatar);
	return kreceive(avatar, msg);
}

static ssize_t msg_file_read (
	struct file	*file,
	char __user	*msg,
	size_t		count,
	loff_t		*ppos)
{
	avatar_s	*avatar = file->private_data;
	loff_t		arg = *ppos;
	unsigned	request;
	int		rc = 0;
FN;
	request = ARG_REQUEST(arg);

	LOCK_MSG;

//pr_in(request);
	switch (request) {

	case MSG_CALL:
		rc = user_call(avatar, ARG_KEY(arg), msg);
		break;

	case MSG_CHANGE_INDEX:
		rc = user_change_index(avatar, ARG_KEY(arg), msg);
		break;

	case MSG_CREATE_GATE:
		rc = user_create_gate(avatar, msg);
		break;

	case MSG_DESTROY_GATE:
		rc = msg_destroy_gate(avatar, (msg_s *)msg);
		out_destroy_gate(avatar, rc);
		break;

	case MSG_DESTROY_KEY:
		rc = msg_destroy_key(avatar, ARG_KEY(arg));
		out_destroy_key(avatar, rc);
		break;

	case MSG_DUPLICATE_KEY:
		rc = msg_duplicate_key(avatar, ARG_KEY(arg), msg);
		break;

	case MSG_NAME:
		rc = msg_name(avatar, msg);
		break;

	case MSG_NODE_ID:
		rc = msg_node_id(avatar, msg);
		break;

	case MSG_NODE_DIED:
		rc = msg_node_died(avatar, ARG_KEY(arg));
		break;

	case MSG_PLUG_KEY:
		rc = msg_plug_key(avatar, ARG_KEY(arg), msg);
		out_plug_key(avatar, rc);
		break;

	case MSG_READ_DATA:
		rc = msg_read_data(avatar, ARG_KEY(arg), msg);
		break;

	case MSG_RECEIVE:
		rc = msg_receive(avatar, msg);
		break;

	case MSG_SEND:
		rc = user_send(avatar, ARG_KEY(arg), msg);
		out_send(avatar, rc);
		break;

	case MSG_STAT_KEY:
		rc = msg_stat_key(avatar, ARG_KEY(arg), msg);
		break;

	case MSG_WRITE_DATA:
		rc = msg_write_data(avatar, ARG_KEY(arg), msg);
		break;

	default:
		printk(WARNING "unknown request=%d\n", request);
		rc = -EINVAL;
		break;
	}
//pr_out(request, rc);
	UNLOCK_MSG;

	return rc;
}

avatar_s *init_msg_tau (const char *name, receive_f recvfn)
{
	avatar_s	*avatar;
FN;
	LOCK_MSG;
	avatar = msg_alloc_avatar();
	UNLOCK_MSG;
	if (!avatar) goto error;
	init_avatar(avatar, name, recvfn);

	enter_tau(avatar);

	return avatar;
error:
	return NULL;
}
EXPORT_SYMBOL(init_msg_tau);

void die_tau (avatar_s *avatar)
{
FN;
	if (!avatar) return;
	avatar->av_dieing = TRUE;
	msg_destroy_avatar(avatar);
}
EXPORT_SYMBOL(die_tau);

int call_tau (ki_t key, void *msg)
{
	msg_s		*m = msg;
	int		rc;
FN;
	LOCK_MSG;
	m->q.q_type = KERNEL_GATE;
	rc = kernel_call(get_avatar(), key, m);
	UNLOCK_MSG;
	return rc;
}
EXPORT_SYMBOL(call_tau);

int change_index_tau (ki_t key, ki_t std_key)
{
	int		rc;
FN;
	LOCK_MSG;
	rc = msg_change_index(get_avatar(), key, std_key);
	UNLOCK_MSG;
	return rc;
}
EXPORT_SYMBOL(change_index_tau);

int create_gate_tau (void *msg)
{
	msg_s		*m = msg;
	int		rc;
FN;
	LOCK_MSG;
	m->q.q_type |= KERNEL_GATE;
	rc = create_gate(get_avatar(), m);
	UNLOCK_MSG;
	return rc;
}
EXPORT_SYMBOL(create_gate_tau);

int destroy_gate_tau (u64 id)
{
	int		rc;
FN;
	LOCK_MSG;
	rc = destroy_gate_by_id(get_avatar(), id);
	UNLOCK_MSG;
	return rc;
}
EXPORT_SYMBOL(destroy_gate_tau);

int destroy_key_tau (ki_t key)
{
	int		rc;
FN;
	LOCK_MSG;
	rc = msg_destroy_key(get_avatar(), key);
	UNLOCK_MSG;
	return rc;
}
EXPORT_SYMBOL(destroy_key_tau);

int getdata_tau (ki_t key, void *msg, unint length, void *start)
{
	msg_s		*m = msg;
	int		rc;
FN;
	LOCK_MSG;
	m->q.q_type   = KERNEL_GATE | WRITE_DATA;
	m->q.q_start  = start;
	m->q.q_length = length;
	rc = kernel_call(get_avatar(), key, m);
	UNLOCK_MSG;
	return rc;
}
EXPORT_SYMBOL(getdata_tau);

int putdata_tau (ki_t key, void *msg, unint length, const void *start)
{
	msg_s		*m = msg;
	int		rc;
FN;
	LOCK_MSG;
	m->q.q_type   = KERNEL_GATE | READ_DATA;
	m->q.q_start  = (void *)start;
	m->q.q_length = length;
	rc = kernel_call(get_avatar(), key, m);
	UNLOCK_MSG;
	return rc;
}
EXPORT_SYMBOL(putdata_tau);

int read_data_tau (
	ki_t		key,
	unint		length,
	void		*start,
	unint		offset)
{
	data_s		data = { start, length, offset };
	avatar_s	*avatar = get_avatar();
	int		rc;
FN;
	LOCK_MSG;
in_read_data(avatar, key, &data);
	rc = rw_data(KERNEL_RW | READ_RW, avatar, key, &data);
out_read_data(avatar, rc);
	UNLOCK_MSG;
	return rc;
}
EXPORT_SYMBOL(read_data_tau);

int send_tau (ki_t key, void *msg)
{
	msg_s		*m = msg;
	avatar_s	*avatar = get_avatar();
	int		rc;
FN;
	LOCK_MSG;
	m->q.q_passed_key = 0;
	rc = kernel_send(avatar, key, m);
	out_send(avatar, rc);
	UNLOCK_MSG;
	return rc;
}
EXPORT_SYMBOL(send_tau);

int send_key_tau (ki_t key, void *msg)
{
	msg_s		*m = msg;
	avatar_s	*avatar = get_avatar();
	int		rc;
FN;
	LOCK_MSG;
	rc = kernel_send(avatar, key, m);
	out_send(avatar, rc);
	UNLOCK_MSG;
	return rc;
}
EXPORT_SYMBOL(send_key_tau);

int write_data_tau (
	ki_t		key,
	unint		length,
	const void	*start,
	unint		offset)
{
	data_s		data = { (void *)start, length, offset };
	avatar_s	*avatar = get_avatar();
	int		rc;
FN;
	LOCK_MSG;
in_write_data(avatar, key, &data);
	rc = rw_data(KERNEL_RW | WRITE_RW, avatar, key, &data);
out_write_data(avatar, rc);
	UNLOCK_MSG;
	return rc;
}
EXPORT_SYMBOL(write_data_tau);

void net_deliver_tau (packet_s *p)
{
	msgbuf_s	*mb;
FN;
	mb = alloc_mb();
	if (!mb) {
		return;
	}
	memcpy( &mb->mb_packet, p, sizeof(*p) + BODY_SIZE);
	LOCK_MSG;
	deliver(mb);
	UNLOCK_MSG;
}
EXPORT_SYMBOL(net_deliver_tau);

void net_go_public_tau (packet_s *p)
{
FN;
// Need to worry about self - don't want to create one for ourselves

	p->pk_key = Network_key;
	net_deliver_tau(p);
}
EXPORT_SYMBOL(net_go_public_tau);


static int msg_open (struct inode *inode, struct file *file)
{
	avatar_s	*avatar;
	int	rc;
FN;
	LOCK_MSG;
	avatar = msg_alloc_avatar();
	if (!avatar) {
		rc = -ENOMEM;
		goto error;
	}
	init_avatar(avatar, "<none>", NULL);

	inode->i_size    = 0x7fffffff;	// Make max number of keys

	file->private_data = avatar;
	UNLOCK_MSG;

	return 0;
error:
	UNLOCK_MSG;
	return rc;
}

static int msg_release (struct inode *inode, struct file *file)
{
	avatar_s	*avatar = file->private_data;

FN;
	LOCK_MSG;
#if 0
void close (process_t *p)
{
	foreach gate
		while usecount != 0
			wait for use count
	free ch_keys
}
#endif
	avatar->av_dieing = TRUE;

	INIT_WORK( &avatar->av_work, msg_destroy_avatar, (void *)avatar);
	queue_work(Msg_workqueue, &avatar->av_work);

	UNLOCK_MSG;
	return 0;
}

struct file_operations Msg_file_operations = {
	.llseek		= generic_file_llseek,
	.read		= msg_file_read,
	.open		= msg_open,
	.release	= msg_release,
};

static struct miscdevice Msg_device = {
	MISC_DYNAMIC_MINOR, DEV_NAME, &Msg_file_operations
};

static void msg_cleanup (void)
{
	extern void stop_alarm (void);
	extern void stop_bicho (void);

	stop_alarm();
	stop_bicho();

	destroy_avatar_pool();
	destroy_gate_pool();
	destroy_msg_pool();
	destroy_msg_workqueue();
}

static void msg_exit (void)
{
	misc_deregister( &Msg_device);
	msg_cleanup();
}

static int msg_init (void)
{
	int	rc;

FN;
	msg_init_spinlock( &Msg_spinlock, "*** Msg_spinlock ***");

	rc = init_msg_workqueue();
	if (rc) goto error;

	rc = init_avatar_pool();
	if (rc) goto error;

	rc = init_gate_pool();
	if (rc) goto error;

	rc = init_msg_pool();
	if (rc) goto error;

	rc = misc_register( &Msg_device);
	if (rc < 0) goto error;

	return 0;

error:
	printk(KERN_INFO "msg loaded failed!\n");
	msg_cleanup();
	return rc;
}


module_init(msg_init)
module_exit(msg_exit)

