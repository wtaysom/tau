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
#include <stdlib.h>

void usage (void)
{
	printf("bi <.1-.9> <2.0-4.0>\n");
	exit(2);
}

int main (int argc, char *argv[])
{
	double	x;
	double	r;
	int	i;

	if (argc != 3) usage();

	x = atof(argv[1]);
	r = atof(argv[2]);

	for (i = 0; i < 30; i++) {
		x = r * x * (1 - x);
		printf("%g\n", x);
	}
	return 0;
}
