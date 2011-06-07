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

#include <sys/errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <setjmp.h>

#include <style.h>
#include <tau/msg.h>
#include <tau/msg_util.h>
#include <tau/switchboard.h>
#include <tau/tag.h>
#include <debug.h>
#include <eprintf.h>
#include <cmd.h>

bool	Zerofill = FALSE;

static jmp_buf	Err_jmp;

void err_jmp (char *msg)
{
	PRd(errno);
	if (errno) {
		perror(msg);
	} else {
		fprintf(stderr, "Error: %s\n", msg);
	}
	longjmp(Err_jmp, 1);
}

void dump_args (int argc, char *argv[])
{
	int	i;

	printf("%d:", argc);
	for (i = 0; i < argc; ++i) {
		printf("%s ", argv[i]);
	}
	printf("\n");
}

void process_command (char *line)
{
	int	argc;
	char	**argv;

	if (setjmp(Err_jmp)) return;
	argc = line2argv(line, &argv);
	execute(argc, argv);
	if (debug_is_on()) dump_args(argc, argv);
}

void tty_died (void *msg)
{
	printf("tty went away\n");
	exit(2);
}

void sw_died (void *msg)
{
	printf("switchboard went away\n");
	exit(2);
}

void get_line (void *msg)
{
	msg_s	*m = msg;
	char	*line;
	int	rc;

	line = malloc(m->q.q_length);
	rc = read_data_tau(m->q.q_passed_key, m->q.q_length, line, 0);
	if (rc) eprintf("read_data %d", rc);
	process_command(line);
	send_tau(m->q.q_passed_key, m);
	free(line);
}

void tty_connection (void *msg)
{
	static type_s	*tty_connection;
	msg_s	*m = msg;
	ki_t	reply = m->q.q_passed_key;
	int	rc;

	tty_connection = make_type("tty_connection", tty_died, get_line);
	m->q.q_passed_key = make_gate( &tty_connection, RESOURCE | PASS_REPLY);
	m->m_method = 0;
	rc = send_key_tau(reply, m);
	if (rc) eprintf("send_key_tau %d", rc);
	destroy_key_tau(reply);
}

void switchboard (void)
{
	extern void	kbicho (void *msg);

	static type_s	*tty_request;
	static type_s	*kbicho_request;
	ki_t	key;
	int	rc;

	rc = sw_register(getprogname());
	if (rc) eprintf("sw_register %d", rc);

	tty_request = make_type("tty_request", sw_died, tty_connection, NULL);
	key = make_gate( &tty_request, RESOURCE | PASS_OTHER);
	rc = sw_request("tty", key);
	if (rc) eprintf("sw_request tty %d", rc);

	kbicho_request = make_type("kbicho_request", sw_died, kbicho, NULL);
	key = make_gate( &kbicho_request, RESOURCE | PASS_OTHER);
	rc = sw_request("kbicho", key);
	if (rc) eprintf("sw_request kbicho %d", rc);
}

static void my_commands (void)
{
	extern int nsp (int argc, char *argv[]);
	extern int psp (int argc, char *argv[]);
	extern int keysp (int argc, char *argv[]);

	CMD(ns,		"# list nodes");
	CMD(ps,		"# list processes and avatars");
	CMD(keys,	"# list processes and keys");
}

static void usage (void)
{
	printf("usage: %s [-dv?]\n"
		"\td - debug\n"
		"\tv - zero fill\n"
		"\t? - usage\n",
		getprogname());
}

int main (int argc, char *argv[])
{
	int	c;

	setprogname(argv[0]);
	debugoff();
	init_shell(NULL);
	my_commands();

	while ((c = getopt(argc, argv, "dv?")) != -1) {
		switch (c) {
		case 'd':
			debugon();
			fdebugon();
			break;
		case 'v':
			Zerofill = TRUE;
			break;
		case '?':
		default:
			usage();
			exit(1);
			break;
		}
	}
	if (init_msg_tau(getprogname())) {
		fatal("Couldn't connect to messaging system");
	}
	switchboard();
	tag_loop();

	return 0;
}
