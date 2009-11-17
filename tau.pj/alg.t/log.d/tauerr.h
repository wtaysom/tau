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

#ifndef _TAUERR_H_
#define _TAUERR_H_

typedef enum {
	qERR_FULL		= -1000,
	qERR_DUP		= -1002,
	qERR_NOT_FOUND		= -1003,
	qERR_TRY_NEXT		= -1004,
	qERR_HASH_OVERFLOW	= -1005,
	qERR_BAD_BLOCK		= -1006,
	qERR_INUSE		= -1007,
	qERR_NULL_FUNCTION	= -1008,
	qERR_BAD_SLOT		= -1009,
	qERR_BAD_LOG		= -1010,
	qERR_BAD_TREE		= -1011
} err_t;

#endif
