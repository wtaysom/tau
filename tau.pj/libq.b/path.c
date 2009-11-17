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

#include <style.h>
#include <path.h>

const char *file_path (const char *path)
{
	const char	*p = path;
	const char	*f = p;

	while (*p) {
		if (*p++ == '/') {
			while (*p == '/') {
				++p;
			}
			if (*p) f = p;
		}
	}
	return f;
}

char *file_get (const char *path, char *file)
{
	const char	*p = path;
	char		*f = file;

	while (*p) {
		if (*p == '/') {
			while (*++p == '/')
				;
			if (*p) f = file;
		}
		*f++ = *p++;
	}
	*f = '\0';
	return file;
}

const char *parse_path (const char *path, char *name)
{
	const char	*p = path;
	char		*n = name;

	while (*p == '/') {
		++p;
	}
	while (*p && (*p != '/')) {
		*n++ = *p++;
	}
	*n = '\0';
	return (n == name) ? 0 : p;
}

int is_last (const char *path)
{
	if (!path) return TRUE;

	while (*path == '/') {
		++path;
	}
	return *path == '\0';
}

#if 0

#include <stdio.h>

char *Path[] = {
	"",
	"/",
	"/a/b/c",
	"/aa/bb/cc",
	"//aa//bb//cc",
	"/a/b/c/",
	NULL};

void t1 (char *path)
{
	printf("%s : %s\n", path, file_path(path));
}

void t2 (char *path)
{
	char	file[1024];

	printf("%s : %s\n", path, file_get(path, file));
}

void t3 (char *path)
{
	char	file[1024];
	char	*p;

	printf("%s : ", path);
	p = parse_path(path, file);
	while (p) {
		printf("%s ", file);
		p = parse_path(p, file);
	}
	printf("\n");
}

int main (int argc, char *argv[])
{
	char	**p;

	for (p = Path; *p; p++) {
		t3(*p);
	}
	return 0;
}
#endif
