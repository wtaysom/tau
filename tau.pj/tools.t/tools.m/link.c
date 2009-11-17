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
 * link creates a hard link to a file.  By passing the name
 * in quotes, spaces and other strange characters in file names
 * can be processed.  Can be used with unlink to rename a file
 * with a space in the name (common with Mac created names).
 */

int main (int argc, char *argv[])
{
	int	rc;

	if (argc != 3)
	{
		printf("%s <filename1> <filename2>\n", argv[0]);
		return 2;
	}
	rc = link(argv[1], argv[2]);
	if (rc != 0)
	{
		perror("link failed");
		return 2;
	}
	return 0;
}
