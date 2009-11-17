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

#include <stdio.h>
#include <unistd.h>
#include <style.h>
#include <eprintf.h>

/*
 * linecnt counts the number of ';', '#', '}', and ',' in a program.
 * This is a first approximation on the number of lines of
 * code.  linecnt reads stdin and writes stdout.
 */
unsigned	Semi = 0;
unsigned	Rightcurly = 0;
unsigned	Pound = 0;
unsigned	Newline = 0;
unsigned	Comma = 0;
unsigned	Total = 0;

bool	Pr_comma      = TRUE;
bool	Pr_pound      = TRUE;
bool	Pr_rightcurly = TRUE;
bool	Pr_semi       = TRUE;

void pr (bool print, unsigned count, unsigned *total, bool *plus)
{
	if (print) {
		if (*plus) printf(" +");
		else *plus = TRUE;
		printf(" %5u", count);
		*total += count;
	}
}

void print (
	char	*label,
	unsigned semi,
	unsigned rightcurly,
	unsigned pound,
	unsigned comma,
	unsigned newline)
{
	unsigned total = 0;
	bool	 plus  = FALSE;

	pr(Pr_semi      , semi      , &total, &plus);
	pr(Pr_rightcurly, rightcurly, &total, &plus);
	pr(Pr_pound     , pound     , &total, &plus);
	pr(Pr_comma     , comma     , &total, &plus);
	if (plus) printf(" = %4u", total);
	else      printf("       ");
	printf(" %4u", newline);
	if (label) printf(" %s", label);
	printf("\n");
}

void linecnt (FILE *in, char *name)
{
	int		c;
	unsigned	semi       = 0;
	unsigned	rightcurly = 0;
	unsigned	pound      = 0;
	unsigned	comma      = 0;
	unsigned	newline    = 0;

	for (;;) {
		c = getc(in);
		if (c == EOF) break;

		switch (c) {
		case ';':	++semi;		break;
		case '}':	++rightcurly;	break;
		case '#':	++pound;	break;
		case ',':	++comma;	break;
		case '\n':	++newline;	break;
		}
	}
	Semi       += semi;
	Rightcurly += rightcurly;
	Pound      += pound;
	Comma      += comma;
	Newline    += newline;

	print(name, semi, rightcurly, pound, comma, newline);
}

void usage (void)
{
	pr_usage("-cprs? [<files>]\n"
		"\tc - exclude commas             ','\n"
		"\tp - exclude pound sign         '#'\n"
		"\tr - exclude right curly braces '{'\n"
		"\ts - exclude semicolons         ';'\n"
		"\t? - usage\n"
		"Output: semicolons + right curlies + pounds + commas = total"
		" newlines <file name>\n"
		);
}

int main (int argc, char *argv[])
{
	FILE	*in;
	int	c;
	int	i;

	setprogname(argv[0]);
	while ((c = getopt(argc, argv, "srpc?")) != -1) {
		switch (c) {
		case 'c':	Pr_comma      = FALSE;	break;
		case 'p':	Pr_pound      = FALSE;	break;
		case 'r':	Pr_rightcurly = FALSE;	break;
		case 's':	Pr_semi       = FALSE;	break;
		case '?':
		default:	usage();	break;
		}
	}
	if (!argv[optind]) {
		linecnt(stdin, NULL);
		return 0;
	}
	for (i = optind; i < argc; i++) {
		in = fopen(argv[i], "r");
		if (!in) {
			fprintf(stderr, "Couldn't open %s\n", argv[i]);
			continue;
		}
		linecnt(in, argv[i]);
		fclose(in);
	}
	if (i > optind) {
		print("Total", Semi, Rightcurly, Pound, Comma, Newline);
	}
	return 0;
}
