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
#include <unistd.h>
#include <ctype.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <style.h>
#include <tau/msg.h>
#include <tau/msg_util.h>
#include <tau/switchboard.h>
#include <tau/tag.h>
#include <debug.h>
#include <eprintf.h>

typedef struct connect_s {
	type_s	*cn_type;
	unint	cn_key;
} connect_s;

connect_s	Connect;

bool	Debug = FALSE;

static char *stripwhite (char *string)
{
	register char *s, *t;

	for (s = string; isspace(*s); s++)
		;
		
	if (*s == 0) return (s);

	t = s + strlen (s) - 1;
	while (t > s && isspace(*t))
		t--;
	*++t = '\0';

	return s;
}

void get_lines (unint key)
{
	msg_s	m;
	char	*line;
	char	*s;
	unint	len;
	int	rc;

	for (;;) {
		line = readline("> ");	/* use getline on OS-X */
		if (!line) {
			break;
		}
		s = stripwhite(line);
		if (*s) {
			add_history(s);
		}
		//Send to debugger
		len = strlen(s) + 1;

		m.m_method = 0;
		rc = putdata_tau(key, &m, len, s);
		if (rc) eprintf("get_lines putdata_tau %d", rc);
	
		free(line);
	}
	printf("\n");
	exit(0);
}

void connect (void *msg)
{
	msg_s	*m = msg;

	if (Connect.cn_key) {
		weprintf("Already connected");
		destroy_key_tau(m->q.q_passed_key);
		return;
	}
	Connect.cn_key = m->q.q_passed_key;
	get_lines(Connect.cn_key);
}	

void connect_with_bicho (void)
{
	int	rc;
	unint	key;

	rc = sw_register(getprogname());
	if (rc) eprintf("sw_register %d", rc);

	Connect.cn_type = make_type("tty connection", NULL, connect);
	key = make_gate( &Connect, REQUEST | PASS_OTHER);
	rc = sw_post("tty", key);
	if (rc) eprintf("sw_post %d", rc);
}

static void usage (void)
{
	printf("usage: %s [-d]\n"
		"\td - debug\n",
		getprogname());
}

int main (int argc, char *argv[])
{
	int	c;

	setprogname(argv[0]);
	while ((c = getopt(argc, argv, "d?")) != -1) {
		switch (c) {
		case 'd':
			Debug = TRUE;
			break;
		case '?':
		default:
			usage();
			exit(1);
			break;
		}
	}
	init_msg_tau(getprogname());
	connect_with_bicho();
	tag_loop();

	return 0;
}
