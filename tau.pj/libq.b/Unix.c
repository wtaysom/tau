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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "style.h"

int DBG_DebugBreak = 1;

void EnterDebugger (void)
{
	printf("EnterDebugger (should have breakpointed)\n");
//	command();
	exit(1);
}

int DBG_AssertError (char *errString)
{
	printf("\nASSERT FAILED: %s\n", errString);
	return 0;
}

int DBG_AssertWarning (char *errString, int *cnt)
{
	++(*cnt);
	if (*cnt == 1) {
		printf("\nWARNING: %s\n", errString);
	}
	return 0;
}

int DBG_AssertErrorStub (void)
{
	return 0;
}

void *zalloc (unint size)
{
	void	*data;

	data = malloc(size);
	if (data == NULL) {
		printf("OUT OF MEMORY\n");
		exit(1);
	}
	bzero(data, size);

	return data;
}

void *zallocPage (unint numPages)
{
	return zalloc(numPages * 4096);
}

void freePage (void *page, unint numPages)
{
	free(page);
}
