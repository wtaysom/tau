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
#include <string.h>

#define	PR(_s_)		printf(#_s_ "=%s\n", _s_)
#define	PX(_x_)		printf(#_x_ "=%x\n", _x_)

char *getPrefix (char *name, char *prefix, char *suffix)
{
	char	*current;
	char	*start;
	char	*startPrefix = prefix;

	for (current = name; *current != '\0'; ++current)
		;
	for (start = current; start != name; --start)	{
		if (*start == '/')	{
			if (start == current)	{
				*prefix = '\0';
				*suffix = '\0';
				return startPrefix;
			}
			++start;
			break;
		}
	}
	for (name = start; *current != '.'; --current)	{
		if (current == name)	{
			*suffix = '\0';
			return strcpy(prefix, name);
		}
	}
	while (name != current)	*prefix++ = *name++;
	*prefix = '\0';
	strcpy(suffix, ++name);
	return startPrefix;
}

#define MAX_NAME	1024
#define MAX_PATH	4096

char	Name[MAX_PATH];
char	Prefix[MAX_PATH];
char	Suffix[MAX_NAME];

int main (int argc, char *argv[])
{
	int		i;
	int		rc;

	if (argc > 1)	{
		for (i = 1; i < argc; ++i)	{
			printf("%s\n", getPrefix(argv[i], Prefix, Suffix));
		}
	} else {
		for (rc = scanf("%s", Name); rc != EOF; rc = scanf("%s", Name))	{
			printf("%s\n", getPrefix(Name, Prefix, Suffix));
		}
	}
	return 0;
}
