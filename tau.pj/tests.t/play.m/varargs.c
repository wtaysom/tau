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
#include <stdarg.h>
#include <stdio.h>

int sum (int x, ...)
{
	va_list	ap;
	int	s = x;
	int	y;

	if (!s) return s;
	va_start(ap, x);
	for (;;) {
		y = va_arg(ap, int);
		if (!y) break;
		s += y;
	}
	va_end(ap);
	printf("%d\n", s);
	s = x;
	va_start(ap, x);
	for (;;) {
		y = va_arg(ap, int);
		if (!y) break;
		s += y;
	}
	va_end(ap);
	return s;
}

int main (int argc, char *argv[])
{
	printf("%d\n", sum(1, 2, 3, 4, 0));
	return 0;
}
