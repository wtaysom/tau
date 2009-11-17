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
 * Msg.h defines the structures and interfaces the users of
 * the nu messaging system need.
 */
#ifndef _TAU_MSG_H_
#define _TAU_MSG_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

enum {	DESTROYED	= 1000,	/* Destruction notification */
	EBADKEY		= 1001,	/* Key is invalid */
	EINUSE		= 1002, /* Std key index is in use */
	ENOMSGS		= 1003,	/* Out of message buffers, abort (crash) */
	EBADGATE	= 1004,	/* Bad gate type */
	ENOTSTD		= 1005, /* Not a std key index */
	ENODUP		= 1006,	/* Can't duplicate key */
	EBADADDR	= 1007, /* Badd address */
	EBADID		= 1008, /* Bad gate id */
	EBADINDEX	= 1009, /* Bad key index */
	ECANTPASS	= 1010, /* Can't pass key on this key */
	EREALLYBAD	= 1011, /* A really bad thing happened, should
				 * probably crash */
	EBROKEN		= 1012, /* Gate is broken */
	EDATA		= 1013, /* Error data area */
	ETOOBIG		= 1014, /* Length too big for data operation */
};

enum {	BODY_SIZE  = 64,
	TAU_NAME   = 32,
	STD_KEYS   = 4,		/* Key indicies 1 thru STD_KEYS are reserved */
	SW_STD_KEY = 1 };

enum {	PERM       = 1<<0,	/* Don't use this directly */
	ONCE       = 1<<1,	/* Don't use this directly */
	READ_DATA  = 1<<2,
	WRITE_DATA = 1<<3,
	PASS_REPLY = 1<<4,
	PASS_OTHER = 1<<5,
	PASS_ANY   = PASS_REPLY | PASS_OTHER,
	RW_DATA    = READ_DATA | WRITE_DATA};

enum {	REPLY      = ONCE,
	RESOURCE   = 0,
	REQUEST    = PERM};

enum {	PLUG_CLIENT,
	PLUG_NETWORK};

#ifdef __LP64__
	#define PADDING(_x)
	#define ZERO_PADDING(_m, _x)	((void)0)
#else
	#define PADDING(_x)		u32	q_padding ## _x
	#define ZERO_PADDING(_m, _x)	((((sys_s *)_m)->q_padding ## _x) = 0)
#endif

typedef u32	ki_t;	/* key index, the kernel has already taken key_t */

typedef struct sys_s {
	void	*q_tag;
	PADDING(tag);
	ki_t	q_passed_key;
	u16	q_type;
	u16	q_unused;
	void	*q_start;
	PADDING(start);
	u64	q_length;
	u64	q_offset;
} sys_s;

/* I don't like this, but it makes things uniform */
#define METHOD_S	struct {					\
				u8	m_method;			\
				u8	m_response;			\
				u8	m_reserved[sizeof(u64)-2]; }

typedef union body_u {
	METHOD_S;

	u8	b_body[BODY_SIZE];

} body_u;

typedef struct msg_s {
	sys_s	q;
	union {
		METHOD_S;
		body_u	b;

		struct {
			METHOD_S;
			char	my_name[TAU_NAME];
		};
		struct {	/* Returned by create_gate_tau */
			u64	cr_id;
		};
		struct {	/* Returned by stat_key_tau */
			u64	sk_gate_id;
			u64	sk_node_no;
			u64	sk_length;
			u64	sk_type;
		};
	};
} msg_s;

enum { CHECK_MSG_SIZE = 1/(sizeof(msg_s) == (sizeof(sys_s) + sizeof(body_u))) };

#ifdef __KERNEL__

	#ifndef _LINUX_MODULE_H
		#include <linux/module.h>
	#endif

	#ifndef _TAU_DEBUG_H_
		#include <tau/debug.h>
	#endif

	typedef struct avatar_s	avatar_s;
	typedef void	(*receive_f)(int rc, void *msg);

	avatar_s *init_msg_tau (const char *name, receive_f recvfn);
	void die_tau (avatar_s *avatar);

	static inline avatar_s *get_avatar (void)
	{
		avatar_s	*avatar = current->journal_info;

		assert(avatar);
		return avatar;
	}

	static inline avatar_s *peek_avatar (void)
	{
		avatar_s	*avatar = current->journal_info;

		return avatar;
	}

	static inline void enter_tau (avatar_s *avatar)
	{
		current->journal_info = (void *)avatar;
		//XXX: need to check state and block if engaged
		// have to consider synchronous operations like call
	}

	static inline avatar_s *exit_tau (void)
	{
		avatar_s	*avatar = current->journal_info;

		current->journal_info = NULL;
		return avatar;
	}
#else
	int init_msg_tau (const char *name);
#endif

int call_tau          (ki_t key, void *msg);
int change_index_tau  (ki_t key, ki_t std_key);
int create_gate_tau   (void *msg);
int destroy_gate_tau  (u64 id);
int destroy_key_tau   (ki_t key);
int duplicate_key_tau (ki_t key, void *msg);
int my_node_id_tau    (void *m);
int plug_key_tau      (unint index, void *msg);
int receive_tau       (void *msg);
int send_tau          (ki_t key, void *msg);
int send_key_tau      (ki_t key, void *msg);
int getdata_tau       (ki_t key, void *msg, unint length,       void *start);
int putdata_tau       (ki_t key, void *msg, unint length, const void *start);
int read_data_tau     (ki_t key, unint length,       void *start, unint offset);
int write_data_tau    (ki_t key, unint length, const void *start, unint offset);
int stat_key_tau      (ki_t key, void *msg);
int node_died_tau     (u64 node_no);

#endif
