/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

/*
 * puny.h - header file to be used by all puny benchmarks to
 * for a consistent set of routines for assisting the benchmarks.
 */

#ifndef _PUNY_H_
#define _PUNY_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

typedef struct Option_s {
	u64	iterations;	/* -i<num> */
	u64	loops;		/* -l<num> */
	u64	numthreads;	/* -t<num> */
	u64	sleep_secs;	/* -s<secs> */
	u64	file_size;	/* -z<size> */
	u64	name_size;	/* -n<num chars> */
	bool	print;		/* -p */
	char	*file;		/* -f<file> */
	char	*dir;		/* -d<dir> */
	char	*dest;		/* -e<file/dir> */
	char	*results;	/* -r<results files> */
	char	*xattr;		/* -x<extended attribute> */
	char	*value;		/* -v<value for extended attribute */
} Option_s;

void usage(void);
void punyopt(
	int  argc,
	char *argv[],
	bool (*myfun)(int c),	/* Function to call to process extra
				 * option specified by 'c'
				 */
	char *myoptions);	/* Extra options used by this test
				 * These options can override puny flags.
				 */

extern Option_s Option;

#endif
