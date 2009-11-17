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

#include <QtGui>
#include "plotter.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <style.h>
#include <eprintf.h>

enum {	LABEL_SIZE = 32,
	MAX_ARGS = 1000,
	MAX_LINE = 4096};

typedef struct Vector_s {
	char	v_label[LABEL_SIZE];
	u64	v_size;
	u64	v_x[0];
} Vector_s;

static char	*Args[MAX_ARGS];

static int get_args (char *s)
{
	char	*c = s;
	int	n = 0;

	for (n = 0; n < MAX_ARGS; n++) {
		while (isspace(*c)) ++c;
		if (!*c) return n;
		Args[n] = c;
		for (; !isspace(*c) && *c; c++)
			;
		*c++ = '\0';
	}
	printf("too many args %d", n);
	return n;
}

#if 0
static void dump_args (int n)
{
	int	i;

	for (i = 0; i < n; i++) {
		printf("%s ", Args[i]);
	}
	printf("\n");
}

static Vector_s *mk_vector (int n)
{
	int		size = n - 1;
	int		i;
	Vector_s	*v;

	if (size <= 0) return NULL;

	v = (Vector_s *)emalloc(sizeof(Vector_s) + sizeof(u64) * size);
	strncpy(v->v_label, Args[0], sizeof(v->v_label));

	for (i = 1; i < n; i++) {
		v->v_x[i-1] = strtoull(Args[i], NULL, 0);
	}
	v->v_size = size;
	return v;
}

static void dump_vector (Vector_s *v)
{
	unint	i;

	printf("%s", v->v_label);
	for (i = 0; i < v->v_size; i++) {
		printf(" %lld", v->v_x[i]);
	}
	printf("\n");
}
#endif

static int match_prefix (char *prefix, char *s)
{
	for (;*prefix == *s++; ++prefix)
		;
	return !*prefix;
}
	
static u64 get_value (char *name)
{
	FILE		*f;
	char		s[MAX_LINE];
	char		*r;
	int		n;

	f = fopen("/proc/stat", "r");
	for (;;) {
		r = fgets(s, sizeof(s), f);
		if (!r) {
			fclose(f);
			return 0;
		}
		if (!match_prefix(name, s)) continue;

		fclose(f);
		n = get_args(s);
		if (n < 2) return 0;

	 	return strtoull(Args[1], NULL, 0);
	}
}

u64	Context_switches;

static void init_context_switches (void)
{
	Context_switches = get_value("ctxt");
}

static double context_switches (double x, int id)
{
	u64	y;
	u64	dy;

	unused(x);
	unused(id);
	y = get_value("ctxt");
	dy = y - Context_switches;
	Context_switches = y;

	return dy;
}

void start_context_switches (Plotter &plotter)
{
	int	color = 0;

	init_context_switches();

	plotter.setWindowTitle(QObject::tr("Context Switches"));
	plotter.setCurveData(color++, context_switches);
	plotter.show();
}
	
