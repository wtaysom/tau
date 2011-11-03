/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <eprintf.h>
#include <style.h>

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

static inline bool is_delim (int c)
{
	char	*d;

	for (d = Delim; *d; d++) {
		if (c == *d) return TRUE;
	}
	return FALSE;
}

char *get_token (void)
{
	int	c;
	char	*next = Token;
	char	*end = &Token[MAX_TOKEN - 1];

	while (next < end) {
		c = getc(Fp);
		if (c == EOF) break;
		if (is_delim(c)) {
			if (next == Token) continue;
			break;
		}
		*next++ = c;
	}
	if (next == Token) return NULL;
	*next = '\0';
	Delimiter = c;
	return Token;
}

int get_delimiter (void)
{
	return Delimiter;
}
