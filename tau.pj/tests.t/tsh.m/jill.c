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

#define _XOPEN_SOURCE 600
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <zParams.h>
#include <zPublics.h>
#include <zError.h>

#include <style.h>
#include <tau/msg.h>
#include <tau/switchboard.h>
#include <tau/msg_util.h>
#include <mylib.h>
#include <eprintf.h>
#include <utf.h>
#include <debug.h>
#include <cmd.h>

#include <jill.h>

//////////////////////////////////////////////////////////////////////////////

static void jill_register       (void *msg);
static void jill_root_key       (void *msg);
static void jill_close          (void *msg);
static void jill_new_connection (void *msg);
static void jill_create         (void *msg);
static void jill_delete         (void *msg);
static void jill_open           (void *msg);
static void jill_read           (void *msg);
static void jill_write          (void *msg);
static void jill_invalid        (void *msg);
static void jill_enumerate      (void *msg);
static void jill_get_info       (void *msg);

static struct {
	type_s		jwt_tag;
	method_f	jwt_ops[JILL_SW_OPS];
} Jill_sw_type = {{ JILL_SW_OPS, NULL }, {
			jill_register }};
static struct {
	type_s	*jw_type;
} Jill_sw = { &Jill_sw_type.jwt_tag };

typedef struct jill_type_s {
	type_s		jt_tag;
	method_f	jt_ops[JILL_OPS];
} jill_type_s;

jill_type_s Jill_type = {{ JILL_OPS, jill_close }, {
			jill_root_key,
			jill_new_connection,
			jill_create,
			jill_delete,
			jill_open,
			jill_invalid,
			jill_invalid,
			jill_invalid,
			jill_invalid
			}};

jill_type_s Jill_open = {{ JILL_OPS, jill_close }, {
			jill_root_key,
			jill_invalid,
			jill_create,
			jill_delete,
			jill_open,
			jill_read,
			jill_write,
			jill_enumerate,
			jill_get_info
			}};

typedef struct jill_s {
	type_s	*j_type;
	unint	j_task;
	unint	j_name_space;
	Key_t	j_key;
} jill_s;

void jill_invalid (void *msg)
{
	jmsg_s	*m = msg;

	warn("unexpected message %d", m->m_method);
	destroy_key_tau(m->q.q_passed_key);
}

void jill_register (void *msg)
{
	jmsg_s	*m = msg;
	ki_t	reply = m->q.q_passed_key;
	jill_s	*j;
	int	rc;

	j = ezalloc(sizeof(*j));
	j->j_type = &Jill_type.jt_tag;
	j->j_task = m->ji_task;
	j->j_name_space = m->ji_name_space;

	rc = zRootKey(0, &j->j_key);
	if (rc) {
		destroy_key_tau(reply);
		warn("zRootKey %d", rc);
		return;
	}
	m->q.q_passed_key = make_gate(j, RESOURCE | PASS_REPLY);
	rc = send_key_tau(reply, m);
	if (rc) {
		destroy_key_tau(reply);
		warn("send_key_tau %d", rc);
		return;
	}
}

void jill_root_key (void *msg)	// Same as jill_register
{
	jmsg_s	*m = msg;
	ki_t	reply = m->q.q_passed_key;
	jill_s	*j;
	int	rc;

	j = ezalloc(sizeof(*j));
	j->j_type = &Jill_type.jt_tag;
	j->j_task = m->ji_task;
	j->j_name_space = m->ji_name_space;

	rc = zRootKey(m->ji_id, &j->j_key);
	if (rc) {
		destroy_key_tau(reply);
		warn("zRootKey %d", rc);
		return;
	}
	m->q.q_passed_key = make_gate(j, RESOURCE | PASS_REPLY);
	rc = send_key_tau(reply, m);
	if (rc) {
		destroy_key_tau(reply);
		warn("send_key_tau %d", rc);
		return;
	}
}

void jill_new_connection (void *msg)
{
	jmsg_s		*m = msg;
	ki_t		reply = m->q.q_passed_key;
	jill_s		*j = m->q.q_tag;
	unicode_t	fdn[JILL_MAX_FDN];
	int		rc;

	if (m->q.q_length >= JILL_MAX_FDN) {
		warn("fdn too long %lld", m->q.q_length);
		destroy_key_tau(reply);
		return;
	}
	rc = read_data_tau(reply, m->q.q_length, fdn, 0);
	if (rc) {
		warn("read_data_tau %d", rc);
		destroy_key_tau(reply);
		return;
	}
	fdn[JILL_MAX_FDN-1] = '\0';
	rc = zNewConnection(j->j_key, fdn);
	if (rc) {
		warn("zNewConnection %d", rc);
		destroy_key_tau(reply);
		return;
	}
	rc = send_tau(reply, m);
	if (rc) {
		warn("send_tau %d", rc);
		destroy_key_tau(reply);
		return;
	}
}

