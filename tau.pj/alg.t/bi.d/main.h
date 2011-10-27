/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _MAIN_H_
#define _MAIN_H_ 1

#include <style.h>

typedef struct Option_s {
	int iterations;
	int level;
	bool debug;
	bool print;
} Option_s;

extern Option_s Option;

void test_bb(int n);
void test_ibi(int n);
void test_ibi_level(int n, int level);
void test_bb_level(int n, int level);
void test_rb_level(int n, int level);
void test_bi_level(int n, int level);
void test_jsw_level(int n, int level);
void test_bt(int n);
void test_bt_level(int n, int level);
void test_bt_find(int n, int level);

#endif
