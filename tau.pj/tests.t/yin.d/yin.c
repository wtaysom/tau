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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <style.h>
#include <crc.h>
#include <tau/msg.h>
#include <tau/switchboard.h>
#include <tau/msg_util.h>
#include <eprintf.h>
#include <cmd.h>
#include <tau/yy.h>
#include <yin.h>

#define	fail(fmt, arg...)	\
	eprintf("FAILED: %s<%d> " fmt "\n", __func__, __LINE__, ## arg)

unsigned Yang_key;

static u64 fill (void *s, unint n)
{
	u8	*p = s;
	unint	i;

	for (i = 0; i < n; i++) {
		*p++ = random();
	}
	return crc64(s, n);
}

static bool check (void *s, unint n, u64 crc)
{
	u64	x;

	x = crc64(s, n);
	if (x != crc) {
		fail("crc %lld != %lld\n", x, crc);
		return FALSE;
	}
	return TRUE;
}

int ylookup (char *path, u64 *id)
{
	yy_msg_s	m;
	int		rc;
	unint		size;

	size = strlen(path) + 1;

	m.m_method = LOOKUP;
	rc = putdata_tau(Yang_key, &m, size, path);
	if (rc) fail("putdata %d", rc);
	*id = m.yy_id;
	return 0;
}

void ylogin (void)
{
	msg_s		m;
	unsigned	key;
	int		rc;

	if (init_msg_tau(getprogname())) {
		exit(2);
	}
	sw_register(getprogname());

	rc = sw_any("yang", &key);
	if (rc) eprintf("sw_any %d", rc);

	m.m_method = LOGIN;
	rc = call_tau(key, &m);
	if (rc) fail("call_tau");
	destroy_key_tau(key);

	Yang_key = m.q.q_passed_key;
}

static int sendp (int argc, char *argv[])
{
	yy_msg_s	m;
	unint		n = 1;
	unint		i;
	int		rc;

	if (argc > 1) {
		n = atoi(argv[1]);
	}
	m.q.q_passed_key = make_gate(0, REQUEST);
	m.m_method = SEND_TST;
	m.yy_num = n;
	rc = send_key_tau(Yang_key, &m);
	if (rc) eprintf("send_key %d", rc);

	for (i = 0; i < n; i++) {
		rc = receive_tau( &m);
		if (rc) eprintf("receive_tau %d", rc);
	}
	return 0;
}

static int rdp (int argc, char *argv[])
{
	yy_msg_s	m;
	char		*start;
	unint		length = 37;
	u32		crc;
	int		rc;

	if (argc > 1) {
		length = atoi(argv[1]);
	}
	start = emalloc(length);
	crc = fill(start, length);
	m.q.q_passed_key = make_datagate(0, REPLY | READ_DATA,
					start, length);
	m.m_method = READ_TST;
	rc = send_key_tau(Yang_key, &m);
	if (rc) eprintf("send_key_tau %d", rc);

	rc = receive_tau( &m);
	if (rc) eprintf("receive_tau %d", rc);

	check(start, length, m.yy_crc);

	return 0;
}

static int wtp (int argc, char *argv[])
{
	yy_msg_s	m;
	char		*start;
	unint		length = 37;
	int		rc;

	if (argc > 1) {
		length = atoi(argv[1]);
	}
	start = emalloc(length);
	m.q.q_passed_key = make_datagate(0, RESOURCE | WRITE_DATA, start, length);
	m.m_method = WRITE_TST;
	rc = send_key_tau(Yang_key, &m);
	if (rc) eprintf("send_key_tau %d", rc);

	rc = receive_tau( &m);
	if (rc) eprintf("receive_tau %d", rc);

	check(start, length, m.yy_crc); 

	return 0;
}

void init_tst (void)
{
	CMD(send, "# send test");
	CMD(rd,   "# read data test");
	CMD(wt,   "# write data test");
}
