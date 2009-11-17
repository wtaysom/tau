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

#ifndef _SYMS_H_
#define _SYMS_H_

#ifndef _STYLE_H_
#include <style.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void  initsyms (char *name);
void  dumpsyms ();
u64   findsym  (char *name);
char *findname (addr address, addr *offset);
void  start_search ();
char *search (char *pattern);

void initkmem (char *name);
int readkmem (addr address, void *data, unint size);
int writekmem (addr address, void *buf, int numBytes);


#ifdef __cplusplus
}
#endif

#endif
