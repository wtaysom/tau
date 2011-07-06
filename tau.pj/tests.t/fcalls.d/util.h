/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

/*
 * Header for simple utilities for writing tests.
 */

#ifndef _UTIL_H_
#define _UTIL_H_ 1

#ifndef _FCALLS_H_
#include <fcalls.h>
#endif

#define error(_fmt, ...) pr_error(HERE_PRM, _fmt, ## __VA_ARGS__)
void pr_error(HERE_DCL, const char *fmt, ...);

void fill(void *buf, int n, int seed);
bool is_same(void *buf, int n, int seed);
char *mkstr(char *s, ...);
char *rndname(unsigned n);


#endif
