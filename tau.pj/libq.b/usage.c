/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

/*
 * Default usage used if program does not supply its own
 * usage function.
 */

#include <stdio.h>
#include <stdlib.h>

#include <eprintf.h>

void usage(void)
{
	fprintf(stderr, "\nDefault usage: %s"
		" [options] [file/directory]*\n\n",
		getprogname());
	fprintf(stderr, "  Possible options"
		" (Not all puny tests use all these options):\n"
		"  -h -f<file> -d<dir> -e<destination> -i<num> -s<secs>"
		" -z<size> -t<num>"
		" -x<name> -v<value> -n<num chars>\n\n");
	fprintf(stderr,
		"  -h  this help message\n"
		"  -f  file name to use (source)\n"
		"  -d  directory to use\n"
		"  -e  destination or target directory/file\n"
		"  -i  number of iterations to run (inner loop)\n"
		"  -l  number of loops (outer loop) 0 -> LLONG_MAX\n"
		"  -n  number of characters to use in file name\n"
		"  -s  seconds to sleep between operations\n"
		"  -t  number of threads to create\n"
		"  -v  value for extended attribute\n"
		"  -x  extended attribute name\n"
		"  -z  size of files to create\n"
		);
	exit(2);
}
