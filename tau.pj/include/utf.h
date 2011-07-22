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

#ifndef _UTF_H_
#define _UTF_H_

#include <style.h>

#ifndef _UNICODE_T
#define _UNICODE_T
typedef unsigned short	unicode_t;
#endif

unint uni2utf (	/* Return -1 = err, else # utf bytes in dest. */
	const unicode_t	*unicode,	/* Source */
	utf8_t		*utf, 		/* Destination */
	unint		bufLen);	/* Length in bytes */

unint utf2uni (	/* Return: -1 = err, else # uni chars in dest. */
	utf8_t		*utf, 		/* Source */
	unicode_t	*unicode, 	/* Destination */
	unint		bufLen);	/* Length in bytes */

#endif
