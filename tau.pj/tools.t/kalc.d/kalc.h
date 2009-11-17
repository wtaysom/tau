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

#ifndef _STYLE_H_
#include <style.h>
#endif

enum { STSIZE = 8, MAX_MEM = 4 };

typedef enum Base_t {
	HEX, SIGNED, UNSIGNED, CHARACTER
} Base_t;

typedef unsigned	index_t;
typedef unsigned	flag_t;

extern double	Memory[];
extern double	Lastx;
extern int	Format;


double pop    (void);
void   push   (double);
void   swap   (void);
void   rotate (void);
void   dup    (void);
double ith    (index_t);

void add    (void);
void sub    (void);
void mul    (void);
void divide (void);
void power  (void);
void neg    (void);
void dsqrt  (void);
void dsin   (void);
void dcos   (void);
void dtan   (void);
void drand  (void);

void output (flag_t newNumber);
void input  (void);

void finish (int sig);
void setSignals (void);
void init (void);

void initMemory (void);
void initStack  (void);
void store      (void);
void retrieve   (void);
