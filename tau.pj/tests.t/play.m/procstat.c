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

enum {	LABEL_SIZE = 32,
	MAX_ARGS = 1000,
	MAX_LINE = 4096};

typedef struct Vector_s {
	char	v_label[LABEL_SIZE];
	u64	v_size;
	u64	v_x[0];
} Vector_s;

char	*Args[MAX_ARGS];

int get_args (char *s)
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
	weprintf("too many args %d", n);
	return n;
}

void dump_args (int n)
{
	int	i;

	for (i = 0; i < n; i++) {
		printf("%s ", Args[i]);
	}
	printf("\n");
}

Vector_s *mk_vector (int n)
{
	int		size = n - 1;
	int		i;
	Vector_s	*v;

	if (size <= 0) return NULL;

	v = emalloc(sizeof(Vector_s) + sizeof(u64) * size);
	strncpy(v->v_label, Args[0], sizeof(v->v_label));

	for (i = 1; i < n; i++) {
		v->v_x[i-1] = strtoull(Args[i], NULL, 0);
	}
	v->v_size = size;
	return v;
}

void dump_vector (Vector_s *v)
{
	int	i;

	printf("%s", v->v_label);
	for (i = 0; i < v->v_size; i++) {
		printf(" %lld", v->v_x[i]);
	}
	printf("\n");
}
	
int main (int argc, char *argv[])
{
	FILE		*f;
	char		s[MAX_LINE];
	char		*r;
	int		n;
	Vector_s	*v;

	f = fopen("/proc/stat", "r");
	if (!f) {
		perror("/proc/stat");
		return 2;
	}
	for (;;) {
		r = fgets(s, sizeof(s), f);
		if (!r) break;
		printf("%s", s);
		n = get_args(s);
		dump_args(n);
		v = mk_vector(n);
		dump_vector(v);
	}
	fclose(f);
	return 0;
}
