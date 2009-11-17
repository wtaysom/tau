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

#ifndef _KERNELSYMBOLS_H_
#define _KERNELSYMBOLS_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Sym_s	Sym_s;

void   initsyms     (void);
void   dumpsyms     (void);
Sym_s *findsym      (char *name);
Sym_s *findname     (addr address);
Sym_s *nextaddress  (Sym_s *sym);
Sym_s *prevaddress  (Sym_s *sym);
Sym_s *nextname     (Sym_s *sym);
void   start_search (void);
Sym_s *search       (char *pattern);
char  *sym_name     (Sym_s *sym);
u64    sym_address  (Sym_s *sym);

#ifdef __cplusplus
}
#endif

#endif
