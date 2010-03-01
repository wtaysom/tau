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

/* eprintf.h: error wrapper functions */
#ifndef _EPRINTF_H_
#define _EPRINTF_H_

#ifndef _STDDEF_H
#include <stddef.h>
#endif
	#define xprintk(L, fmt, arg...)	\
		printk(L "tau: " "%s<%d>: " fmt "\n", __func__, __LINE__, ## arg)

	#define iprintk(fmt, arg...) xprintk(KERN_INFO, fmt, ## arg)

#ifdef F
#define MYFILE	F
#else
#define MYFILE	__FILE__
#endif

#define fatal(fmt, arg...)	pr_fatal(MYFILE, __func__, __LINE__, fmt, ## arg)
#define warn(fmt, arg...)	pr_warn(MYFILE, __func__, __LINE__, fmt, ## arg)
#define display(fmt, arg...)	pr_display(MYFILE, __func__, __LINE__, fmt, ## arg)

void pr_fatal  (const char *file, const char *func, int line, const char *fmt, ...);
void pr_warn   (const char *file, const char *func, int line, const char *fmt, ...);
void pr_display(const char *file, const char *func, int line, const char *fmt, ...);
void pr_usage  (const char *fmt, ...);
void eprintf   (const char *, ...);
void weprintf  (const char *, ...);
void *emalloc  (size_t);
void *ezalloc  (size_t);
void *erealloc (void *, size_t);
char *estrdup  (const char *);

void setprogname (const char *);
const char *getprogname (void);
#if __linux__
#endif

#define	NELEMS(a)	(sizeof(a) / sizeof(a[0]))

#endif
