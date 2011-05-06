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
#include <stdarg.h>
#include <ctype.h>
#include <debug.h>
#include <string.h>

#ifdef __WINDOWS__
	#define strncasecmp	_strnicmp
	#define strcasecmp	_stricmp
	#define snprintf	sprintf_s
#endif

#include <style.h>

int	DebugIsOn = 1;
int	AssertIsOn = 1;
int	FnDebugIsOn = 1;

static FILE **stdbug = &stdout;

int debugon (void)
{
	return DebugIsOn = 1;
}

int debugoff (void)
{
	return DebugIsOn = 0;
}

int fdebugon (void)
{
	return FnDebugIsOn = 1;
}

int fdebugoff (void)
{
	return FnDebugIsOn = 0;
}

void debugstderr (void)
{
	stdbug = &stderr;
}

void debugstdout (void)
{
	stdbug = &stdout;
}

bool debug_is_on (void)
{
	return DebugIsOn;
}

bool debug_is_off (void)
{
	return !DebugIsOn;
}

int debugenv (void)
{
	char	*c;

	c = getenv("DEBUG");
	if (c) {
		if (strcasecmp(c, "on") == 0) {
			fprintf(*stdbug, "Debug is on\n");
			DebugIsOn = 1;
			debugon();
			fdebugon();
		} else if (strcasecmp(c, "off") == 0) {
			debugoff();
			fdebugoff();
		}
	}
	return DebugIsOn;
}

bool prf (const char *fn)
{
	if (FnDebugIsOn) {
		fprintf(*stdbug, "%s\n", fn);
		fflush(*stdbug);
	}
	return TRUE;
}

int prbug (const char *format, ...)
{
	va_list	args;
	int	rc;

	va_start(args, format);
	rc = vfprintf(*stdbug, format, args);
	va_end(args);

	return rc;
}

int flushbug (void)
{
	return fflush(*stdbug);
}

bool print (const char *fn, unsigned line, const char *format, ...)
{
	va_list	args;
	char	buf[80];
	char	*b = buf;
	int	r = sizeof(buf);
	int	n;

	if (!DebugIsOn) return TRUE;

	if (line) {
		n = snprintf(b, r, "%s<%u>", fn, line);
	} else {
		n = snprintf(b, r, "%s", fn);
	}
	b += n;
	r -= n;

	va_start(args, format);
	vsnprintf(b, r, format, args);
	va_end(args);

	fprintf(*stdbug, "%s\n", buf);
	fflush(*stdbug);

	return TRUE;
}

bool pr (const char *fn, unsigned line, const char *label)
{
	if (label) return print(fn, line, "%s", label);
	else return print(fn, line, "");
}

bool prd (const char *fn, unsigned line, const char *label, s64 x)
{
	return print(fn, line, "%s=%lld", label, x);
}

bool prp (const char *fn, unsigned line, const char *label, void *x)
{
	return print(fn, line, "%s=%p", label, x);
}

bool prs (const char *fn, unsigned line, const char *label, const char *s)
{
	return print(fn, line, "%s=%s", label, s);
}

bool pru (const char *fn, unsigned line, const char *label, u64 x)
{
	return print(fn, line, "%s=%llu", label, x);
}

bool prx (const char *fn, unsigned line, const char *label, u64 x)
{
	return print(fn, line, "%s=%llx", label, x);
}

bool prg (const char *fn, unsigned line, const char *label, double x)
{
	return print(fn, line, "%s=%g", label, x);
}

static void pr_n_u8 (const void *mem, unint n)
{
	const u8	*u = mem;
	unint		i;

	prbug(" ");
	for (i = 4; i > n; i--) prbug("  ");

	while (i-- > 0) prbug("%.2x", u[i]);
}

bool prmem (
	const char	*label,
	const void	*mem,
	unsigned int	n)
{
	enum { NLONGS = 4,
		NCHARS = NLONGS * sizeof(u32) };

	const u32	*p = mem;
	const char	*c = mem;
	unsigned	i, j;
	unsigned	q, r;

	if (!DebugIsOn) return TRUE;

	prbug("%s:\n", label);

	q = n / NCHARS;
	r = n % NCHARS;
	for (i = 0; i < q; i++) {
		prbug("%p:", p);
		for (j = 0; j < NLONGS; j++) {
			prbug(" %8x", *p++);
		}
		prbug(" | ");
		for (j = 0; j < NCHARS; j++, c++) {
			prbug("%c", isprint(*c) ? *c : '.');
		}
		prbug("\n");
	}
	flushbug();
	if (!r) return TRUE;
	prbug("%8p:", p);
	for (j = 0; j < r / sizeof(u32); j++) {
		prbug(" %8x", *p++);
	}
	i = r % sizeof(u32);
	if (i) {
		++j;
		pr_n_u8(p, i);
	}
	for (; j < NLONGS; j++) {
		prbug("         ");
	}
	prbug(" | ");
	for (j = 0; j < r; j++, c++) {
		prbug("%c", isprint(*c) ? *c : '.');
	}
	prbug("\n");
	flushbug();
	return TRUE;
}

bool prbytes (
	const char	*label,
	const void	*mem,
	unsigned int	n)
{
	enum { CHARS_PER_LINE = 16 };
	const u8	*p = mem;
	const char	*c = mem;
	unsigned	i, j;
	unsigned	q, r;

	if (!DebugIsOn) return TRUE;

	prbug("%s:\n", label);

	q = n / CHARS_PER_LINE;
	r = n % CHARS_PER_LINE;
	for (i = 0; i < q; i++) {
		prbug("%p:", p);
		for (j = 0; j < CHARS_PER_LINE; j++) {
			prbug(" %.2x", *p++);
		}
		prbug(" | ");
		for (j = 0; j < CHARS_PER_LINE; j++, c++) {
			prbug("%c", isprint(*c) ? *c : '.');
		}
		prbug("\n");
	}
	if (r) {
		prbug("%8p:", p);
		for (j = 0; j < r; j++) {
			prbug(" %.2x", *p++);
		}
		for (; j < CHARS_PER_LINE; j++) {
			prbug("   ");
		}
		prbug(" | ");
		for (j = 0; j < r; j++, c++) {
			prbug("%c", isprint(*c) ? *c : '.');
		}
		prbug("\n");
	}
	flushbug();
	return TRUE;
}

int assertError (const char *what)
{
	if (AssertIsOn) {
		prbug("ASSERT FAILED: %s\n", what);
		flushbug();
		stacktrace();
		exit(2);
	}
	return 0;
}

