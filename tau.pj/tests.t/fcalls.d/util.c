/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

/*
 * Simple utilities for writing tests.
 */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crc.h>
#include <debug.h>
#include <style.h>
#include <util.h>

/* pr_error prints an error message including location */
void pr_error (HERE_DCL, const char *fmt, ...)
{
	int	lasterr = errno;
	va_list args;

	fflush(stdout);
	fprintf(stderr, "ERROR ");
	if (getprogname() != NULL) {
		fprintf(stderr, "%s ", getprogname());
	}
	fprintf(stderr, "%s:%s<%d> ", HERE_ARG);
	va_start(args, fmt);
	if (fmt) {
		vfprintf(stderr, fmt, args);

		if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':') {
			fprintf(stderr, " %s<%d>",
				strerror(lasterr), lasterr);
		}
	}
	va_end(args);
	fprintf(stderr, "\n");
	if (StackTrace) stacktrace_err();
	if (Fatal) exit(2); /* conventional value for failed execution */
}

/* rndname generates a random name. The caller must free the returned name */
char *rndname (unsigned n)
{
	static char namechar[] = "abcdefghijklmnopqrstuvwxyz"
				 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				 "_";
	char	*s;
	unint	i;

	if (!n) return NULL;

	s = emalloc(n);
	for (i = 0; i < n - 1; i++) {
		s[i] = namechar[random() % (sizeof(namechar) - 1)];
	}
	s[i] = '\0';
	return s;
}

/*
 * mkstr creates a string of all the given strings,
 * the last string must be NULL.
 * Caller must free.
 */
char *mkstr (char *s, ...)
{
	char	*t;
	char	*u;
	int	sum;
	va_list	args;

	if (!s) return NULL;

	sum = strlen(s);
	va_start(args, s);
	for (;;) {
		t = va_arg(args, char *);
		if (!t) break;
		sum += strlen(t);
	}
	va_end(args);

	u = emalloc(sum + 1);
	strcat(u, s);
	va_start(args, s);
	for (;;) {
		t = va_arg(args, char *);
		if (!t) break;
		strcat(u, t);	//XXX: could be more efficient
	}
	va_end(args);
	return u;
}	

static inline u8 hash (s64 offset)
{
	return crc32( &offset, sizeof(offset));
}

/* fill a buffer of size n with bytes starting with a seed */
void fill (void *buf, int n, s64 offset)
{
	u8	*b = buf;
	u8	*end = &b[n];

	for (; b < end; b++) {
		*b = hash(offset);
		++offset;
	}
}

/* is_same: does the buffer have the same content that was set by fill */
bool is_samep (HERE_DCL, void *buf, int n, s64 offset)
{
	u8	*b = buf;
	u8	*end = &b[n];

	for (; b < end; b++) {
		if (*b != hash(offset)) {
			pr_error(HERE_ARG,
				"is_same at offset %td expected 0x%2x but is 0x%2x",
				b - (u8 *)buf, hash(offset), *b);
			return FALSE;
		}
		++offset;
	}
	return TRUE;
}
