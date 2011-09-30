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

extern void test_ibi(int n, int level);
extern void test_bb(int n, int level);
extern void test_rb(int n, int level);

#endif
