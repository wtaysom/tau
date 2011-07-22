/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Header for simple utilities for writing tests.
 */

#ifndef _UTIL_H_
#define _UTIL_H_ 1

#include <where.h>

#define PrError(_fmt, ...) PrErrorp(WHERE, _fmt, __VA_ARGS__)
#define IsSame(_b, _n, _o) IsSamep(WHERE, _b, _n, _o)

void PrErrorp(Where_s w, const char *fmt, ...);
bool IsSamep(Where_s w, void *buf, int n, s64 offset);

void Fill(void *buf, int n, s64 offset);
char *Mkstr(char *s, ...);
char *RndName(unsigned n);


#endif  /* _UTIL_H_ */
