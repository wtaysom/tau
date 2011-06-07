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
#include <ctype.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <cmd.h>

static char *stripwhite (char *string)
{
	register char *s, *t;

	//for (s = string; whitespace(*s); s++)
	for (s = string; isspace(*s); s++)
		;

	if (*s == 0) return (s);

	t = s + strlen (s) - 1;
	//while (t > s && whitespace(*t))
	while (t > s && isspace(*t))
		t--;
	*++t = '\0';

	return s;
}

int shell (void)
{
	char	*line;
	char	*s;
	int	argc;
	char	**argv;

	for (;;) {
		line = readline("> ");
		if (!line) {
			break;
		}
		s = stripwhite(line);
		if (*s) {
			add_history(s);
		}
		argc = line2argv(s, &argv);
		execute(argc, argv);
		free(line);
	}
	stop_shell();
	printf("\n");
	return 0;
}
