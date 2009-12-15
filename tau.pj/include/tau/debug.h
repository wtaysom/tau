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

#ifndef _TAU_DEBUG_H_
#define _TAU_DEBUG_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

#define TAU_DEBUG ENABLE

#ifdef __KERNEL__
	#define xprintk(L, fmt, arg...)	\
		printk(L "tau: " "%s<%d>: " fmt "\n", __func__, __LINE__, ## arg)

	#define iprintk(fmt, arg...) xprintk(KERN_INFO, fmt, ## arg)
	#define eprintk(fmt, arg...) xprintk(KERN_ERR, fmt, ## arg)
	#define dprintk(fmt, arg...) xprintk(KERN_DEBUG, fmt, ## arg)

	#define bug(fmt, arg...) {				\
		xprintk(KERN_EMERG, "!!!" fmt, ## arg);		\
		BUG();						\
	}
#endif

enum {	tMSG   = 0x1,
	tNET   = 0x2,
	tFS    = 0x4,
	tKACHE = 0x8,
	tALL   = ~0ull };

extern bool	Tau_debug_is_on;
extern bool	Tau_debug_func;
extern bool	Tau_debug_trace;
extern bool	Tau_debug_filter;
extern bool	Tau_debug_show_only;
extern bool	Tau_debug_delay;
extern u64	Tau_debug_mask;

int debugon    (void);
int debugoff   (void);
int tau_pr     (const char *fn, unsigned line, const char *fmt, ...);
int tau_label  (const char *fn, unsigned line, const char *);
int tau_prd    (const char *fn, unsigned line, const char *, s64);
int tau_prp    (const char *fn, unsigned line, const char *, const void *);
int tau_prs    (const char *fn, unsigned line, const char *, const char *);
int tau_pru    (const char *fn, unsigned line, const char *, u64);
int tau_prx    (const char *fn, unsigned line, const char *, u64);
int tau_prguid (const char *fn, unsigned line, const char *, guid_t);
int tau_prfn   (const char *fn, u64 mask);
int tau_here   (const char *fn, unsigned line);
int tau_prexit (const char *fn, unsigned line);
int tau_prmem  (const char *label, const void *addr, unint bytes);

int   tau_enter (const char *fn);
void  tau_vret  (const char *fn, unsigned line);
snint tau_iret  (const char *fn, unsigned line, snint x);
u64   tau_qret  (const char *fn, unsigned line, u64 x);
void *tau_pret  (const char *fn, unsigned line, void *x);

void tau_pr_packet (void *packet);

#if TAU_DEBUG IS_ENABLED

	#ifdef tMASK
		#define FN	tau_prfn(__func__, tMASK)
	#else
		#define FN	tau_prfn(__func__, tALL)
	#endif
	#define PR(fmt, arg...)	tau_pr(__func__, __LINE__, fmt, ## arg)
	#define HERE		tau_here(__func__, __LINE__)
	#define EXIT		tau_prexit(__func__, __LINE__)
	#define LABEL(_s_)	tau_label (__func__, __LINE__, # _s_)
	#define PRd(_x_)	tau_prd(__func__, __LINE__, # _x_, _x_)
	#define PRp(_x_)	tau_prp(__func__, __LINE__, # _x_, (void *)_x_)
	#define PRs(_x_)	tau_prs(__func__, __LINE__, # _x_, _x_)
	#define PRu(_x_)	tau_pru(__func__, __LINE__, # _x_, _x_)
	#define PRx(_x_)	tau_prx(__func__, __LINE__, # _x_, _x_)
	#define PRguid(_x_)	tau_prguid(__func__, __LINE__, # _x_, _x_)

	#define ENTER		tau_enter(__func__)
	#define vRet		return tau_vret(__func__, __LINE__)
	#define iRet(_x_)	return tau_iret(__func__, __LINE__, _x_)
	#define qRet(_x_)	return tau_qret(__func__, __LINE__, _x_)
	#define pRet(_x_)	return tau_pret(__func__, __LINE__, _x_)

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

#define NOT_IMPLEMENTED	tau_pr(__func__, __LINE__, "Not Implemented")

#undef assert
int tau_assert (const char *func, const char *what);
#define assert(_e_)	\
	((void)((_e_) || (tau_assert(__func__, WHERE "(" # _e_ ")"))))

int q_assert(const char *func, const char *what);

#endif
