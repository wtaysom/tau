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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <style.h>
#include <eprintf.h>
#include <debug.h>

#include <cmd.h>

typedef struct Cmd_s	Cmd_s;
struct Cmd_s {
	Cmd_s	*next;
	char	*name;
	cmd_f	func;
	char	*help;
};

cmd_f	Quit_callback;
Cmd_s	*Cmd;

static Cmd_s *cmd_lookup (char *name)
{
	Cmd_s	*cmd;

	for (cmd = Cmd; cmd; cmd = cmd->next) {
		if (strcmp(name, cmd->name) == 0) {
			return cmd;
		}
	}
	return NULL;
}

void add_cmd (cmd_f fn, char *name, char *help)
{
	Cmd_s	*cmd;

	cmd = ezalloc(sizeof *cmd);
	cmd->name = name;
	cmd->func = fn;
	cmd->help = help;

	cmd->next = Cmd;
	Cmd = cmd;
}

static int invoke_cmd (char *name, int argc, char *argv[])
{
	Cmd_s	*cmd;

	cmd = cmd_lookup(name);
	if (cmd == 0) {
		printf("Command %s not found\n", name);
		return -1;
	}
	return cmd->func(argc, argv);
}

void execute (int argc, char *argv[])
{
	int	rc;

	if (argc == 0) {
		return;
	}
	rc = invoke_cmd(argv[0], argc, argv);
	if (rc != 0) {
		printf("error in %s = %d\n", argv[0], rc);
	}
}

int map_argv (int argc, char *argv[], argv_f fn, void *data)
{
	int	i;
	int	rc;

	for (i = 1; i < argc; i++) {
		rc = fn(argv[i], data);
		if (rc) return rc;
	}
	return 0;
}

int short_helpp (int argc, char *argv[])
{
	Cmd_s	*cmd;
	int	n;
	int	line;

	line = 0;
	for (cmd = Cmd; cmd; cmd = cmd->next) {
		n = strlen(cmd->name);
		if (line + n + 1 < 80) {
			printf(" %s", cmd->name);
			line += n + 1;
		} else {
			printf("\n%s", cmd->name);
			line = n;
		}
	}
	printf("\n");
	return 0;
}

static int helpp (int argc, char *argv[])
{
	Cmd_s	*cmd;

	for (cmd = Cmd; cmd; cmd = cmd->next) {
		if (cmd->help[0] == '#') {
			printf("%-6s %s\n", cmd->name, cmd->help);
		} else {
			printf("%s %s\n", cmd->name, cmd->help);
		}
	}
	return 0;
}

static int echop (int argc, char *argv[])
{
	int	i;
	int	space = FALSE;

	for (i = 1; i < argc; ++i) {
		if (space) printf(" ");
		else space = TRUE;
		printf("%s", argv[i]);
	}
	printf("\n");
	return 0;
}

static int onp (int argc, char *argv[])
{
	debugon();
	printf("Debug is on\n");
	return 0;
}

static int offp (int argc, char *argv[])
{
	debugoff();
	printf("Debug is off\n");
	return 0;
}

static int fonp (int argc, char *argv[])
{
	fdebugon();
	printf("Function tracing is on\n");
	return 0;
}

static int foffp (int argc, char *argv[])
{
	fdebugoff();
	printf("Function tracing is off\n");
	return 0;
}

static int qp (int argc, char *argv[])
{
	int	rc = 0;

	if (Quit_callback) rc = Quit_callback(argc, argv);
	exit(rc);
	return rc;
}

void init_shell (cmd_f quit_callback)
{
	Quit_callback = quit_callback;

	add_cmd(short_helpp,	"?",	"# print command list");

	CMD(help,	"# print this message");
	CMD(echo,	"<args>");
	CMD(on,		"# debug on");
	CMD(off,	"# debug off");
	CMD(fon,	"# function tracing on");
	CMD(foff,	"# function tracing off");
	CMD(q,		"# quit");
}

void stop_shell (void)
{
	if (Quit_callback) Quit_callback(0, NULL);
}
