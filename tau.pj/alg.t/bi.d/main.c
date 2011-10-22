/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <crc.h>
#include <debug.h>
#include <eprintf.h>
#include <mystdlib.h>
#include <style.h>

#include "bitree.h"
#include "lump.h"
#include "main.h"
#include "util.h"

Option_s Option = {
	.iterations = 3000,
	.level = 150,
	.debug = FALSE,
	.print = FALSE };

void usage(void)
{
	pr_usage("[-dhp] [-i<iterations>] [-l<level>]\n"
		"\t-d - turn on debugging [%s]\n"
		"\t-h - print this help message\n"
		"\t-i - num iterations [%d]\n"
		"\t-l - level of records to keep [%d]\n"
		"\t-p - turn on printing [%s]\n",
		Option.debug ? "on" : "off",
		Option.iterations,
		Option.level,
		Option.print ? "on" : "off");
}

void myoptions(int argc, char *argv[])
{
	int c;

	fdebugoff();
	setprogname(argv[0]);
	setlocale(LC_NUMERIC, "en_US");
	while ((c = getopt(argc, argv, "dhpi:l:")) != -1) {
		switch (c) {
		case 'h':
		case '?':
			usage();
			break;
		case 'd':
			Option.debug = TRUE;
			fdebugon();
			break;
		case 'i':
			Option.iterations = strtoll(optarg, NULL, 0);
			break;
		case 'l':
			Option.level = strtoll(optarg, NULL, 0);
			break;
		case 'p':
			Option.print = TRUE;
			break;
		default:
			fatal("unexpected option %c", c);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	myoptions(argc, argv);
	test_jsw_level(Option.iterations, Option.level);
	test_bt_level(Option.iterations, Option.level);
	test_jsw_level(Option.iterations, Option.level);
	//test_ibi_level(Option.iterations, Option.level);
	return 0;
}

#if 0
	test_ibi(Option.iterations);
	test_bb(Option.iterations);

	test_ibi_level(Option.iterations, Option.level);
	test_bb_level(Option.iterations, Option.level);
	test_perf(Option.iterations, Option.level);
	test_rb(Option.iterations, Option.level);
	test_bi_delete(Option.iterations);
	test_bi_level(Option.iterations, Option.level);
	test_jsw_level(Option.iterations, Option.level);
	test_bi_level(Option.iterations, Option.level);
	test_ibi_level(Option.iterations, Option.level);
	test_jsw_level(Option.iterations, Option.level);
	test_bt(Option.iterations);
	test_bt_level(Option.iterations, Option.level);
#endif
