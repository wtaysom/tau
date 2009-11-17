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

#ifndef _MSG_INTERNAL_H_
#define _MSG_INTERNAL_H_ 1

#ifndef _LINUX_FS_H
#include <linux/fs.h>
#endif

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

#ifndef _TAU_MSG_SYS_H_
#include <tau/msg_sys.h>
#endif

#ifndef _TAU_DEBUG_H_
#include <tau/debug.h>
#endif

#ifndef _TAU_NET_H_
#include <tau/net.h>
#endif

#ifndef _Q_H_
#include <q.h>
#endif

#define OFFSET_MASK	~PAGE_CACHE_MASK

enum {	MAX_KEYS = 1000,
	CHECK_MAX_KEYS = 1 / (MAX_KEYS >= STD_KEYS+3),
	MAX_DATA_PAGES = 9};

enum {	KERNEL_GATE = PASS_OTHER<<1,
	DATA_GATE   = READ_DATA | WRITE_DATA,
	VALID_GATE  = PERM | ONCE | DATA_GATE | PASS_ANY | KERNEL_GATE};

enum {	NET_VERSION = 2};

enum {	READ_RW   = 0,
	WRITE_RW  = 1,
	USER_RW   = 0,
	KERNEL_RW = 2 };

enum reply_states { REPLY_STARTING, REPLY_WAITING, REPLY_FINISHED };

#ifndef _ADDR_
	#define _ADDR_ 1
	typedef unsigned long   addr;
#endif
typedef struct super_s		super_s;
typedef struct gate_s		gate_s;
typedef struct datagate_s	datagate_s;
typedef struct replygate_s	replygate_s;

typedef struct msg_spinlock_s {
	spinlock_t		msg_spinlock;
	struct task_struct	*msg_process;
	char			*msg_where;
	char			*msg_name;
	unsigned long		msg_flags;
} msg_spinlock_s;

typedef struct key_chain_s {
	key_s	*kc_top;
	key_s	kc_chain[MAX_KEYS];
} key_chain_s;
	

struct super_s {
	struct list_head	sp_avatars;
};

#define GATE_S {							\
	datagate_s	*gt_hash;					\
	avatar_s	*gt_avatar;					\
	u64		gt_id;						\
	dqlink_t	gt_siblings;					\
	void		*gt_tag;					\
	u32		gt_usecnt;	/* can only be 0, 1, 2 */	\
	u8		gt_type;					\
	u8		gt_reserved;					\
	u16		gt_node;					\
}

#define DATA_S {							\
	struct		GATE_S;						\
	addr		dg_start;					\
	u64		dg_length;					\
	u32		dg_nr_pages;					\
	u32		dg_page_offset;					\
	struct page	*dg_pages[0];					\
}

struct gate_s		GATE_S;
struct datagate_s	DATA_S;	

struct replygate_s {
	struct		DATA_S;
	struct page	*rg_pages[MAX_DATA_PAGES];
};

enum { AVATAR_MAGIC = 0x726174617661 };

struct avatar_s {
	u64			av_magic;
	char			av_name[TAU_NAME];
	dqlink_t		av_link;
	u64			av_id;
	u64			av_inuse;
	u8			av_dieing;
	u8			av_reserved[7];
	void			(*av_kick)(avatar_s *);
	d_q			av_gates;
	msg_spinlock_s		av_spinlock;
	d_q			av_waiters;
	d_q			av_msgq;
	receive_f		av_recvfn;
	const char		*av_last_op;
	struct work_struct	av_work;
	ki_t			av_sw_key;
	key_chain_s		av_chain;
};

typedef struct msgbuf_s {
	dqlink_t	mb_avatar;
	packet_s	mb_packet;
	body_u		mb_body;
} msgbuf_s;

typedef struct reply_s {
	enum reply_states	rp_state;
	struct task_struct	*rp_task;
	msgbuf_s		*rp_mb;
	msgbuf_s		rp_msgbuf;
	replygate_s		rp_gate;
} reply_s;

