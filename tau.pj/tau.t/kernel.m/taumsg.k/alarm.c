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

#include <linux/kernel.h>
#include <linux/workqueue.h>

#include <tau/msg.h>
#include <tau/switchboard.h>
#include <tau/tag.h>
#include <tau/alarm.h>

#include <msg_debug.h>
#include <msg_internal.h>
#include <util.h>

typedef struct alarm_sw_type_s {
	type_s		tasw_tag;
	method_f	tasw_create;
} alarm_sw_type_s;

typedef struct alarm_sw_s {
	alarm_sw_type_s	*asw_type;
} alarm_sw_s;

static avatar_s	*Alarm_avatar;

static void create_alarm (void *msg);
alarm_sw_type_s	Alarm_sw_type = {
				{ "Alarm_sw", ALARM_SW_NUM, NULL },
				create_alarm };

alarm_sw_s	Alarm_sw = { &Alarm_sw_type };	

typedef struct alarm_type_s {
	type_s		ta_tag;
	method_f	ta_start;
} alarm_type_s;

typedef struct alarm_s {
	alarm_type_s		*a_type;
	struct work_struct	a_work;
	unint			a_key;
	bool			a_stop;
	u32			a_time;
} alarm_s;

static void destroy_alarm (void *msg);
static void start_cyclic  (void *msg);
alarm_type_s	Alarm_type = {
			{ "Alarm", ALARM_NUM, destroy_alarm },
			start_cyclic };

unint	Num_alarms = 0;
bool	Stop_alarm = FALSE;

static void free_alarm (alarm_s *a)
{
FN;
	destroy_key_tau(a->a_key);
	free_tau(a);
	--Num_alarms;
	if (Stop_alarm && (Num_alarms == 0)) {
		//XXX: finish shut down
	}
}

static void pop_cyclic (void *alarm)
{
	alarm_s	*a = alarm;
	msg_s	msg;
	int	rc;
FN;
	enter_tau(Alarm_avatar);
	if (Stop_alarm || a->a_stop) {
		free_alarm(a);
		goto exit;
	}
	msg.m_method = ALARM_POP;
	rc = send_tau(a->a_key, &msg);
	if (rc) {
		printk("send failed %d", rc);
		destroy_key_tau(a->a_key);
		goto exit;
	}
	schedule_delayed_work( &a->a_work, a->a_time);
exit:
	exit_tau();
}

static void destroy_alarm (void *msg)
{
	alarm_msg_s	*m = msg;
	alarm_s		*alarm = m->q.q_tag;
FN;
	alarm->a_stop = TRUE;
}	

static void start_cyclic (void *msg)
{
	alarm_msg_s	*m = msg;
	alarm_s		*alarm = m->q.q_tag;
FN;
	alarm->a_key = m->q.q_passed_key;
	alarm->a_stop = FALSE;
	alarm->a_time = (m->al_msec * HZ / 1000);
	
	INIT_WORK( &alarm->a_work, pop_cyclic, alarm);

	schedule_delayed_work( &alarm->a_work, alarm->a_time);
}	

static void create_alarm (void *msg)
{
	alarm_msg_s	*m = msg;
	alarm_s		*alarm;
	ki_t		reply = m->q.q_passed_key;
	ki_t		key = 0;
	int		rc;
FN;
	alarm = zalloc_tau(sizeof(*alarm));
	if (!alarm) {
		eprintk("Couldn't allocate space for alarm");
		goto error;
	}
	alarm->a_type = &Alarm_type;

	key = make_gate(alarm, RESOURCE | PASS_OTHER);
	if (!key) goto error;
	++Num_alarms;

	m->q.q_passed_key = key;
	rc = send_key_tau(reply, m);
	if (rc) {
		eprintk("Alarm send_key_tau error=%d", rc);
		goto error;
	}
	return;

error:
	if (key) {
		destroy_key_tau(key);
	} else if (alarm) {
		free_tau(alarm);
	}
	destroy_key_tau(reply);
}

// Need to worry about smp locks
// Need to setup local switchboard requests.
//
// May want to do what I did in Oryx/Pecos - setup a thread for this guy
// and do a normal receive.
static void alarm_receive (int rc, void *msg)
{
	msg_s		*m = msg;
	object_s	*obj;
	type_s		*type;
FN;
	obj = m->q.q_tag;
if (!obj) bug("tag is null");
	type  = obj->o_type;
if (!type) bug("type is null %p", obj);
	if (!rc) {
		if (m->m_method < type->ty_num_methods) {
			type->ty_method[m->m_method](m);
			return;
		}
		printk(KERN_INFO "bad method %u >= %u\n",
			m->m_method, type->ty_num_methods);
		if (m->q.q_passed_key) {
			destroy_key_tau(m->q.q_passed_key);
		}
		return;	
	}
	if (rc == DESTROYED) {
		if (type->ty_destroy) type->ty_destroy(m);
		return;
	}
	printk(KERN_INFO "alarm_receive err = %d\n", rc);
}

void start_alarm (void *not_used)
{
	ki_t	key;
	int	rc;
FN;
	Alarm_avatar = init_msg_tau("alarm_drv", alarm_receive);
	if (!Alarm_avatar) {
		eprintk("init_alarm failed");
		return;
	}
	mod_put();	// Becuase alarm is part of msg module, need to
			// account for try_module_get so we can unload.

	rc = sw_register("alarm");
	if (rc) return;

	key = make_gate( &Alarm_sw, REQUEST | PASS_OTHER);
	if (!key) return;

	//XXX: Should just post this one locally.
	sw_post("alarm", key);
	exit_tau();
}

void init_alarm (void)
{
	static struct work_struct	start;
FN;
	INIT_WORK( &start, start_alarm, NULL);

	schedule_work( &start);
}

void stop_alarm (void)
{
FN;
	Stop_alarm = TRUE;
	set_current_state(TASK_INTERRUPTIBLE);
	while (Num_alarms > 0) {
		schedule_timeout(50);
	}
	if (Alarm_avatar) {
		die_tau(Alarm_avatar);
		Alarm_avatar = NULL;
	}
}
