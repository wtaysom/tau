/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/

#ifndef _MYLIB_H_
#define _MYLIB_H_ 1

#ifndef _STDDEF_H
#include <stddef.h>
#endif

#ifndef _STYLE_H_
#include <style.h>
#endif

void *myalloc (unsigned size);
void *myrealloc (void *p, unsigned size);
char *strAlloc (char *s);
void strFree (char *s);
char *cat (char *dst, ...);

size_t strlcpy(char *dest, const char *src, size_t size);
size_t strlcat(char *dest, const char *src, size_t count);
 
unsigned int murmurhash (const unsigned char *data, int len, unsigned int h);

void seed_random (void);
long range (long max);
int percent (int x);
long exp_dist (long range);

int isPattern (char *string);
int isMatch (char *pattern, char *string);

double current_time (void);
void prTimer (void);
void startTimer (void);
void stopTimer (void);
void resetTimer (void);

void init_sum (void);
void q_sum (long long x);
void f_sum (double x);
void pr_sum (char *msg);
void pr_sum_min_max (char *msg);

char *date (unsigned long time);

u64 findprime (u64 x);

void donothing (void);

void to_base64   (const void *data, int length, char *out);
int  from_base64 (const char *in, void *data);

#endif
