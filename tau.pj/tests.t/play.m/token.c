/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <eprintf.h>

/*
 * Given a file, read it one line at a time, tokenize it,
 * set things up so we can pull things out. Not thread safe.
 */

enum {	MAX_TOKEN = 1024 };

static FILE *Fp;
static char *Delim;
static char Token[MAX_TOKEN];
static int Delimiter;

void close_token (void)
{
	if (Fp) fclose(Fp);
	Fp = NULL;
	if (Delim) free(Delim);
	Delim = NULL;
}

void open_token (char *file, char *delim)
{
	if (Fp) close_token();
	Fp = fopen(file, "r");
	if (!Fp) warn("couldn't open %s", file);
	Delim = strdup(delim);
	if (!Delim) fatal("couldn't duplicate %s", delim);
}

char *get_token (void)
{
	int	c;
	char	*next = Token;
	char	*end = &Token[MAX_TOKEN - 1];
	char	*d;

	while (next < end) {
skip:
		c = getc(Fp);
		if (c == EOF) break;
		for (d = Delim; *d; d++) {
			if (c == *d) {
				if (next == Token) goto skip;
				goto done;
			}
		}
		*next++ = c;
	}
	c = 0;
	if (next > Token) {
done:
		*next = '\0';
		Delimiter = c;
		return Token;
	}
	return NULL;
}

int get_delimiter (void)
{
	return Delimiter;
}

int main (int argc, char *argv[])
{
	char	*t;
	int	id = -1;

	open_token(
		"/sys/kernel/debug/tracing/events"
		"/raw_syscalls/sys_enter/format",
		" \n\t");
	for (;;) {
		t = get_token();
		if (!t) break;
		printf("%s\n", t);
	}
	open_token(
		"/sys/kernel/debug/tracing/events"
		"/raw_syscalls/sys_enter/format",
		" \n\t");
	for (;;) {
		t = get_token();
		if (!t) break;
		if (strcmp(t, "ID:") == 0) {
			t = get_token();
			if (!t) break;
			id = atoi(t);
		}
	}
	printf("id = %d\n", id);
	return 0;
}
