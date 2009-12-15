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

#ifndef _PDEBUG_H_
#define _PDEBUG_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

#define PI_DEBUG ENABLE

#ifdef __KERNEL__
	#define xprintk(L, fmt, arg...)	\
		printk(L "pi: " "%s<%d>: " fmt "\n", __func__, __LINE__, ## arg)

	#define iprintk(fmt, arg...) xprintk(KERN_INFO, fmt, ## arg)
	#define eprintk(fmt, arg...) xprintk(KERN_ERR, fmt, ## arg)
	#define dprintk(fmt, arg...) xprintk(KERN_DEBUG, fmt, ## arg)

	#define bug(fmt, arg...) {				\
		xprintk(KERN_EMERG, "!!!" fmt, ## arg);		\
		BUG();						\
	}
#endif

extern bool	Pi_debug_is_on;
extern bool	Pi_debug_func;
extern bool	Pi_debug_filter;
extern bool	Pi_debug_show_only;
extern bool	Pi_debug_delay;

int debugon    (void);
int debugoff   (void);
int pi_pr     (const char *fn, unsigned line, const char *fmt, ...);
int pi_label  (const char *fn, unsigned line, const char *);
int pi_prd    (const char *fn, unsigned line, const char *, s64);
int pi_prp    (const char *fn, unsigned line, const char *, const void *);
int pi_prs    (const char *fn, unsigned line, const char *, const char *);
int pi_pru    (const char *fn, unsigned line, const char *, u64);
int pi_prx    (const char *fn, unsigned line, const char *, u64);
int pi_prguid (const char *fn, unsigned line, const char *, guid_t);
int pi_prfn   (const char *fn);
int pi_here   (const char *fn, unsigned line);
int pi_prexit (const char *fn, unsigned line);
int pi_prmem  (const char *label, const void *addr, unint bytes);

int   pi_enter (const char *fn);
void  pi_vret  (const char *fn, unsigned line);
snint pi_iret  (const char *fn, unsigned line, snint x);
u64   pi_qret  (const char *fn, unsigned line, u64 x);
void *pi_pret  (const char *fn, unsigned line, void *x);

void pi_pr_packet (void *packet);

#if PI_DEBUG IS_ENABLED

	#define PR(fmt, arg...)	pi_pr(__func__, __LINE__, fmt, ## arg)
	#define FN		pi_prfn(__func__)
	#define HERE		pi_here(__func__, __LINE__)
	#define EXIT		pi_prexit(__func__, __LINE__)
	#define LABEL(_s_)	pi_label (__func__, __LINE__, # _s_)
	#define PRd(_x_)	pi_prd(__func__, __LINE__, # _x_, _x_)
	#define PRp(_x_)	pi_prp(__func__, __LINE__, # _x_, (void *)_x_)
	#define PRs(_x_)	pi_prs(__func__, __LINE__, # _x_, _x_)
	#define PRu(_x_)	pi_pru(__func__, __LINE__, # _x_, _x_)
	#define PRx(_x_)	pi_prx(__func__, __LINE__, # _x_, _x_)
	#define PRguid(_x_)	pi_prguid(__func__, __LINE__, # _x_, _x_)

	#define ENTER		pi_enter(__func__)
	#define vRet		return pi_vret(__func__, __LINE__)
	#define iRet(_x_)	return pi_iret(__func__, __LINE__, _x_)
	#define qRet(_x_)	return pi_qret(__func__, __LINE__, _x_)
	#define pRet(_x_)	return pi_pret(__func__, __LINE__, _x_)

#else

	#define FN		((void)0)
	#define HERE		((void)0)
	#define EXIT		((void)0)
	#define PR(_s_)		((void)0)
	#define PRd(_x_)	((void)0)
	#define PRp(_x_)	((void)0)
	#define PRs(_x_)	((void)0)
	#define PRu(_x_)	((void)0)
	#define PRx(_x_)	((void)0)
	#define PRguid(_x_)	((void)0)

	#define ENTER		((void)0)
	#define vRet		return
	#define iRet(_x_)	return _x_
	#define qRet(_x_)	return _x_
	#define pRet(_x_)	return _x_

#endif

#define NOT_IMPLEMENTED	pi_pr(__func__, __LINE__, "Not Implemented")

#undef assert
int pi_assert (const char *func, const char *what);
#define assert(_e_)	\
	((void)((_e_) || (pi_assert(__func__, WHERE "(" # _e_ ")"))))

int q_assert(const char *func, const char *what);

#endif
