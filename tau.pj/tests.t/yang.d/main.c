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
#include <stdio.h>
#include <stdlib.h>

#include <tau/msg.h>
#include <tau/switchboard.h>
#include <tau/tag.h>
#include <tau/msg_util.h>
#include <tau/yy.h>

#include <style.h>
#include <debug.h>
#include <eprintf.h>
#include <crc.h>

#define	fail(fmt, arg...)	\
	eprintf("FAILED: %s<%d> " fmt, __func__, __LINE__, ## arg)

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
		if (rc) eprintf("%ld. send_tau %d", i, rc);
	}
	rc = destroy_key_tau(key);
	if (rc) eprintf("destroy_key_tau %d", rc);
}

static void read_tst (void *msg)
{
	yy_msg_s	*m = msg;
	unint		key = m->q.q_passed_key;
	unint		length = m->q.q_length;
	char		*start;
	int		rc;

	start = emalloc(length);
	if (!start) {
		eprintf("emalloc(%ld)", length);
		goto exit;
	}
	rc = read_data_tau(key, length, start, 0);

	m->yy_crc = crc64(start, length);
exit:
	if (start) free(start);
	rc = send_tau(key, m);
	if (rc) eprintf("send_tau %d", rc);
}

static void write_tst (void *msg)
{
	yy_msg_s	*m = msg;
	unint		key = m->q.q_passed_key;
	unint		length = m->q.q_length;
	char		*start;
	int		rc;

	start = emalloc(length);
	if (!start) {
		eprintf("emalloc(%ld)", length);
		goto exit;
	}
	m->yy_crc = fill(start, length);
	rc = write_data_tau(key, length, start, 0);
	if (rc) eprintf("write_data_tau failed %d", rc);

exit:
	if (start) free(start);
	rc = send_tau(key, m);
	if (rc) eprintf("send_tau %d", rc);
}

static void cleanup_tst (void *msg)
{
	eprintf("Yin has gone away");
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
	if (rc) fail("send_key_tau");
}

login_type_s	Login_type = {
	{ LOGIN_METHODS, NULL },
	login };
login_s	Login = { &Login_type.tlg_tag };

static int yang_init (void)
{
	ki_t	key;
	int	rc = 0;
FN;
	rc = init_msg_tau(getprogname());
	if (rc) goto error;

	rc = sw_register(getprogname());
	if (rc) goto error;

	key = make_gate( &Login, REQUEST | PASS_REPLY);
	if (!key) goto error;

	rc = sw_post("yang", key);
	if (rc) goto error;

	return 0;
error:
	eprintf("yang failed initialization rc=%d", rc);
	return rc;
}

int main (int argc, char *argv[])
{
	setprogname(argv[0]);

	debugon();
	fdebugon();

	yang_init();

	tag_loop();

	return 0;
}
