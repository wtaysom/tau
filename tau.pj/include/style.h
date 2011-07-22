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

/*
 * style.h define types needed by the coding suggestions below. style.h should
 * be included after standard header files, especially <sys/types.h> or header
 * files including <sys/types.h>.
 *
 *
 * NOT SO GENTLE CODING SUGESTIONS
 *
 * Authors: Greg Pachner, Vandana Rungta, Paul Taysom
 * Monday April 2, 2007 : July 8, 2005
 *
 * Data Types:
 *
 *	1.  For media structures use only:  __u8, __u16, __u32, __u64,
 *	    __s8, __s16, __s32, __s64, char.  char is only used for strings
 *	    stored on the media.
 *
 *	2.  Other types use: u8, u16, u32, u64, s8, s16, s32, s64, unint,
 *	    snint, char *, addr, saddr. For error returns, use int.
 *
 *	3.  Only use (char *) for strings. Do not use (u8 *).
 *
 *	4.  A separate header file will be used for media structures.  This
 *	    header file should only contain media dependent structures and
 *	    be independent of other header files, except <linux/kernel.h>
 *	    which defines __u8, __u16,  etc.
 *
 *	5.  Typedefs should end with _s, _u, or _t.  The kernel folks don't
 *	    seem to like typedefs, however, we  have found the following
 *	    convention works well with 'cscope.'
 *		typedef struct greg_s {
 *			u64	gr_a;
 *			u32	gr_b;
 *		} greg_s;
 *
 *	6.  All characters strings use utf-8 encoding.  Because some languages
 *	    need 32 bit unicode, the utf-8 encoding can take up to 6 octets.
 *	    Code that cares about the maximum number of characters should allow
 *	    6 octets for every utf-8 character.
 *
 * Coding Guidelines:
 *
 *	1.  Tab stops 8. Use tabs, not spaces.
 *
 *	2.  K&R style curly braces:
 *		if {
 *		} else {
 *		}
 *
 *	3.  Use curly braces around statements in body of `if`, `while`,
 *	    `switch`, `do`, and `for` statements.
 *
 *	4.  routines_use_underscores_to_separate_words.
 *
 *	5.  Global variables begin with a capital letter.
 *
 *	6.  Use unique names in structure elements.
 *		// instead of:
 *		typedef struct paul_s {
 *			void	*link;
 *			...
 *		} paul_s;
 *		// use:
 *		typedef struct paul_s {
 *			void	*p_link;
 *			...
 *		} paul_s;
 *
 *	7.  Use longer, descriptive names for local variable. I'm not sure I
 *	    agree with this one. I'm starting to tend towards single character
 *	    variable names -- a, b, c for parameters, x, y, z for variables,
 *	    i, j, k for indices, p, q for pointers, and r, s, t for strings.
 *
 *	8.  Routines should fit in one full screen except `switch` statements
 *	    can be longer.
 *
 *	9.  To avoid variable name conflicts and scoping problems, use inline
 *	    functions instead of macros for small routines.
 *
 *	10. Clean Header files (what did I mean by this?)
 *
 *	11. No global spinlock. Today, NSS uses one global spinlock. This does
 *	    make it simpler to ensure code correctness, however, now that most
 *	    systems have multiple cores and processors, we must not prevent
 *	    applications from taking advantage of parallelism.
 *
 *	12. For machine specific code use:
 *		__i386__   - 32 bit x86
 *		__x86_64__ - 64 bit x86
 *
 *	13. For all other cases, we default to
 *	   /usr/src/linux/Documentation/CodingStyle.
 *
 * Lessons Learned
 *
 *	1.  Using -1 for an invalid value for unsigned values, is a bad idea.
 *	    Use 0xff...fff.
 *
 * Acknowledgments:
 *	Thanks to William Taysom for editing.
 *
 * gcc -E -dM - </dev/null # to print all predefineds
 */

#ifndef _STYLE_H_
#define _STYLE_H_ 1

#ifdef __KERNEL__
	#include <linux/version.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ENABLE		(1
#define DISABLE		(0
#define IS_ENABLED	==1)
#define IS_DISABLED	==0)

