/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

/*
 * testopt tests punyopt, the option processor for the
 * puny benchmarks.
 */

#include <stdio.h>

#include <eprintf.h>
#include <puny.h>

static bool My_option = FALSE;

#if 0
/* Comment to test having my own usage function */
void usage(void)
{
	pr_usage("my usage");
}
#endif

bool myopt (int c)
{
	switch (c) {
	case 'y':
		My_option = TRUE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

int main(int argc, char *argv[])
{
	punyopt(argc, argv, myopt, "y");

	printf("Options:\n"
		" iterations  = %lld\n"
		" num threads = %lld\n"
		" sleep secs  = %lld\n"
		" file size   = %lld\n"
		" name size   = %lld\n"
		" file name   = %s\n"
		" dir name    = %s\n"
		" dest name   = %s\n"
		" xattr name  = %s\n"
		" value name  = %s\n"
		" print       = %s\n",
		Option.iterations,
		Option.numthreads,
		Option.sleep_secs,
		Option.file_size,
		Option.name_size,
		Option.file,
		Option.dir,
		Option.dest,
		Option.xattr,
		Option.value,
		Option.print ? "true" : "false");

	if (My_option) {
		printf("My own option: -y\n");
	}
	return 0;
}
