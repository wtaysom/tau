/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

#include <errno.h>
#include <unistd.h>

#include <style.h>
#include <eprintf.h>
#include <puny.h>

#include <fcalls.h>

void usage (void)
{
	pr_usage("-d<directory>");
}

int main (int argc, char *argv[])
{
	punyopt(argc, argv, NULL, NULL);
	chdir(Option.dir);
	chdirErr(ENOTDIR, "/etc/passwd");
	return 0;
}
