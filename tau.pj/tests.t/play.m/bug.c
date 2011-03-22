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

#include <style.h>

typedef struct bug_s	bug_s;

typedef void	(*op_f)(bug_s *bug, char *args);

struct bug_s {
	op_f	dm_op;
	addr	dm_address;
	unint	dm_num_lines;
};

op_f	Ops[256];

void execute (bug_s *bug, char *s)
{
	u8	c = *s;

	if (!s) return;

	Ops[c](bug, ++s);
}

static void invalidp (bug_s *bug, char *s)
{
	printf("invalide command\n");
}

int add_op (u8 c, op_f op)
{
	if (Ops[c] != invalidp) {
		fprintf(stderr, "opcode already exits: %c\n", c);
		return -1;
	}
	Ops[c] = op;
	return 0;
}

void init_ops (void)
{
	int	i;

	for (i = 0; i < 255; i++) {
		Ops[i] = invalidp;
	}
}

static void ap (bug_s *bug, char *args)
{
	printf("%s\n", args);
}

void init (void)
{
	init_ops();

	add_op('a', ap);
}

int main (int argc, char *argv[])
{
	char	s[1024];
	bug_s	bug;

	init();

	if (!fgets(s, sizeof(s), stdin)) return 0;
	execute( &bug, s);
	return 0;
}
