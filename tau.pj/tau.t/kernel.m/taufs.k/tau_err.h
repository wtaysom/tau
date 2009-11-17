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

#ifndef _TAU_ERR_H_
#define _TAU_ERR_H_

typedef enum {
	qERR_FULL		= -3000,
	qERR_DUP		= -3002,
	qERR_NOT_FOUND		= -3003,
	qERR_TRY_NEXT		= -3004,
	qERR_HASH_OVERFLOW	= -3005,
	qERR_BAD_BLOCK		= -3006,
	qERR_INUSE		= -3007,
	qERR_NULL_FUNCTION	= -3008,
	qERR_BAD_SLOT		= -3009,
	qERR_BAD_LOG		= -3010,
	qERR_BAD_TREE		= -3011,
} err_t;

#endif
