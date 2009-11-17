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

#include <tau/msg.h>
#include <tau/switchboard.h>
#include <msg_internal.h>

#define sw_error(_note, _rc)	_sw_error(__func__, __LINE__, _note, _rc)

int _sw_error (
	const char	*func,
	int		line,
	const char	*note,
	int		rc)
{
	if (!rc) return 0;

	printk(KERN_ERR "Switchboard error: %s<%d> %s %d\n",
		func, line, note, rc);
	return rc;
}

int sw_register (const char *name)
{
	sw_msg_s	msg;
	int		rc;
	size_t		len;

	len = strnlen(name, TAU_NAME);
	if (len < TAU_NAME) ++len;
	msg.sw_name.nm_length = len;
	memcpy(msg.sw_name.nm_name, name, len);

	msg.sw_method = SW_REGISTER;
	rc = call_tau(SW_STD_KEY, &msg);
	if (rc) return sw_error("call_tau", rc);

	destroy_key_tau(SW_STD_KEY);
	rc = change_index_tau(msg.q.q_passed_key, SW_STD_KEY);
	if (rc) return sw_error("change_index_tau", rc);

	return 0;
}
EXPORT_SYMBOL(sw_register);

int sw_post (const char *name, ki_t key)
{
	sw_msg_s	msg;
	int		rc;
	size_t		len;

	len = strnlen(name, TAU_NAME);
	if (len < TAU_NAME) ++len;
	msg.sw_name.nm_length = len;
	memcpy(msg.sw_name.nm_name, name, len);

	msg.sw_method = SW_POST;
	msg.q.q_passed_key = key;
	rc = send_key_tau(SW_STD_KEY, &msg);
	if (rc) return sw_error("send_key_tau", rc);

	return rc;
}
EXPORT_SYMBOL(sw_post);

int sw_post_v (unint len, const void *name, ki_t key)
{
	sw_msg_s	msg;
	int		rc;

	if (len > TAU_NAME) return ETOOBIG;
	msg.sw_name.nm_length = len;
	memcpy(msg.sw_name.nm_name, name, len);

	msg.sw_method = SW_POST;
	msg.q.q_passed_key = key;
	rc = send_key_tau(SW_STD_KEY, &msg);
	if (rc) return sw_error("send_key_tau", rc);

	return rc;
}
EXPORT_SYMBOL(sw_post_v);

int sw_request (const char *name, ki_t key)
{
	sw_msg_s	msg;
	int		rc;
	size_t		len;

	len = strnlen(name, TAU_NAME);
	if (len < TAU_NAME) ++len;
	msg.sw_name.nm_length = len;
	memcpy(msg.sw_name.nm_name, name, len);

	msg.sw_method  = SW_GET;
	msg.sw_request = SW_ALL;
	msg.q.q_passed_key = key;
	rc = send_key_tau(SW_STD_KEY, &msg);
	if (rc) return sw_error("send_key_tau", rc);

	return rc;
}
EXPORT_SYMBOL(sw_request);

int sw_request_v (unint len, const void *name, ki_t key)
{
	sw_msg_s	msg;
	int		rc;

	if (len > TAU_NAME) return ETOOBIG;
	msg.sw_name.nm_length = len;
	memcpy(msg.sw_name.nm_name, name, len);

	msg.sw_method  = SW_GET;
	msg.sw_request = SW_ALL;
	msg.q.q_passed_key = key;
	rc = send_key_tau(SW_STD_KEY, &msg);
	if (rc) return sw_error("send_key_tau", rc);

	return rc;
}
EXPORT_SYMBOL(sw_request_v);

int sw_remote (const char *name, ki_t key)
{
	sw_msg_s	msg;
	int		rc;
	size_t		len;

	len = strnlen(name, TAU_NAME);
	if (len < TAU_NAME) ++len;
	msg.sw_name.nm_length = len;
	memcpy(msg.sw_name.nm_name, name, len);

	msg.sw_method  = SW_GET;
	msg.sw_request = SW_REMOTE;
	msg.q.q_passed_key = key;
	rc = send_key_tau(SW_STD_KEY, &msg);
	if (rc) return sw_error("send_key_tau", rc);

	return rc;
}
EXPORT_SYMBOL(sw_remote);

int sw_remote_v (unint len, const void *name, ki_t key)
{
	sw_msg_s	msg;
	int		rc;

	if (len > TAU_NAME) return ETOOBIG;
	msg.sw_name.nm_length = len;
	memcpy(msg.sw_name.nm_name, name, len);

	msg.sw_method  = SW_GET;
	msg.sw_request = SW_REMOTE;
	msg.q.q_passed_key = key;
	rc = send_key_tau(SW_STD_KEY, &msg);
	if (rc) return sw_error("send_key_tau", rc);

	return rc;
}
EXPORT_SYMBOL(sw_remote_v);

int sw_any (const char *name, ki_t *key)
{
	sw_msg_s	msg;
	int		rc;
	size_t		len;

	len = strnlen(name, TAU_NAME);
	if (len < TAU_NAME) ++len;
	msg.sw_name.nm_length = len;
	memcpy(msg.sw_name.nm_name, name, len);

	msg.sw_method  = SW_GET;
	msg.sw_request = SW_ALL;
	rc = call_tau(SW_STD_KEY, &msg);
	if (rc) return sw_error("call_tau", rc);

	*key = msg.q.q_passed_key;
	return rc;
}
EXPORT_SYMBOL(sw_any);

int sw_any_v (unint len, const void *name, ki_t *key)
{
	sw_msg_s	msg;
	int		rc;

	if (len > TAU_NAME) return ETOOBIG;
	msg.sw_name.nm_length = len;
	memcpy(msg.sw_name.nm_name, name, len);

	msg.sw_method  = SW_GET;
	msg.sw_request = SW_ALL;
	rc = call_tau(SW_STD_KEY, &msg);
	if (rc) return sw_error("call_tau", rc);

	*key = msg.q.q_passed_key;
	return rc;
}
EXPORT_SYMBOL(sw_any_v);

int sw_next (const char *name, ki_t *key, u64 start, u64 *next)
{
	sw_msg_s	msg;
	int		rc;
	size_t		len;

	len = strnlen(name, TAU_NAME);
	if (len < TAU_NAME) ++len;
	msg.sw_name.nm_length = len;
	memcpy(msg.sw_name.nm_name, name, len);

	msg.sw_method = SW_NEXT;
	msg.sw_sequence = start;
	rc = call_tau(SW_STD_KEY, &msg);
	if (rc == DESTROYED) {
		*key = 0;
		return rc;
	}
	if (rc) return sw_error("call_tau", rc);

	*key  = msg.q.q_passed_key;
	*next = msg.sw_sequence;
	return rc;
}
EXPORT_SYMBOL(sw_next);
