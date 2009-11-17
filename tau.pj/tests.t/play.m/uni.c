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
#include <uni.h>

int main (int argc, char *argv[])
{
	int	i;
	int	cnt = 0;

	for (i = 0; i < 0x10000; i++) {
		if (uni_to_lower(i) != i) {
			++cnt;
		}
	}
	printf("%d\n", cnt);
	return 0;
}
