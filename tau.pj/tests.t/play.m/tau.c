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
//#include <τ.h> svn on OS-X doesn't like this name - can't convert it to
// something the mac can understand. Apprently, OS-X does't grok utf-8,
// though I have created file names on mac with τ in them, I suspect mac
// is using a different encoding.
#include <stdio.h>

#if 1 //JUST_TO_PROVE_IT_CAN_BE_DONE
int main (int argc, char *argv[])
{
	//int	τ;

	printf("%c%c", '\xcf', '\x84');
	//printf("%d\n", 'τ');

	return 0;
}
#endif
