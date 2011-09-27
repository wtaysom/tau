/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Header for simple utilities for writing tests.
 */

#ifndef _UTIL_H_
#define _UTIL_H_ 1

#include <where.h>

enum { NAME_LEN = 10 };  /* Length of generated names counting nul */

#define PrError(...) PrErrork(WHERE, __VA_ARGS__)
#define CheckFill(b, n, o) CheckFillk(WHERE, b, n, o)
#define CheckEq(b1, b2, n) CheckEqk(WHERE, b1, b2, n)
#define Check(e) ((void)((e) || CheckFailed(WHERE, # e)))

void PrErrork(Where_s w, const char *fmt, ...);
bool CheckFillk(Where_s w, const void *buf, int n, s64 offset);
bool CheckEqk(Where_s w, const void *b1, const void *b2, int n);
bool CheckFailed(Where_s w, const char *e);

void Fill(void *buf, int n, s64 offset);
char *Catstr(char *s, ...);
char *RndName(void);
void CrFile (const char *name, u64 size);


#endif  /* _UTIL_H_ */
