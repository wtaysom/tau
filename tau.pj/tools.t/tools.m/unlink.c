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

#include <stdio.h>
#include <unistd.h>

/*
 * unlink filename - unlinks the specified file name.  By
 * using double quotes around the file name, unusal file names
 * can be deleted such as file names with spaces in them.
 */

int main (int argc, char *argv[])
{
	int	rc;

	if (argc != 2) {
		printf("%s <filename>\n", argv[0]);
		return 2;
	}
	rc = unlink(argv[1]);
	if (rc != 0) {
		perror("unlink failed");
		return 2;
	}
	return 0;
}
