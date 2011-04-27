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

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifndef _STYLE_H_
#include <style.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

int debugon  (void);
int debugoff (void);
int fdebugon (void);
int fdebugoff(void);
int debugenv (void);

void debugstderr(void);
void debugstdout(void);

bool debug_is_on  (void);
bool debug_is_off (void);

bool prf     (const char *);
bool pr      (const char *fn, unsigned line, const char *);
bool prd     (const char *fn, unsigned line, const char *, s64);
bool prp     (const char *fn, unsigned line, const char *, void *);
bool prs     (const char *fn, unsigned line, const char *, const char *);
bool pru     (const char *fn, unsigned line, const char *, u64);
bool prx     (const char *fn, unsigned line, const char *, u64);
bool prg     (const char *fn, unsigned line, const char *, double x);
bool prid    (const char *fn, unsigned line, const char *, guid_t x);
bool prmem   (const char *, const void *, unsigned int);
bool prbytes (const char *, const void *, unsigned int);
bool print   (const char *fn, unsigned line, const char *format, ...);

#define FN_ARG		__FUNCTION__, __LINE__
#define FN		prf(__FUNCTION__)
#define HERE		pr(FN_ARG, NULL)
#define PR(_s_)		pr(FN_ARG, # _s_)
#define PRd(_x_)	prd(FN_ARG, # _x_, _x_)
#define PRp(_x_)	prp(FN_ARG, # _x_, _x_)
#define PRs(_x_)	prs(FN_ARG, # _x_, _x_)
#define PRu(_x_)	pru(FN_ARG, # _x_, _x_)
#define PRx(_x_)	prx(FN_ARG, # _x_, _x_)
#define PRg(_x_)	prg(FN_ARG, # _x_, _x_)
#define PRid(_x_)	prid(FN_ARG, # _x_, _x_)

typedef struct counter_s counter_s;
struct counter_s {
	counter_s	*next;
	const char	*where;
	const char	*fn;
	u64		count;
};
void count (counter_s *coutner);
void report (void);
#define CNT {								\
	static counter_s counter = { NULL, WHERE, __FUNCTION__, 0 };	\
	count( &counter);						\
}

#ifndef assert
extern int assertError(const char *what);
#define assert(_e_)	((void)((_e_) || assertError(WHERE " (" # _e_ ")")))
#endif

void stacktrace(void);

#ifdef __cplusplus
}
#endif

#endif
