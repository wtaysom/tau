/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <sys/utsname.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <debug.h>
#include <eprintf.h>
#include <style.h>

/* test uname */

int release (void)
{
	int a;
	int b;
	int i;
	int c;
	char *r;
	struct utsname buf;
	int rc = uname(&buf);
	if (rc) fatal("rc=%d:", rc);
	r = buf.release;
	for (a = 0, i = 0; i < 3; i++) {
		for (b = 0, c = *r++; isdigit(c); c = *r++) {
			b = (b << 4) | (c - '0');
		}
		a = (a << 8) | b;
	}
	return a;
}

int main (int argc, char *argv[])
{
	struct utsname buf;
	int rc = uname(&buf);
	if (rc) fatal("rc=%d:", rc);
	PRs(buf.sysname);
	PRs(buf.nodename);
	PRs(buf.release);
	PRs(buf.version);
	PRs(buf.machine);
#ifdef _GNU_SOURCE
	PRs(buf.domainname);
#endif
	PRx(release());
	return 0;
}
