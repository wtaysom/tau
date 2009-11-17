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

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#define STSIZE	8
#define MAX_MEM	8

#define HEX		0
#define SIGNED		1
#define UNSIGNED	2
#define OCTAL		3
#define CHARACTER	4

typedef unsigned long	ulong;
typedef unsigned	index_t;
typedef unsigned	flag_t;

void initStack (void);
ulong pop (void);
void push (ulong x);
void swap (void);
void rotate (void);
void dup (void);
ulong ith (index_t i);

void add (void);
void sub (void);
void mul (void);
void divide (void);
void mod (void);
void neg (void);
void not (void);
void and (void);
void or  (void);
void xor (void);
void leftShift (void);
void rightShift (void);
void lrand (void);

void output (flag_t newNumber);
void input (void);

void finish (int sig);
void setSignals (void);
void init (void);

void initMemory (void);
void store (void);
void retrieve (void);
