/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

/*
 * punyopt - uses getopt to provide a consistent set of options
 *	for the puny benchmarks.
 */

#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <eprintf.h>
#include <mylib.h>
#include <puny.h>

Option_s Option = {
	.iterations  = 10000,
	.loops       = 2,
	.numthreads  = 7,
	.sleep_secs  = 1,
	.file_size   = 1457,
	.name_size   = 17,
	.print       = FALSE,
	.file        = "_test.out",
	.dir         = "_dir",
	.dest        = "_dest",
	.results     = NULL,	/* NULL -> use stdout */
	.xattr       = "attribute",
	.value       = "value" };

static char Default[] = "hpf:d:e:i:l:n:s:t:v:x:z:?";

static void pr_cmd_line (int argc, char *argv[])
{
	int	i;

	printf("\n%s", argv[0]);
	for (i = 1; i < argc; i++) {
		printf(" %s", argv[i]);
	}
	printf("\n");
}

void punyopt (
	int  argc,
	char *argv[],
	void (*myfun)(int c),
	char *myoptions)
{
	char	*options;
	int	c;

	pr_cmd_line(argc, argv);

	if (myoptions) {
		options = emalloc(strlen(myoptions) + strlen(Default) + 1);
		cat(options, Default, myoptions, NULL);
	} else {
		options = Default;
	}
	setprogname(argv[0]);
	setlocale(LC_NUMERIC, "en_US");
	while ((c = getopt(argc, argv, options)) != -1) {
		switch (c) {
		case 'h':
		case '?':
			usage();
			break;
		case 'f':
			Option.file = optarg;
			break;
		case 'd':
			Option.dir = optarg;
			break;
		case 'e':
			Option.dest = optarg;
			break;
		case 'i':
			Option.iterations = strtoll(optarg, NULL, 0);
			break;
		case 'l':
			Option.loops = strtoll(optarg, NULL, 0);
			if (Option.loops == 0) {
				Option.loops = LLONG_MAX;
			}
			break;
		case 'n':
			Option.name_size = strtoll(optarg, NULL, 0);
			break;
		case 'p':
			Option.print = TRUE;
			break;
		case 'r':
			Option.results = optarg;
			break;
		case 's':
			Option.sleep_secs = strtoll(optarg, NULL, 0);
			break;
		case 't':
			Option.numthreads = strtoll(optarg, NULL, 0);
			break;
		case 'v':
			Option.value = optarg;
			break;
		case 'x':
			Option.xattr = optarg;
			break;
		case 'z':
			Option.file_size = strtoll(optarg, NULL, 0);
			break;
		default:
			if (myfun) {
				myfun(c);
			} else {
				usage();
			}
			break;
		}
	}
}
