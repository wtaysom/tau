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

#ifndef _DYNAFILE_H_
#define _DYNAFILE_H_

#ifndef _STDIO_H_
#include <stdio.h>
#endif

typedef enum DynaErrors_t {
	NO_MEMORY = -2,
	TOO_BIG   = -3
} DynaErrors_t;

typedef struct DynaFile_s {
	size_t	eof;
	size_t	phyEof;
	size_t	offset;
	char	*buf;
} DynaFile_s;

extern DynaFile_s *dynaCreate(void);
extern ssize_t dynaWrite(DynaFile_s *df, const void *buf, size_t numBytes);
extern ssize_t dynaRead(DynaFile_s *df, void *buf, size_t numBytes);
extern void dynaClose(DynaFile_s *df);
extern void dynaSeek(DynaFile_s *df, size_t offset);
extern size_t dynaEOF(DynaFile_s *df);

#endif
