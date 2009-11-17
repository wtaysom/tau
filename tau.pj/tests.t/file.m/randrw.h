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

#ifndef _RANDRW_H_
#define _RANDRW_H_

#ifndef _CRC_H_
#include <crc.h>
#endif

enum { NUM_LONGS = 3157, NUM_BUFS = (1<<17) };

//typedef enum { FALSE = 0, TRUE } BOOL;

typedef struct Buf_s {
	long	seed;
	u32	crc;
	long	data[NUM_LONGS];
} Buf_s;

#endif
