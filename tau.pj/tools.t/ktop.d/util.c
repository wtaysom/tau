/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <sys/utsname.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <eprintf.h>

#include "util.h"

int release_to_int (char *r)
{
	int a;
	int b;
	int i;
	int c;

	for (a = 0, i = 0; i < 3; i++) {
		for (b = 0, c = *r++; isdigit(c); c = *r++) {
			b = (b << 4) | (c - '0');
		}
		a = (a << 8) | b;
	}
	return a;
}

int kernel_release (void)
{
	struct utsname buf;
	int rc = uname(&buf);
	if (rc) fatal("rc=%d:", rc);
	return release_to_int(buf.release);
}

char *getpidname (int pid)
{
	char path[100];
	static char name[4096];
	int rc;

	snprintf(path, sizeof(path), "/proc/%d/exe", pid);
	rc = readlink(path, name, sizeof(name));
	if (rc == -1) {
		return strdup("<unknown>");
	}
	if (rc == sizeof(name)) {
		fatal("pid name too long");
	}
	name[rc] = '\0';

	return strdup(name);
}
