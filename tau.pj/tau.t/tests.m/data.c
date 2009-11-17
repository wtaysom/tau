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
#include <unistd.h>

#include <style.h>
#include <eprintf.h>
#include <debug.h>

#include <tau/msg.h>
#include <tau/switchboard.h>
#include <tau/msg_util.h>

#include "data.h"

/* Data */
enum {	MAX_DATA = 1000000 };

typedef struct data_s {
	void	(*d_fn)(struct data_s *);
	u64	d_num_bytes;
	u64	d_seed;
	u8	d_data[0];
} data_s;

int failure (char *err, int rc)
{
	eprintf("FAILURE: %s rc=%d\n", err, rc);
	return 0;
}

int Ignore_prompt = FALSE;
void ignore_prompt (void)
{
	Ignore_prompt = TRUE;
}

void prompt (char *s)
{
	static int	cnt = 0;

	++cnt;
	printf("data:%s %d> ", s, cnt);
	if (Ignore_prompt) {
		printf("\n");
	} else {
		getchar();
	}
}

static int check_data (u8 *buf, unsigned length, unsigned seed)
{
	unsigned	i;
	u8		x;

	srandom(seed);

	for (i = 0; i < length; i++) {
		x = random();
		if (buf[i] != x) {
			weprintf("check failed at %d %x!=%x\n",
				i, buf[i], x);
			prompt("failure:");
			return FALSE;
		}
	}
	return TRUE;
}

static void fill_data (u8 *buf, unsigned length, unsigned seed)
{
	unsigned	i;
	u8		x;

	srandom(seed);

	for (i = 0; i < length; i++) {
		x = random();
		buf[i] = x;
	}
}

static void finish_reader (data_s *data)
{
	free(data);
}

static void reader_data (data_msg_s *msg)
{
	unsigned	reader_key = msg->q.q_passed_key;
	data_s		*data;
	int		rc;

	if (msg->dt_num_bytes > MAX_DATA) {
		failure("data area too big", msg->dt_num_bytes);
	}
	data = emalloc(sizeof(*data) + msg->dt_num_bytes);
PRp(data);
	data->d_num_bytes = msg->dt_num_bytes;
	data->d_seed      = msg->dt_seed;
	data->d_fn        = finish_reader;
	fill_data(data->d_data, data->d_num_bytes, data->d_seed);
	msg->q.q_passed_key = make_datagate(data, REPLY | READ_DATA,
				data->d_data, data->d_num_bytes);

	rc = send_key_tau(reader_key, msg);
	if (rc) failure("send_key_tau", rc);
}

static void finish_writer (data_s *data)
{
	check_data(data->d_data, data->d_num_bytes, data->d_seed);
	free(data);
}

static void writer_data (data_msg_s *msg)
{
	unsigned	writer_key = msg->q.q_passed_key;
	data_s		*data;
	int		rc;

	if (msg->dt_num_bytes > MAX_DATA) {
		failure("data area too big", msg->dt_num_bytes);
	}
	data = emalloc(sizeof(*data) + msg->dt_num_bytes);
	data->d_num_bytes = msg->dt_num_bytes;
	data->d_seed      = msg->dt_seed;
	data->d_fn        = finish_writer;
	msg->q.q_passed_key = make_datagate(data, REPLY | WRITE_DATA,
				data->d_data, data->d_num_bytes);

	rc = send_key_tau(writer_key, msg);
	if (rc) failure("send_key_tau", rc);
}

void usage (void)
{
	fprintf(stderr, "usage: %s [-?i]\n"
		"\t-?  this message\n"
		"\t-i  ignore prompt\n",
 		getprogname());
	exit(1);
}

int main (int argc, char *argv[])
{
	data_msg_s	msg;
	ki_t		key;
	data_s		*data;
	int		c;
	int		rc;

	setprogname(argv[0]);

	while ((c = getopt(argc, argv, "i")) != -1) {
		switch (c) {
		case 'i':
			ignore_prompt();
			break;
		default:
			usage();
			break;
		}
	}

prompt("start");
	if (init_msg_tau(getprogname())) {
		exit(2);
	}
prompt("sw_register");
	sw_register(argv[0]);

prompt("create_gate");
	key = make_gate(0, REQUEST | PASS_REPLY);

prompt("sw_post");
	rc = sw_post("data", key);
	if (rc) failure("sw_post", rc);

	for (;;) {
prompt("receive");
		rc = receive_tau( &msg);
		if (rc) {
			if (rc == DESTROYED) {
				data = msg.q.q_tag;
				if (data) {
					data->d_fn(data);
				}
			} else {
				failure("receive", rc);
			}
			continue;
		}
		switch (msg.m_method) {
		case TST_READ_DATA:
			reader_data( &msg);
			break;
		case TST_WRITE_DATA:
			writer_data( &msg);
			break;
		default:
			failure("unknown method", msg.m_method);
			break;
		}
	}
}
