/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Simple utilities for writing tests. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crc.h>
#include <debug.h>
#include <eprintf.h>
#include <mystdlib.h>
#include <style.h>
#include <util.h>

#include "fcalls.h"

/* PrErrorp prints an error message including location */
void PrErrorp (Where_s w, const char *fmt, ...) {
  int lasterr = errno;
  va_list args;

  fflush(stdout);
  fprintf(stderr, "ERROR ");
  if (getprogname() != NULL) {
    fprintf(stderr, "%s ", getprogname());
  }
  fprintf(stderr, "%s:%s<%d> ", w.file, w.fn, w.line);
  va_start(args, fmt);
  if (fmt) {
    vfprintf(stderr, fmt, args);

    if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':') {
      fprintf(stderr, " %s<%d>",
        strerror(lasterr), lasterr);
    }
  }
  va_end(args);
  fprintf(stderr, "\n");
  if (StackTrace) stacktrace_err();
  if (Fatal) exit(2); /* conventional value for failed execution */
}

/* RndName generates a random name. The caller must free the returned name */
char *RndName (unsigned n) {
  static char namechar[] = "abcdefghijklmnopqrstuvwxyz"
         "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
         "_";
  char *s;
  unint i;

  if (!n) return NULL;

  s = emalloc(n);
  for (i = 0; i < n - 1; i++) {
    s[i] = namechar[urand(sizeof(namechar) - 1)];
  }
  s[i] = '\0';
  return s;
}

/* Mkstr creates a string by concatenating all the string
 * arguments. The last string must be NULL.
 * Caller must free the returned pointer.
 * O(n^2), so don't pass too many strings.
 */
char *Mkstr (char *s, ...) {
  char *t;
  char *u;
  int sum;
  va_list args;

  if (!s) return NULL;

  sum = strlen(s);
  va_start(args, s);
  for (;;) {
    t = va_arg(args, char *);
    if (!t) break;
    sum += strlen(t);
  }
  va_end(args);

  u = ezalloc(sum + 1);
  strcat(u, s);
  va_start(args, s);
  for (;;) {
    t = va_arg(args, char *);
    if (!t) break;
    strcat(u, t);
  }
  va_end(args);
  return u;
}

/* Generate a 8 bit hash of a 64 bit number */
static inline u8 Hash (s64 x) {
  return crc32( &x, sizeof(x));
}

/* Fill a buffer of size n with bytes starting with a seed */
void Fill (void *buf, int n, s64 offset) {
  u8 *b = buf;
  u8 *end = &b[n];

  for (; b < end; b++) {
    *b = Hash(offset);
    ++offset;
  }
}

/* IsSamep: does the buffer have the same content that was set by Fill */
bool IsSamep (Where_s w, void *buf, int n, s64 offset) {
  u8 *b = buf;
  u8 *end = &b[n];

  for (; b < end; b++) {
    if (*b != Hash(offset)) {
      PrErrorp(w,
        "IsSame at offset %td expected 0x%2x"
        " but is 0x%2x",
        b - (u8 *)buf, Hash(offset), *b);
      return FALSE;
    }
    ++offset;
  }
  return TRUE;
}
