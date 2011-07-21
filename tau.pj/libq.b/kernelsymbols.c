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

/*
 * Read in symbol table from /proc/kallsyms
 *	format: <address> <type> <name>
 * Sort them by both address and name
 * Provide look up routines
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <style.h>
#include <mystdlib.h>
#include <qsort.h>
#include <kernelsymbols.h>

enum {
	EOL = '\n',
	MAX_LINE = 2048,
	MAX_ARGS = 128,
	MIN_SYMS = 1 /* Should be bigger but good for testing*/};

struct Sym_s {
	u64	s_addr;
	char	s_type;
	char	s_name[0];
};

static Sym_s	**AddrTable;
static Sym_s	**NameTable;

static FILE	*File;

static char	Line[MAX_LINE];
static char	*Argv[MAX_ARGS];
static char	*Arg;
static char	*Next;
static int	Argc;
static bool	Done = FALSE;
static bool	Quit = FALSE;

static void skip_spaces (void)
{
	int	c;

	for (;;) {
		c = getc(File);
		if (c == EOF) {
			Quit = TRUE;
			Done = TRUE;
			return;
		}
		if (c == EOL) {
			Done = TRUE;
			return;
		}
		if (!isspace(c)) {
			ungetc(c, File);
			return;
		}
	}
}

static void get_arg (void)
{
	int	c;

	Arg = Next;
	for (;;) {
		c = getc(File);
		if (c == EOF) {
			Done = TRUE;
			Quit = TRUE;
			break;
		}
		if (c == EOL) {
			Done = TRUE;
			break;
		}
		if (isspace(c)) {
			break;
		}
		*Next++ = c;
	}
	if (Arg != Next) {
		Argv[Argc++] = Arg;
		*Next++ = '\0';
	}
}

static void read_line (void)
{
	Argc = 0;
	Next = Line;

	while (!Done) {
		skip_spaces();
		if (Done) break;
		get_arg();
	}
	Done = FALSE;
}

static void error (char *msg)
{
	fprintf(stderr, "ERROR: %s\n", msg);
	exit(1);
}

static unint	NumSyms = 0;
static unint	MaxSyms = 0;

static void storesym (Sym_s *sym)
{
	Sym_s	**newsyms;

	if (NumSyms >= MaxSyms) {
		if (AddrTable) {
			MaxSyms <<= 1;
			newsyms = realloc(AddrTable, MaxSyms * sizeof(Sym_s *));
		} else {
			MaxSyms = MIN_SYMS;
			newsyms = malloc(MIN_SYMS * sizeof(Sym_s *));
		}
		if (newsyms) {
			AddrTable = newsyms;
		} else {
			error("Out of memory");
		}
	}
	AddrTable[NumSyms++] = sym;
}

static void savesyms (u64 addr, char type, char *name)
{
	Sym_s	*sym;

	sym = malloc(sizeof(Sym_s) + strlen(name) + 1);
	if (sym == NULL) {
		error("out of memory");
	}
	sym->s_addr = addr;
	sym->s_type = type;
	strcpy(sym->s_name, name);
	storesym(sym);
}

static void opensyms (char *file_name)
{
	File = fopen(file_name, "r");
	if (!File) {
		perror(file_name);
		exit(errno);
	}
}

static bool valid (char type)
{
	return type != 'U';
}

static void readsyms (void)
{
	u64	addr;
	char	type;

	for (;;) {
		read_line();
		if (Quit) break;
		if ((Argc < 3) || (Argc > 4)) {
			fprintf(stderr, "didn't read as much as I thought we should\n");
			continue;
		}
		addr = strtoull(Argv[0], NULL, 16);
		type = Argv[1][0];
		if (addr && valid(type)) {
			savesyms(addr, type, Argv[2]);
		}
	}
}

static int compare_sym_addr (const Sym_s *r, const Sym_s *s)
{
	if (r->s_addr < s->s_addr) return -1;
	if (r->s_addr == s->s_addr) return 0;
	return 1;
}

