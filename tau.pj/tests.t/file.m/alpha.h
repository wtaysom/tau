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

#ifndef _ALPHA_H_
#define _ALPHA_H_

#ifndef _STDIO_H
#include <stdio.h>
#endif

#ifndef MAGIC_STRING
	#define MAGIC_STRING(_x_)   # _x_
	#define MAKE_STRING(_x_)	MAGIC_STRING(_x_)
#endif

#ifndef WHERE
	#ifdef FNAM_
		#define WHERE   FNAM_ "[" MAKE_STRING(__LINE__) "]"
	#else
		#define WHERE   __FILE__ "[" MAKE_STRING(__LINE__) "]"
	#endif
#endif

#define DUCK(item, type, linkField)   \
	((type *)(((addr)(item)) - offsetof(type, linkField)))

#undef assert
int assertError (char *msg)
{
	fprintf(stderr, "ASSERT FAILED:%s\n", msg);
	return 0;
}

#define assert(_e_) \
	((void)((_e_) || (assertError(WHERE " (" # _e_ ")"))))


#define pr(_x)	fprintf(stderr, "|%s\n", # _x)
#define prp(_x)	fprintf(stderr, "|%s=%p\n", # _x, _x)
#define prx(_x)	fprintf(stderr, "|%s=%x\n", # _x, _x)
#define prd(_x)	fprintf(stderr, "|%s=%d\n", # _x, _x)
#define prq(_x)	fprintf(stderr, "|%s=%qx\n", # _x, _x)
#define prs(_x)	fprintf(stderr, "|%s=%s\n", # _x, _x)

#endif
