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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#define __USE_ISOC99
#include <ctype.h>
#include <style.h>

#include <debug.h>

#include <mdb.h>

/* global.h */

#define BSIZE	128
#define NONE	-1
#define EOS		'\0'

enum { NUM = 256, ID, DONE };

struct Entry_s {
	char	*lexptr;
	int		token;
};

struct Entry_s Symtable[];

void parse  (void);
void expr   (void);
void term   (void);
void factor (void);
void match  (int t);
void emit   (int t, u64 tval);
int lookup  (char s[]);
int insert  (char s[], int tok);

u64 pop (void);
void push (u64 x);

void add        (void);
void sub        (void);
void mul        (void);
void divide     (void);
void mod        (void);
void neg        (void);
void not        (void);
void and        (void);
void or         (void);
void xor        (void);
void leftShift  (void);
void rightShift (void);
void qrand      (void);

char	Lexbuf[BSIZE];
int	Lineno = 1;
u64	Tokenval = NONE;
char	*Expression;
char	*Next;

static jmp_buf	ExprJmpBuf;

#define ERROR(_m)	error(__func__, __LINE__, _m)

static void error (const char *fn, int line, char *m)
{
	printf("ERROR %s<%d> input %d: %s\n", fn, line, Lineno, m);
	longjmp(ExprJmpBuf, 1);
}

/* lexer.c */

int nextchar (void)
{
	return *Next++;
}

void prevchar (void)
{
	--Next;
}

void getval (void)
{
	static char	id[BSIZE];
	char	c;
	char	*r;

	r = id;
	for (c = nextchar(); isalnum(c) || (c == '_'); c = nextchar()) {
		*r++ = c;
	}
	*r = 0;
	prevchar();
	Tokenval = strtoull(id, &r, 16);
	if (!*r) return;

	Tokenval = sym_address(findsym(id));
	if (Tokenval == 0) ERROR(id);
}

int lexan (void)
{
	int	t;

	for (;;) {
		t = nextchar();
		if (!t) {
			return DONE;
		} else if (isblank(t)) {
		} else if (t == '\n') {
			++Lineno;
		} else if (isalnum(t) || (t == '_')) {
			prevchar();
			getval();
			return NUM;
		} else if (isalpha(t)) {
			int	p, b = 0;
			while (isalnum(t)) {
				Lexbuf[b] = t;
				t = nextchar();
				++b;
				if (b >= BSIZE) ERROR("compiler error");
			}
			Lexbuf[b] = EOS;
			if (t != EOF) ungetc(t, stdin);
			p = lookup(Lexbuf);
			if (p == 0) p = insert(Lexbuf, ID);
			Tokenval = p;
			return Symtable[p].token;
		} else {
			Tokenval = NONE;
			return t;
		}
	}
}

/* parse.c */

int	Lookahead;

u64 eval (char *p)
{
	u64	x;

	Expression = Next = p;
	if (setjmp(ExprJmpBuf)) {
		return 0;
	}
	parse();
	x = pop();
	if (Debug) printf("eval=%llx\n", x);
	return x;
}

void parse (void)
{
	Lookahead = lexan();

	if (Lookahead != DONE) {
		expr();
	}
}

void expr (void)
{
	int	t;

	term();
	for (;;) {
		switch (Lookahead) {
			case '+':
			case '-':
				t = Lookahead;
				match(Lookahead); term(); emit(t, NONE);
				continue;
			default:
				return;
		}
	}
}

void term (void)
{
	int	t;

	factor();
	for (;;) {
		switch (Lookahead) {
			case '*':
			case '/':
			case '%':
				t = Lookahead;
				match(Lookahead); factor(); emit(t, NONE);
				continue;
			default:
				return;
			}
	}
}

void factor()
{
	switch (Lookahead) {
		case '(':
			match('('); expr(); match(')');
			break;
		case NUM:
			emit(NUM, Tokenval); match(NUM);
			break;
		case ID:
			emit(ID, Tokenval); match(ID);
			break;
		default:
			ERROR("syntax error");
			break;
	}
}

void match (int t)
{
	if (Lookahead == t) Lookahead = lexan();
	else ERROR("syntax error");
}

/* emitter.c */

void emit (int t, u64 tval)
{
	switch (t) {
		case '+':	add(); break;
		case '-':	sub(); break;
		case '*':	mul();  break;
		case '/':	divide(); break;
		case NUM:
			push(tval);
			break;
		case ID:
			printf("%s\n", Symtable[tval].lexptr);
			break;
		default:
			printf("token %d, Tokenval %llu\n", t, tval);
			break;
	}
}

/* symbol.c */

#define STRMAX	999
#define SYMMAX	100

char		Lexemes[STRMAX];
int		Lastchar = -1;
struct Entry_s	Symtable[SYMMAX];
int		Lastentry = 0;

int lookup (char s[])
{
	int	p;

	for (p = Lastentry; p > 0; --p) {
		if (strcmp(Symtable[p].lexptr, s) == 0) return p;
	}
	return 0;
}

int insert (char s[], int tok)
{
	int	len;

	len = strlen(s);
	if (Lastentry + 1 >= SYMMAX) ERROR("symbol table full");
	if (Lastchar + len + 1 >= STRMAX) ERROR("lexemes array full");
	++Lastentry;
	Symtable[Lastentry].token = tok;
	Symtable[Lastentry].lexptr = &Lexemes[Lastchar + 1];
	Lastchar = Lastchar + len + 1;
	strcpy(Symtable[Lastentry].lexptr, s);
	return Lastentry;
}

struct Entry_s Keywords[] = {
	{ 0,		0 }
};

void initeval (void)
{
	struct Entry_s	*p;

	for (p = Keywords; p->token; ++p) {
		insert(p->lexptr, p->token);
	}
}
