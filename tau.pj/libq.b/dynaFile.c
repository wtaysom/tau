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

#undef TEST

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dynaFile.h"

enum {
	STARTSIZE = 16,
	MAXSIZE   = (1<<24)
};

DynaFile_s *dynaCreate (void)
{
	DynaFile_s	*df;

	df = malloc(sizeof(DynaFile_s));
	if (df == NULL) {
		return NULL;
	}
	df->eof    = 0;
	df->phyEof = STARTSIZE;
	df->offset = 0;
	df->buf    = malloc(STARTSIZE);
	if (df->buf == NULL) {
		free(df);
		return NULL;
	}
	return df;
}

ssize_t dynaWrite (DynaFile_s *df, const void *buf, size_t numBytes)
{
	void	*new;

	if (df->offset + numBytes > df->phyEof) {
		if (df->phyEof >= MAXSIZE) return TOO_BIG;
		new = realloc(df->buf, df->phyEof << 1);
		if (new == NULL) return NO_MEMORY;
		df->buf = new;
		df->phyEof <<= 1;
	}
	memcpy( &df->buf[df->offset], buf, numBytes);
	df->offset += numBytes;
	if (df->offset > df->eof) {
		df->eof = df->offset;
	}
	return numBytes;
}

ssize_t dynaRead (DynaFile_s *df, void *buf, size_t numBytes)
{
	size_t	bytesToRead;

	if (df->offset >= df->eof) return 0;
	if (df->offset + numBytes > df->eof) {
		bytesToRead = df->eof - df->offset;
	} else {
		bytesToRead = numBytes;
	}
	memcpy(buf, &df->buf[df->offset], bytesToRead);
	df->offset += bytesToRead;
	return bytesToRead;
}

void dynaClose (DynaFile_s *df)
{
	free(df->buf);
	free(df);
}

void dynaSeek (DynaFile_s *df, size_t offset)
{
	df->offset = offset;
}

size_t dynaEOF (DynaFile_s *df)
{
	return df->eof;
}

#ifdef TEST
int main (int argc, char *argv[])
{
	DynaFile_s	*df;
	int			i;
	int			rc;
	char		writeBuf[2];
	char		readBuf[2];

	writeBuf[0] = 'a';
	writeBuf[1] = 'b';
	df = dynaCreate();
	for (i = 0; i < 20000; ++i) {
		rc = dynaWrite(df, writeBuf, 2);
		if (rc != 2) {
			printf("Error:dynaWrite=%d\n", rc);
			exit(3);
		}
	}
	dynaSeek(df, 0);
	for (i = 0; i < 20000; ++i) {
		rc = dynaRead(df, readBuf, 2);
		if (rc != 2) {
			printf("Error:dynaRead=%d\n", rc);
			exit(3);
		}
		if (memcmp(readBuf, writeBuf, 2) != 0) {
			printf("%d. %c!=%c or %c!=%c\n", i, readBuf[0], writeBuf[0],
												readBuf[1], writeBuf[1]);
			exit(2);
		}
	}
	dynaClose(df);
	return 0;
}
#endif