#if defined(__APPLE__) || defined(__WINDOWS__)
	typedef	unsigned long long	__u64;
	typedef unsigned int		__u32;
	typedef unsigned short		__u16;
	typedef unsigned char		__u8;

	typedef long long		__s64;
	typedef int			__s32;
	typedef short			__s16;
	typedef char			__s8;
#else
	#ifndef _LINUX_TYPES_H
	#include <linux/types.h>
	#endif
#endif

#ifndef __KERNEL__
	typedef	unsigned long long	u64;
	typedef unsigned int		u32;
	typedef unsigned short		u16;
	typedef unsigned char		u8;

	typedef long long		s64;
	typedef int			s32;
	typedef short			s16;
	typedef char			s8;
#endif

#ifndef _NATURAL_INT_
	#define _NATURAL_INT_ 1
	typedef long		snint;
	typedef unsigned long	unint;
#endif

#ifndef _ADDR_
	#define _ADDR_ 1
	typedef unsigned long	addr;
#endif

#ifndef _SADDR_
	#define _SADDR_ 1
	typedef unsigned long	saddr;
#endif

#if !defined(_BOOL_) && !defined(__cplusplus) && !(__WINDOWS__)
	#ifndef FALSE
		enum { FALSE, TRUE };
	#endif
	#if defined(__KERNEL__)
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
			#undef bool
			#define _BOOL_ 1
			typedef _Bool	bool;
		#endif
	#else
		#undef bool
		#define _BOOL_ 1
		typedef _Bool	bool;
	#endif
#endif

#ifdef __WINDOWS__
	#ifndef FALSE
		enum { FALSE, TRUE };
	#endif
	typedef unsigned char	bool;
#endif

#ifndef _UTF8_T
#define _UTF8_T
typedef char	utf8_t;
#endif

typedef u8	guid_t[16];
#ifdef _UUID_UUID_H
enum { GUID_ASSERT = 1 / (sizeof(guid_t) == sizeof(uuid_t)) };
#endif

typedef struct timeval		timeval_s;

#ifndef NULL
#define NULL	0
#endif

#define unused(_x)	((void) _x)
enum {
	A_THOUSAND  = 1000,
	A_MILLION   = A_THOUSAND * A_THOUSAND,
	A_BILLION   = A_MILLION * A_THOUSAND
};

#ifndef MAGIC_STRING
	#define MAGIC_STRING(_x_)	# _x_
	#define MAKE_STRING(_x_)	MAGIC_STRING(_x_)
#endif

#undef WHERE
#ifdef KBUILD_BASENAME
	#ifdef KBUILD_MODNAME
		#define WHERE	KBUILD_MODNAME ":" KBUILD_BASENAME \
				"<" MAKE_STRING(__LINE__) "> "
	#else
		#define WHERE	KBUILD_BASENAME "<" MAKE_STRING(__LINE__) "> "
	#endif
#elif defined _F
	#define WHERE	_F "<" MAKE_STRING(__LINE__) "> "
#else
	#define WHERE	__FILE__ "<" MAKE_STRING(__LINE__) "> "
#endif

/*
 * Macros for checking values of constants
 * Generates a divide by 0 error during compilation
 */
#define CONCATENATE_DETAIL(_x_, _y_)	_x_##_y_
#define CONCATENATE(_x_, _y_)		CONCATENATE_DETAIL(_x_, _y_)
#define MAKE_UNIQUE(_x_)		CONCATENATE(_x_, __COUNTER__)
#define CHECK_CONST(_expression_)	\
	enum { MAKE_UNIQUE(CHECK_) = 1 / (_expression_) }

/*
 * When given a pointer to a field of sub-struct of a
 * structure, container returns a pointer to the beginning
 * of structure.
 */
#define container(_item, _type, _field)   \
	((_type *)(((addr)(_item)) - offsetof(_type, _field)))
/*
 * field returns the offset of a field in a structure
 */
#define field(_x, _f)	((addr)&(((__typeof__(_x))0)->_f))

#define zero(_x)	(memset( &(_x), 0, sizeof(_x)))

#undef ALIGN
#define ALIGN(_x_, _p_)	(((_x_) + (_p_) - 1) & ~((_p_) - 1))

#ifdef __cplusplus
}
#endif

#endif
