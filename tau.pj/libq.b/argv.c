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

#include <cmd.h>

enum {	MAX_ARGS = 512 };

static int	Argc;
static char	*Argv[MAX_ARGS];

static void new_arg (char *arg)
{
	char	*a;

	if (Argc == MAX_ARGS)
	{
		printf("Hit max args\n");
		return;
	}
	a = malloc(strlen(arg) + 1);
	if (!a)
	{
		printf("Out of memory\n");
		return;
	}
	strcpy(a, arg);
	Argv[Argc++] = a;
}

static void save_arg (char *arg)
{
	if (*arg == '\0') {
		return;
	}
	new_arg(arg);
#if 0
	if (isPattern(arg)) {
		expand(arg, expand_arg); // expandArg(arg);
	} else {
		new_arg(arg);
	}
#endif
}

static void free_args (void)
{
	int	i;

	for (i = 0; i < Argc; ++i) {
		free(Argv[i]);
	}
	Argc = 0;
}

int line2argv (char *line, char ***argvp)
{
	char	*cp = line;
	char	*arg;

	free_args();
	for (;;) {
		while (isspace(*cp)) ++cp;
		if (*cp == '\0') {
			goto finish;
		}
		arg = cp;
		for (;;) {
			if (isspace(*cp)) break;
			if (*cp == '\0') {
				save_arg(arg);
				goto finish;
			}
			++cp;
		}
		*cp++ = '\0';
		save_arg(arg);
	}
finish:
	*argvp = Argv;
	return Argc;
}
