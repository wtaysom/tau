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
#include <setjmp.h>
//#define __USE_ISOC99
#include <ctype.h>

#include <sys/errno.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <style.h>
#include <debug.h>

#include <mdb.h>

/*
 * Work items:
 *	*1. Can we write? Probably - cc, cb, cw, cd, cq
 *	*2. Parse /proc/kallsyms
 *	*3. Repeat last command
 *		a. Advance address
 *		b. Remember format
 *		c. Command
 *	*4. More formats, dc, db, dw, dq
 *	*5. Command line editing
 *	*6. Stack back trace
 *	7. Symbol display -- continuous refinement
 *	9. History
 *		*a. non-persistent
 *		b. presistent
 *	10. Use /proc/<pid> -- can't do this on Mac
 *	*11. Help
 *	12. Autolookup DebugIsOn and set it
 */

enum {
	EOL = '\n',
	MAX_LINE = 2048,
	MAX_ARGS = 128};

bool	Debug = FALSE;
bool	Zerofill = FALSE;

char	Args[MAX_LINE];
char	*Argv[MAX_ARGS];
char	*Next;
int	Argc;

static jmp_buf	Err_jmp;

void err_jmp (char *msg)
{
printf("errno=%d\n", errno);
	if (errno) {
		perror(msg);
	} else {
		fprintf(stderr, "Error: %s\n", msg);
	}
	longjmp(Err_jmp, 1);
}

void prompt (void)
{
	printf("> ");
}

static void parse_line (char *line)
{
	char	*s;
	char	*r;
	char	*arg;
	char	c;

	Argc = 0;
	r = arg = Args;
	s = line;
	for (;;) {
		c = *s++;
		if (isspace(c) || !c) {
			if (arg != r) {
				*r++ = 0;
				Argv[Argc++] = arg;
				arg = r;
			}
			if (!c) {
				Argv[Argc] = NULL;
				return;
			}
		} else {
			*r++ = c;
		}
	}
}

void dump_args (void)
{
	int	i;

	printf("%d:", Argc);
	for (i = 0; i < Argc; ++i) {
		printf("%s ", Argv[i]);
	}
	printf("\n");
}

char *stripwhite (char *string)
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

void usage (void)
{
	printf("usage: mdb [-dv?]\n"
		"\td - debug\n"
		"\tv - zero fill\n"
		"\t? - usage\n"
		"\tWhen using mdb, type \"help\" for commands.\n"
		);
}

int main (int argc, char *argv[])
{
	char	*line;
	char	*s;
	int	c;

	while ((c = getopt(argc, argv, "dv?")) != -1) {
		switch (c) {
		case 'd':
			Debug = TRUE;
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
	initsyms();
	initkmem();

	initeval();

	setjmp(Err_jmp);
	for (;;) {
#if READLINE IS_ENABLED
		line = readline("> ");	/* use getline on OS-X */
		if (!line) {
			break;
		}
		s = stripwhite(line);
		if (*s) {
			add_history(s);
		}
#else
		ssize_t	rc;
		size_t	n;
		printf("> ");
		fflush(stdin);
		line = NULL;
		rc = getline( &line, &n, stdin);
		s = stripwhite(line);
#endif
		parse_line(s);
		if (Debug) dump_args();
		invokeCmd(Argc, Argv);
		free(line);
	}
	printf("\n");
	return 0;
}
