/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Header for simple utilities for writing tests.
 */

#ifndef _UTIL_H_
#define _UTIL_H_ 1

#include <where.h>

#define PrError(...) PrErrork(WHERE, __VA_ARGS__)
#define IsSame(b, n, o) IsSamek(WHERE, b, n, o)
#define IsEq(b1, b2, n) IsEqk(WHERE, b1, b2, n)

void PrErrork(Where_s w, const char *fmt, ...);
bool IsSamek(Where_s w, const void *buf, int n, s64 offset);
bool IsEqk(Where_s w, const void *b1, const void *b2, int n);

void Fill(void *buf, int n, s64 offset);
char *Mkstr(char *s, ...);
char *RndName(unsigned n);
void CrFile (const char *name, u64 size);


#endif  /* _UTIL_H_ */
