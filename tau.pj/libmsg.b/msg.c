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

#define _XOPEN_SOURCE 500
#include <sys/types.h>
#include <sys/uio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <tau/msg.h>
#include <tau/msg_sys.h>
#include <path.h>

enum { CHECK_TAU_NAME = 1 / (TAU_NAME < sizeof(msg_s)) };

static char	Dev_name[] = FULL_DEV_NAME;
static char	Process_name[TAU_NAME];
static int	Msg_fd;

#define RTN(_rc)	{return ((_rc) == -1) ? -errno : (_rc);}

void shutdown_msg_tau (void)
{
	close(Msg_fd);
	Msg_fd = 0;
}

int init_msg_tau (const char *name)
{
	ssize_t	rc;
	msg_s	m;

	if (Msg_fd) {
		fprintf(stderr, "Message system already initialized %d\n",
			Msg_fd);
		return EBUSY;
	}
	Msg_fd = open(Dev_name, O_RDWR);
	if (Msg_fd == -1) {
		Msg_fd = 0;
		fprintf(stderr,
			"Couldn't open %s to initalize message system %s\n",
			Dev_name, strerror(errno));
		return errno;
	}
	strncpy(Process_name, file_path(name), TAU_NAME);
	Process_name[TAU_NAME-1] = '\0';
	strncpy(m.my_name, Process_name, TAU_NAME);
	rc = pread(Msg_fd, &m, sizeof(msg_s), MSG_NAME);
	if (rc != 0) {
		shutdown_msg_tau();
	}
	RTN(rc);
}

int call_tau (ki_t key, void *msg)
{
	msg_s	*m = msg;
	ssize_t	rc;

	m->q.q_type = 0;
	rc = pread(Msg_fd, msg, sizeof(msg_s), MAKE_ARG(key, MSG_CALL));
	RTN(rc);
}

int change_index_tau (ki_t key, ki_t std_key)
{
	msg_s	m;
	ssize_t	rc;

	m.q.q_type = 0;
	m.q.q_passed_key = std_key;
	rc = pread(Msg_fd, &m, sizeof(m), MAKE_ARG(key, MSG_CHANGE_INDEX));
	RTN(rc);
}

int create_gate_tau (void *msg)
{
	ssize_t	rc;

	ZERO_PADDING(msg, tag);
	ZERO_PADDING(msg, start);
	rc = pread(Msg_fd, msg, sizeof(msg_s), MSG_CREATE_GATE);
	RTN(rc);
}

int destroy_gate_tau (u64 id)
{
	msg_s	m;
	ssize_t	rc;

	m.cr_id = id;
	rc = pread(Msg_fd, &m, sizeof(m), MSG_DESTROY_GATE);
	RTN(rc);
}

int destroy_key_tau (ki_t key)
{
	ssize_t	rc;

	rc = pread(Msg_fd, NULL, 0, MAKE_ARG(key, MSG_DESTROY_KEY));
	RTN(rc);
}

int duplicate_key_tau (ki_t key, void *msg)
{
	ssize_t	rc;

	rc = pread(Msg_fd, msg, sizeof(msg_s), MAKE_ARG(key, MSG_DUPLICATE_KEY));
	RTN(rc);
}

int getdata_tau (ki_t key, void *msg, unint length, void *start)
{
	msg_s	*m = msg;
	ssize_t	rc;

	ZERO_PADDING(msg, start);
	m->q.q_type   = WRITE_DATA;
	m->q.q_start  = start;
	m->q.q_length = length;
	rc = pread(Msg_fd, msg, sizeof(msg_s), MAKE_ARG(key, MSG_CALL));
	RTN(rc);
}

int my_node_id_tau (void *msg)
{
	ssize_t	rc;

	rc = pread(Msg_fd, msg, sizeof(msg_s), MSG_NODE_ID);
	RTN(rc);
}

int node_died_tau (u64 node_no)
{
	msg_s	msg;
	ssize_t	rc;

	rc = pread(Msg_fd, &msg, sizeof(msg_s), MAKE_ARG(node_no, MSG_NODE_DIED));
	RTN(rc);
}

int plug_key_tau (unint plug, void *msg)
{
	ssize_t	rc;

	rc = pread(Msg_fd, msg, sizeof(msg_s), MAKE_ARG(plug, MSG_PLUG_KEY));
	RTN(rc);
}

int putdata_tau (ki_t key, void *msg, unint length, const void *start)
{
	msg_s	*m = msg;
	ssize_t	rc;

	ZERO_PADDING(msg, start);
	m->q.q_type   = READ_DATA;
	m->q.q_start  = (void *)start;
	m->q.q_length = length;
	rc = pread(Msg_fd, msg, sizeof(msg_s), MAKE_ARG(key, MSG_CALL));
	RTN(rc);
}

int read_data_tau (ki_t key, unint length, void *start, unint offset)
{
	msg_s	m;
	ssize_t	rc;

	ZERO_PADDING( &m, start);
	m.q.q_start  = start;
	m.q.q_length = length;
	m.q.q_offset = offset;
	rc = pread(Msg_fd, &m, sizeof(m), MAKE_ARG(key, MSG_READ_DATA));
	RTN(rc);
}

int receive_tau (void *msg)
{
	ssize_t	rc;

	rc = pread(Msg_fd, msg, sizeof(msg_s), MSG_RECEIVE);
	RTN(rc);
}

int send_tau (ki_t key, void *msg)
{
	msg_s	*m = msg;
	ssize_t	rc;

	m->q.q_passed_key = 0;
	rc = pread(Msg_fd, msg, sizeof(msg_s), MAKE_ARG(key, MSG_SEND));
	RTN(rc);
}

int send_key_tau (ki_t key, void *msg)
{
	ssize_t	rc;

	rc = pread(Msg_fd, msg, sizeof(msg_s), MAKE_ARG(key, MSG_SEND));
	RTN(rc);
}

int stat_key_tau (ki_t key, void *msg)
{
	ssize_t	rc;

	rc = pread(Msg_fd, msg, sizeof(msg_s), MAKE_ARG(key, MSG_STAT_KEY));
	RTN(rc);
}

int write_data_tau (ki_t key, unint length, const void *start, unint offset)
{
	msg_s	m;
	ssize_t	rc;

	ZERO_PADDING( &m, start);
	m.q.q_start  = (void *)start;
	m.q.q_length = length;
	m.q.q_offset = offset;
	rc = pread(Msg_fd, &m, sizeof(m), MAKE_ARG(key, MSG_WRITE_DATA));
	RTN(rc);
}