typedef struct Inst_s {
	u64	msgs_queued;
	u64	msgs_dequeued;
	u64	avatars_created;
	u64	avatars_destroyed;
	u64	gates_created;
	u64	gates_destroyed;
	u64	msg_alloc;
	u64	msg_free;
} Inst_s;

#define TYPE			(ONCE | PERM)
#define RESOURCE_TYPE(_t)	(((_t) & TYPE) == 0)
#define REPLY_TYPE(_t)		(((_t) & TYPE) == ONCE)
#define REQUEST_TYPE(_t)	(((_t) & TYPE) == PERM)
#define PERM_REPLY_TYPE(_t)	(((_t) & TYPE) == (ONCE | PERM))

#define MAKE_KEY(_k, _gate)	{			\
	(_k).k_type = (_gate)->gt_type;			\
	(_k).k_id   = (_gate)->gt_id;			\
	(_k).k_node = 0;				\
	if ((_k).k_type & DATA_GATE) {			\
		(_k).k_length = (_gate)->dg_length;	\
	} else {					\
		(_k).k_length = 0;			\
	}						\
}

#if 1
	extern msg_spinlock_s	Msg_spinlock;

	#define LOCK_MSG	(msg_spinlock( &Msg_spinlock, WHERE))
	#define UNLOCK_MSG	(msg_spinunlock( &Msg_spinlock, WHERE))
	#define LOCKED_MSG	(msg_locked( &Msg_spinlock, WHERE))
	#define UNLOCKED_MSG	(msg_unlocked( &Msg_spinlock, WHERE))
#else
	#define LOCK_MSG	((void)0)
	#define UNLOCK_MSG	((void)0)
	#define LOCKED_MSG	((void)0)
#endif

extern Inst_s	Inst;

extern struct workqueue_struct	*Msg_workqueue;


void msg_init_spinlock (msg_spinlock_s *lock, char *name);
void msg_spinlock      (msg_spinlock_s *lock, char *where);
void msg_spinunlock    (msg_spinlock_s *lock, char *where);
void msg_locked        (msg_spinlock_s *lock, char *where);
void msg_unlocked      (msg_spinlock_s *lock, char *where);

void   init_key_chain (avatar_s *avatar);
ki_t   new_key        (avatar_s *avatar, key_s *passed_key);
int    assign_std_key (avatar_s *avatar, ki_t std_key_index, key_s *key);
void   delete_key     (avatar_s *avatar, key_s *key);
key_s *get_key        (avatar_s *avatar, ki_t key_index);
int    read_key       (avatar_s *avatar, ki_t key_index, key_s *buf);

int     init_gate_pool    (void);
void    destroy_gate_pool (void);
void    add_gate          (datagate_s *gate);
void    release_gate      (datagate_s *gate);
void    free_gate         (datagate_s *gate);
int     read_gate         (u64 id, datagate_s *buf);
datagate_s *get_gate      (key_s *key);
datagate_s *find_gate     (u64 key_id);
datagate_s *lookup_gate   (u64 key_id);

int  init_reply_gate (sys_s *q, avatar_s *avatar, datagate_s *gate, void *tag);
int  wait_reply (reply_s *reply);
void wake_reply (reply_s *reply);

int new_gate (
	avatar_s	*avatar,
	unint		type,
	void		*tag,
	void		*start,
	u64		length,
	datagate_s	**pgate);

int map_data (
	struct task_struct	*process,
	datagate_s	*gate,
	void		*start,
	unsigned	nr_pages,
	unint		type);

void msg_put_user_pages (
	struct page	**pages,
	int		nr_pages);


msgbuf_s *alloc_mb (void);
void     free_mb   (msgbuf_s *mb);
void     kick      (avatar_s *avatar);
int      wait      (avatar_s *avatar);
void     deliver   (msgbuf_s *mb);


void mod_put (void);
void mod_get (void);

int rw_data (
	unint		flags,
	avatar_s	*avatar,
	unint		key_index,
	data_s		*data);

extern d_q	Avatars;

#endif