static int compare_key_addr (const addr key, const Sym_s *s)
{
	if (key < s->s_addr) return -1;
	if (key == s->s_addr) return 0;
	return 1;
}

static int compare_sym_name (const Sym_s *r, const Sym_s *s)
{
	return strcmp(r->s_name, s->s_name);
}

static int compare_key_name (const char *key, const Sym_s *s)
{
	return strcmp(key, s->s_name);
}

void sortsyms (void)
{
	quickSort((vector_t)AddrTable, NumSyms, (cmp_f)compare_sym_addr);
	NameTable = malloc(NumSyms * sizeof(Sym_s *));
	if (!NameTable) error("Out of Memory");
	memcpy(NameTable, AddrTable, NumSyms * sizeof(Sym_s *));
	quickSort((vector_t)NameTable, NumSyms, (cmp_f)compare_sym_name);
}

void dumpsyms (void)
{
	Sym_s	*sym;
	unint	i;

	printf("    addr t name\n");
	for (i = 0; i < NumSyms; ++i) {
		sym = AddrTable[i];
		printf("%8llx %c %s\n", sym->s_addr, sym->s_type, sym->s_name);
	}
	printf("    addr t name\n");
	for (i = 0; i < NumSyms; ++i) {
		sym = NameTable[i];
		printf("%8llx %c %s\n", sym->s_addr, sym->s_type, sym->s_name);
	}
}

void initsyms (void)
{
	static bool	inited = FALSE;

	if (inited) return;
	inited = TRUE;

	opensyms("/proc/kallsyms");
	readsyms();
	sortsyms();
//	dumpsyms();
}

Sym_s *findsym (char *name)
{
	Sym_s	*sym;

	sym = binarySearch(name, (void **)NameTable, NumSyms,
				(cmp_f)compare_key_name, NULL);
	return sym;
}

Sym_s *findname (addr address)
{
	Sym_s	*sym;
	int	prev;

	sym = binarySearch((void *)address, (void **)AddrTable, NumSyms,
				(cmp_f)compare_key_addr, &prev);
	if (sym) {
		return sym;
	} else if ((prev == 0) || (prev >= NumSyms - 1)) {
		return NULL;
	} else {
		return AddrTable[prev];
	}
}

Sym_s *nextaddress (Sym_s *sym)
{
	unint	i;

	for (i = 0; i < NumSyms; i++) {
		if (AddrTable[i] == sym) {
			if (++i == NumSyms) return NULL;
			return AddrTable[i];
		}
	}
	return NULL;
}

Sym_s *prevaddress (Sym_s *sym)
{
	unint	i;

	for (i = 0; i < NumSyms; i++) {
		if (AddrTable[i] == sym) {
			if (i-- == 0) return NULL;
			return AddrTable[i];
		}
	}
	return NULL;
}

Sym_s *nextname (Sym_s *sym)
{
	unint	i;

	for (i = 0; i < NumSyms; i++) {
		if (NameTable[i] == sym) {
			if (++i == NumSyms) return NULL;
			return AddrTable[i];
		}
	}
	return NULL;
}

static unint	CurrentSym;

void start_search (void)
{
	CurrentSym = 0;
}

Sym_s *search (char *pattern)
{
	Sym_s	*sym;

	while (CurrentSym < NumSyms) {
		sym = NameTable[CurrentSym++];
		if (isMatch(pattern, sym->s_name)) {
			return sym;
		}
	}
	return NULL;
}

char *sym_name (Sym_s *sym)
{
	return sym ? sym->s_name : NULL;
}

u64 sym_address (Sym_s *sym)
{
	return sym ? sym->s_addr : 0;
}

#if 0
/*
 * Sort by name too
 */

int main (int argc, char *argv[])
{
	Sym_s	*sym;

	printf("sizeof(Sym_s)=%lu name=%lu\n", sizeof(Sym_s), sizeof(sym->s_name));
	initsyms();
	return 0;
}
#endif
