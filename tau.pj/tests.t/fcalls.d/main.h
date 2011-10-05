/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _MAIN_H_
#define _MAIN_H_ 1

typedef struct LocalOption_s {
	s64 size_big_file;   /* Size of the "Big File" in bytes */
	snint block_size;    /* Block size and buffer size */
	bool exit_on_error;  /* Exit on unexpected errors */
	bool stack_trace;    /* Print a stack trace on error */
	bool is_root;        /* TRUE if effective uid is root */
	bool flaky;          /* Run flaky tests */
	bool verbose;        /* Print every fcall */
	bool seed_rand;      /* Seed random numbers */
	bool test_sparse;    /* Run tests requiring sparse files */
	bool test_direct;    /* Run tests using O_DIRECT */
} LocalOption_s;

extern LocalOption_s Local_option;

extern char BigFile[];
extern char EmptyFile[];
extern char OneFile[];

void DumpRecords(void);

#endif
