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

/* Copyright (C) 1999 Lucent Technologies */
/* Excerpted from 'The Practice of Programming' */
/* by Brian W. Kernighan and Rob Pike */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <eprintf.h>

#include <debug.h>
#include <style.h>

bool Stacktrace = TRUE;

/* pr_display: print debug message */
void pr_display (const char *file, const char *func, int line, const char *fmt, ...)
{
	va_list args;

	fflush(stdout);
	if (getprogname() != NULL) {
		fprintf(stderr, "%s ", getprogname());
	}
	fprintf(stderr, "%s:%s<%d> ", file, func, line);
	if (fmt) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);

		if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':') {
			fprintf(stderr, " %s<%d>", strerror(errno), errno);
		}
	}
	fprintf(stderr, "\n");
}

/* pr_fatal: print error message and exit */
void pr_fatal (const char *file, const char *func, int line, const char *fmt, ...)
{
	va_list args;

	fflush(stdout);
	fprintf(stderr, "Fatal ");
	if (getprogname() != NULL) {
		fprintf(stderr, "%s ", getprogname());
	}
	fprintf(stderr, "%s:%s<%d> ", file, func, line);
	if (fmt) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);

		if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':') {
			fprintf(stderr, " %s<%d>", strerror(errno), errno);
		}
	}
	fprintf(stderr, "\n");
	if (Stacktrace) stacktrace_err();
	exit(2); /* conventional value for failed execution */
}

/* pr_warn: print warning message */
void pr_warn (const char *file, const char *func, int line, const char *fmt, ...)
{
	va_list args;

	fflush(stdout);
	fprintf(stderr, "Warn ");
	if (getprogname() != NULL) {
		fprintf(stderr, "%s ", getprogname());
	}
	fprintf(stderr, "%s:%s<%d> ", file, func, line);
	if (fmt) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);

		if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':') {
			fprintf(stderr, " %s<%d>", strerror(errno), errno);
		}
	}
	fprintf(stderr, "\n");
}

/* pr_usage: print usage message */
void pr_usage (const char *fmt, ...)
{
	va_list args;

	fflush(stdout);
	fprintf(stderr, "Usage: ");
	if (getprogname() != NULL) {
		fprintf(stderr, "%s ", getprogname());
	}
	if (fmt) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
	}
	fprintf(stderr, "\n");
	exit(2);
}

/* eprintf: print error message and exit */
void eprintf (const char *fmt, ...)
{
	va_list args;

	fflush(stdout);
	if (getprogname() != NULL)
		fprintf(stderr, "%s: ", getprogname());

	if (fmt) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);

		if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':')
			fprintf(stderr, " %s<%d>", strerror(errno), errno);
	}
	fprintf(stderr, "\n");
	exit(2); /* conventional value for failed execution */
}

/* weprintf: print warning message */
void weprintf (const char *fmt, ...)
{
	va_list args;

	fflush(stdout);
	fprintf(stderr, "warning: ");
	if (getprogname() != NULL)
		fprintf(stderr, "%s: ", getprogname());
	if (fmt) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
		if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':')
			fprintf(stderr, " %s<%d>", strerror(errno), errno);
	}
	fprintf(stderr, "\n");
}

/* emalloc: malloc and report if error */
void *emalloc (size_t n)
{
	void *p;

	p = malloc(n);
	if (p == NULL)
		eprintf("malloc of %u bytes failed:", n);
	return p;
}

/* ezalloc: malloc, zero, and report if error */
void *ezalloc (size_t n)
{
	void *p;

	p = malloc(n);
	if (p == NULL)
		eprintf("malloc of %u bytes failed:", n);
	memset(p, 0, n);
	return p;
}

/* erealloc: realloc and report if error */
void *erealloc (void *vp, size_t n)
{
	void *p;

	p = realloc(vp, n);
	if (p == NULL)
		eprintf("realloc of %u bytes failed:", n);
	return p;
}

/* estrdup: duplicate a string, report if error */
char *estrdup (const char *s)
{
	char *t;

	t = (char *) malloc(strlen(s)+1);
	if (t == NULL) {
		eprintf("estrdup(\"%.20s\") failed:", s);
	}
	strcpy(t, s);
	return t;
}

#if __linux__ || __WINDOWS__
static char *name = NULL;  /* program name for messages */

// Defined in stdlib.h getprogname instead of progname
/* progname: return stored name of program */
const char *getprogname (void)
{
	return name;
}

/* setprogname: set stored name of program */
void setprogname (const char *str)
{
	name = estrdup(str);
}
#endif