void jill_create (void *msg)
{
	jmsg_s	*m = msg;
	ki_t	reply = m->q.q_passed_key;
	jill_s	*j = m->q.q_tag;
	jill_s	*jcreate;
	char	path[JILL_MAX_PATH];
	int	rc;

	if (m->q.q_length >= JILL_MAX_PATH) {
		warn("path too long %lld", m->q.q_length);
		destroy_key_tau(reply);
		return;
	}
	rc = read_data_tau(reply, m->q.q_length, path, 0);
	if (rc) {
		warn("read_data_tau %d", rc);
		destroy_key_tau(reply);
		return;
	}
	jcreate = ezalloc(sizeof(*jcreate));
	*jcreate = *j;
	jcreate->j_type = &Jill_open.jt_tag;

	rc = zCreate(j->j_key, j->j_task, zNILXID, j->j_name_space,
			path, m->ji_file_type, m->ji_file_attributes,
			m->ji_create_flags, m->ji_requested_rights,
			&jcreate->j_key);
	if (rc) {
		warn("zCreate=%d", rc);
		destroy_key_tau(reply);
		free(jcreate);
		return;
	}
	m->q.q_passed_key = make_gate(jcreate, RESOURCE | PASS_REPLY);
	if (!m->q.q_passed_key) {
		destroy_key_tau(reply);
		warn("make_gate");
		return;
	}	
	rc = send_key_tau(reply, m);
	if (rc) {
		destroy_key_tau(m->q.q_passed_key);
		destroy_key_tau(reply);
		warn("send_key_tau %d", rc);
		return;
	}
}

void jill_delete (void *msg)
{
	jmsg_s	*m = msg;
	ki_t	reply = m->q.q_passed_key;
	jill_s	*j = m->q.q_tag;
	char	path[JILL_MAX_PATH];
	int	rc;

	if (m->q.q_length >= JILL_MAX_PATH) {
		warn("path too long %lld", m->q.q_length);
		destroy_key_tau(reply);
		return;
	}
	rc = read_data_tau(reply, m->q.q_length, path, 0);
	if (rc) {
		warn("read_data_tau %d", rc);
		destroy_key_tau(reply);
		return;
	}

	rc = zDelete(j->j_key, zNILXID, j->j_name_space,
			path, 0, 0);
	if (rc) {
		warn("zDelete %d", rc);
		destroy_key_tau(reply);
		return;
	}
	rc = send_tau(reply, m);
	if (rc) {
		warn("send_tau %d", rc);
		destroy_key_tau(reply);
		return;
	}
}

void jill_open (void *msg)
{
	jmsg_s	*m = msg;
	ki_t	reply = m->q.q_passed_key;
	jill_s	*j = m->q.q_tag;
	jill_s	*jopen;
	char	path[JILL_MAX_PATH];
	int	rc;

	if (m->q.q_length >= JILL_MAX_PATH) {
		warn("path too long %lld", m->q.q_length);
		destroy_key_tau(reply);
		return;
	}
	rc = read_data_tau(reply, m->q.q_length, path, 0);
	if (rc) {
		warn("read_data_tau %d", rc);
		destroy_key_tau(reply);
		return;
	}
	
	jopen = ezalloc(sizeof(*jopen));
	*jopen = *j;
	jopen->j_type = &Jill_open.jt_tag;

	rc = zOpen(j->j_key, j->j_task, j->j_name_space,
			path, m->ji_requested_rights, &jopen->j_key);
	if (rc) {
		warn("zOpen %d", rc);
		destroy_key_tau(reply);
		free(jopen);
		return;
	}
	m->q.q_passed_key = make_gate(jopen, RESOURCE | PASS_REPLY);
	if (!m->q.q_passed_key) {
		destroy_key_tau(reply);
		warn("make_gate");
		return;
	}	
	rc = send_key_tau(reply, m);
	if (rc) {
		destroy_key_tau(reply);
		warn("send_key_tau %d", rc);
		return;
	}
}

void jill_close (void *msg)
{
	jmsg_s	*m = msg;
	jill_s	*j = m->q.q_tag;
	int	rc;

	rc = zClose(j->j_key);
	if (rc) warn("zClose=%d", rc);
	free(j);
}

void jill_enumerate (void *msg)
{
	jmsg_s	*m = msg;
	jill_s	*j = m->q.q_tag;
	ki_t	reply = m->q.q_passed_key;
	u64	cookie = m->jo_cookie;
	u64	ret_cookie;
	zInfo_s	info;
	int	rc;

	rc = zEnumerate(j->j_key, cookie, zNTYPE_FILE, 0/*match*/,
			zGET_STD_INFO | zGET_NAME, sizeof(info),
			zINFO_VERSION_A, &info, &ret_cookie);
	if (rc) {
		warn("zEnumerate %d", rc);
		destroy_key_tau(reply);
		return;
	}
	m->jo_cookie = ret_cookie;
	rc = write_data_tau(reply, sizeof(info), &info, 0);
	if (rc) {
		warn("write_data_tau %d", rc);
		destroy_key_tau(reply);
		return;
	}
	rc = send_tau(reply, m);
	if (rc) {
		warn("send_tau %d", rc);
		destroy_key_tau(reply);
	}
}

