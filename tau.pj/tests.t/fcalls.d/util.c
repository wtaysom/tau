/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Simple utilities for writing tests. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crc.h>
#include <debug.h>
#include <eprintf.h>
#include <mystdlib.h>
#include <style.h>
#include <util.h>

#include "fcalls.h"
#include "main.h"

/* PrErrork prints an error message including location */
void PrErrork (Where_s w, const char *fmt, ...)
{
	int lasterr = errno;
	va_list args;

	fflush(stdout);
	fprintf(stderr, "ERROR ");
	if (getprogname() != NULL) {
		fprintf(stderr, "%s ", getprogname());
	}
	fprintf(stderr, "%s:%s<%d> ", w.file, w.fn, w.line);
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
	if (Local_option.stack_trace) stacktrace_err();
	if (Local_option.exit_on_error) exit(2);
}

/* RndName generates a random name. The caller must free the returned name */
char *RndName (void)
{
	static char namechar[] = "abcdefghijklmnopqrstuvwxyz"
				 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				 "_";
	char *s;
	unint i;

	s = emalloc(NAME_LEN);
	for (i = 0; i < NAME_LEN - 1; i++) {
		s[i] = namechar[urand(sizeof(namechar) - 1)];
	}
	s[i] = '\0';
	return s;
}

/* Catstr creates a string by concatenating all the string
 * arguments. The last string must be NULL.
 * Caller must free the returned pointer.
 * O(n^2), so don't pass too many strings.
 */
char *Catstr (char *s, ...)
{
	char *t;
	char *u;
	int sum;
	va_list args;

	if (!s) return NULL;

	sum = strlen(s);
	va_start(args, s);
	for (;;) {
		t = va_arg(args, char *);
		if (!t) break;
		sum += strlen(t);
	}
	va_end(args);

	u = ezalloc(sum + 1);
	strcat(u, s);
	va_start(args, s);
	for (;;) {
		t = va_arg(args, char *);
		if (!t) break;
		strcat(u, t);
	}
	va_end(args);
	return u;
}

/* Generate a 8 bit hash of a 64 bit number */
static inline u8 Hash (s64 x)
{
	return crc32( &x, sizeof(x));
}

/* Fill a buffer of size n with bytes.
 * The values used to fill the buffer are derived
 * from the offset in the file for the data.
 * When the data is read, we can then check that
 * is the correct data for that location in the file.
 */
void Fill (void *buf, int n, s64 offset)
{
	u8 *b = buf;
	u8 *end = &b[n];

	for (; b < end; b++) {
		*b = Hash(offset);
		++offset;
	}
}

/* CheckFillk: does the buffer have the same content that was set by Fill */
bool CheckFillk (Where_s w, const void *buf, int n, s64 offset)
{
	const u8 *b = buf;
	const u8 *end = &b[n];

	for (; b < end; b++) {
		if (*b != Hash(offset)) {
			PrErrork(w,
				"CheckFill at offset %td expected 0x%2x"
				" but is 0x%2x",
				b - (u8 *)buf, Hash(offset), *b);
			return FALSE;
		}
		++offset;
	}
	return TRUE;
}

/* CheckEqk: checks if the two buffers are equal */
bool CheckEqk (Where_s w, const void *b1, const void *b2, int n)
{
	if (memcmp(b1, b2, n) != 0) {
		PrErrork(w, "CheckEq");
		return FALSE;
	}
	return TRUE;
}

/* CheckFailed: reports failure of expression */
bool CheckFailed (Where_s w, const char *e)
{
	PrErrork(w, " %s", e);
	return FALSE;
}

/* CrFile creates a file of specified size and Fills it with data. */
void CrFile (const char *name, u64 size)
{
	u8 buf[Local_option.block_size];
	int fd;
	u64 offset;

	fd = creat(name, 0666);
	for (offset = 0; size; offset += Local_option.block_size) {
		unint n = Local_option.block_size;
		if (n > size) n = size;
		Fill(buf, n, offset);
		write(fd, buf, n);
		size -= n;
	}
	close(fd);
}
