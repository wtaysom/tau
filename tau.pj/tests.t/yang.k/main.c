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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>

#include <style.h>
#include <crc.h>
#include <tau/msg.h>
#include <tau/switchboard.h>
#include <tau/debug.h>
#include <tau/tag.h>
#include <tau/msg_util.h>
#include <tau/yy.h>

avatar_s	*Yang;

typedef struct login_type_s {
	type_s		tlg_tag;
	method_f	tlg_login;
} login_type_s;

typedef struct login_s {
	type_s		*lg_tag;
} login_s;

typedef struct test_type_s {
	type_s		tt_tag;
	method_f	tt_send;
	method_f	tt_read;
	method_f	tt_write;
} test_type_s;

typedef struct test_s {
	test_type_s	*t_type;
} test_s;

static u64 fill (void *s, unint n)
{
	u8	*p = s;
	unint	i;

	for (i = 0; i < n; i++) {
		*p++ = i;
	}
	return crc64(s, n);
}

static void send_tst (void *msg)
{
	yy_msg_s	*m = msg;
	unint		key = m->q.q_passed_key;
	unint		n = m->yy_num;
	unint		i;
	int		rc;

	for (i = 0; i < n; i++) {
		rc = send_tau(key, m);
		if (rc) eprintk("%ld. send_tau %d", i, rc);
	}
	rc = destroy_key_tau(key);
	if (rc) eprintk("destroy_key_tau %d", rc);
}

static void read_tst (void *msg)
{
	yy_msg_s	*m = msg;
	unint		key = m->q.q_passed_key;
	unint		length = m->q.q_length;
	char		*start;
	int		rc;

	start = kmalloc(length, GFP_KERNEL);
	if (!start) {
		eprintk("kmalloc(%ld)", length);
		goto exit;
	}
	rc = read_data_tau(key, length, start, 0);

	m->yy_crc = crc64(start, length);
exit:
	if (start) kfree(start);
	rc = send_tau(key, m);
	if (rc) eprintk("send_tau %d", rc);
}

static void write_tst (void *msg)
{
	yy_msg_s	*m = msg;
	unint		key = m->q.q_passed_key;
	unint		length = m->q.q_length;
	char		*start;
	int		rc;

	start = kmalloc(length, GFP_KERNEL);
	if (!start) {
		eprintk("kmalloc(%ld)", length);
		goto exit;
	}
	m->yy_crc = fill(start, length);
	rc = write_data_tau(key, length, start, 0);

exit:
	if (start) kfree(start);
	rc = send_tau(key, m);
	if (rc) eprintk("send_tau %d", rc);
}

static void cleanup_tst (void *msg)
{
	eprintk("Yin has gone away");
}

static test_type_s	Test_type = { {TEST_METHODS, cleanup_tst },
					send_tst,
					read_tst,
					write_tst};
static test_s		Tests = { &Test_type };

static void login (void *msg)
{
	msg_s	*m = msg;
	unint	reply = m->q.q_passed_key;
	int	rc;
FN;
	m->q.q_passed_key = make_gate( &Tests, RESOURCE | PASS_ANY);

	rc = send_key_tau(reply, msg);
	if (rc) eprintk("send_key_tau %d", rc);
}

static login_type_s	Login_type = { { LOGIN_METHODS, NULL },
				login };
static login_s	Login = { &Login_type.tlg_tag };

static void yang_receive (int rc, void *msg)
{
	msg_s		*m = msg;
	object_s	*obj;
	type_s		*type;
ENTER;
	obj = m->q.q_tag;
if (!obj) bug("tag is null");
	type  = obj->o_type;
if (!type) bug("type is null %p", obj);
PRd(m->q.q_length);
	if (!rc) {
		if (m->m_method < type->ty_num_methods) {
			type->ty_method[m->m_method](m);
			vRet;
		}
		printk(KERN_INFO "bad method %u >= %u\n",
			m->m_method, type->ty_num_methods);
		if (m->q.q_passed_key) {
			destroy_key_tau(m->q.q_passed_key);
		}
		vRet;
	}
	if (rc == DESTROYED) {
		if (type->ty_destroy) type->ty_destroy(m);
		vRet;
	}
	printk(KERN_INFO "kcache_receive err = %d\n", rc);
vRet;
}

static void yang_cleanup (void)
{
FN;
	if (Yang) die_tau(Yang);
EXIT;
}

static int yang_init (void)
{
	unint	key;
	int	rc = 0;
ENTER;
	Yang = init_msg_tau("yang", yang_receive);
	if (!Yang) goto error;

	rc = sw_register("yang");
	if (rc) goto error;

	key = make_gate( &Login, REQUEST | PASS_REPLY);
	if (!key) goto error;

	rc = sw_post("yang", key);
	if (rc) goto error;

	iRet(0);
error:
	printk(KERN_INFO "yang failed initialization\n");
	yang_cleanup();
	iRet(rc);
}

static void yang_exit (void)
{
	yang_cleanup();
	printk(KERN_INFO "yang un-loaded\n");
}

MODULE_AUTHOR("Paul Taysom");
MODULE_DESCRIPTION("yang");
MODULE_LICENSE("GPL");

module_init(yang_init)
module_exit(yang_exit)