void jill_get_info (void *msg)
{
	jmsg_s		*m = msg;
	jill_s		*j = m->q.q_tag;
	ki_t		reply = m->q.q_passed_key;
	u64		mask = m->jo_info_mask;
	zInfoC_s	info;
	int		rc;

	rc = zGetInfo(j->j_key, mask,
			sizeof(info), zINFO_VERSION_C, &info);
	if (rc) {
		warn("zGetInfo %d", rc);
		destroy_key_tau(reply);
		return;
	}
	rc = write_data_tau(reply, sizeof(info), &info, 0);
	if (rc) {
		warn("write_data_tau %d", rc);
		destroy_key_tau(reply);
		return;
	}
	rc = send_tau(reply, m);
	if (rc) {
		warn("send_tau %d", rc);
		destroy_key_tau(reply);
	}
}

void jill_modify_info (void *msg)
{
	jmsg_s		*m = msg;
	jill_s		*j = m->q.q_tag;
	ki_t		reply = m->q.q_passed_key;
	u64		mask = m->jo_info_mask;
	zInfoC_s	info;
	int		rc;

	rc = read_data_tau(reply, sizeof(info), &info, 0);
	if (rc) {
		warn("readdata_tau %d", rc);
		destroy_key_tau(reply);
	}
	rc = zModifyInfo(j->j_key, 0, mask,
			sizeof(info), zINFO_VERSION_C, &info);
	if (rc) {
		warn("zModifyInfo %d", rc);
		destroy_key_tau(reply);
		return;
	}
	rc = send_key_tau(reply, m);
	if (rc) {
		warn("send_key_tau %d", rc);
		destroy_key_tau(reply);
	}
}

void jill_read (void *msg)
{
	jmsg_s	*m = msg;
	ki_t	reply = m->q.q_passed_key;
	jill_s	*j = m->q.q_tag;
	unint	bytes_to_read;
	unint	bytes_read;
	u64	total;
	u64	remainder;
	u8	buf[JILL_IO_SIZE];
	int	rc;

	total = 0;
	remainder = m->q.q_length;
	for (;;) {
		if (remainder > JILL_IO_SIZE) {
			bytes_to_read = JILL_IO_SIZE;
		} else {
			bytes_to_read = remainder;
		}
		rc = zRead(j->j_key, 0, m->jo_offset,
				bytes_to_read, buf, &bytes_read);
		if (rc) {
			warn("zRead %d", rc);
			destroy_key_tau(reply);
			return;
		}
		if (bytes_read == 0) {	// At EOF
			break;
		}
		rc = write_data_tau(reply, bytes_read, buf, total);
		if (rc) {
			warn("write_data %d", rc);
			destroy_key_tau(reply);
			return;
		}
		total += bytes_read;
		if (bytes_read < bytes_to_read) {
			break;
		}
		remainder -= bytes_read;
	}
	m->jo_length = total;
	rc = send_tau(reply, m);
	if (rc) {
		warn("send_tau %d", rc);
		destroy_key_tau(reply);
	}
}

void jill_write (void *msg)
{
	jmsg_s	*m = msg;
	ki_t	reply = m->q.q_passed_key;
	jill_s	*j = m->q.q_tag;
	unint	bytes_to_write;
	unint	bytes_written;
	u64	total;
	u64	remainder;
	u8	buf[JILL_IO_SIZE];
	int	rc;

	total = 0;
	remainder = m->q.q_length;
	for (;;) {
		if (remainder > JILL_IO_SIZE) {
			bytes_to_write = JILL_IO_SIZE;
		} else {
			bytes_to_write = remainder;
		}
		rc = read_data_tau(reply, bytes_to_write, buf, total);
		if (rc) {
			warn("read_data %d", rc);
			destroy_key_tau(reply);
			return;
		}
		rc = zWrite(j->j_key, 0, m->jo_offset,
				bytes_to_write, buf, &bytes_written);
		if (rc) {
			warn("zWrite %d", rc);
			destroy_key_tau(reply);
			return;
		}
		if (bytes_to_write != bytes_written) {
			warn("bytes_to_write %lld != %lld bytes_written",
				bytes_to_write, bytes_written);
			destroy_key_tau(reply);
			return;
		}
		total += bytes_written;
		remainder -= bytes_written;
		if (!remainder) break;
	}
	m->jo_length = total;
	rc = send_tau(reply, m);
	if (rc) {
		warn("send_tau %d", rc);
		destroy_key_tau(reply);
	}
}

void jill_init (void)
{
	int	rc;
	ki_t	key;

	if (init_msg_tau(getprogname())) {
		exit(2);
	}
	sw_register(getprogname());

	key = make_gate( &Jill_sw, REQUEST | PASS_REPLY);

	rc = sw_post("jill", key);
	if (rc) fatal("sw_post %d", rc);
}


int main (int argc, char *argv[])
{
	setprogname(argv[0]);

	jill_init();

	tag_loop();

	return 0;
}
