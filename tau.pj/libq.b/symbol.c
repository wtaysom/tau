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

#ifdef __APPLE__
#include <malloc/malloc.h>
#endif

#include <style.h>
#include <crc.h>
#include <eprintf.h>
#include <mylib.h>
#include <symbol.h>

/*
 * Primes from planetmath.org
 * Not sure what other primes belong to this set but possible candidates:
 * 3 7 13 23
 */
unint Primes[] = {
	53,
	97,
	193,
	389,
	769,
	1543,
	3079,
	6151,
	12289,
	24593,
	49157,
	98317,
	196613,
	393241,
	786433,
	1572869,
	3145739,
	6291469,
	12582917,
	25165843,
	50331653,
	100663319,
	201326611,
	402653189,
	805306457,
	1610612741,
	0};

#define HASH(_x)	((_x) % Hash)
//#define UHASH(_x)	(((_x) & 0xff) + 1)
#define STEP(_x)       (((_x) % Step) + 1)

const char	**Bucket;
unint	Num_symbols;
unint	Hash;
unint	Step;
unint	*NextPrime;

char	*Space;
unint	Free_space;
unint	Alloc_size = (1<<16);
unint	Total_space;

unint	Collisions;
unint	Duplicates;

static void init_symbol (void)
{
	if((saddr)&Bucket < 0) {
		eprintf("Negative\n");
	} else {
		printf("Positive\n");
	}
	NextPrime = Primes;
	Step = *NextPrime++;
	Hash = *NextPrime++;
	Bucket = ezalloc(sizeof(char *) * Hash);
}

static void move_symbol (const char *s)
{
	unint	x = hash_string_32(s);
	unint	h;
	unint	u;

	h = HASH(x);
	u = STEP(x);
	while (Bucket[h]) {
		h += u;
		if (h >= Hash) h %= Hash;
	}
	Bucket[h] = s;
}

static void grow_symbol (void)
{
	const char	**old_bucket = Bucket;
	const char	**b;
	unint	old_hash = Hash;

	if (!*NextPrime) eprintf("Out of Primes");

	Step = Hash;
	Hash = *NextPrime++;
	Bucket = ezalloc(sizeof(char *) * Hash);
	for (b = old_bucket; b < &old_bucket[old_hash]; b++) {
		if (*b) move_symbol(*b);
	}
	free(old_bucket);
}

static const char *save_symbol (const char *s)
{
	const char	*sym;
	unint		len;

	len = strlen(s) + 1;
	if (len > Free_space) {
		while (len > Alloc_size / 16) {
			Alloc_size <<= 2;
		}
		//Alloc_size = malloc_good_size(Alloc_size);
		Space = emalloc(Alloc_size);
		#ifdef __APPLE__
			Free_space = malloc_size(Space);
		#else
			Free_space = Alloc_size;
		#endif
		Total_space += Free_space;
	}
	memcpy(Space, s, len);
	sym = Space;
	Space += len;
	Free_space -= len;
	return sym;
}

symbol_t symbol (const char *s)
{
	unint	x = hash_string_32(s);
	unint	h;
	unint	u;
	unint	len;
	const char	*t;

	len = strlen(s);
	if (!Bucket) init_symbol();
	//if (Num_symbols >= (Hash / 10) * 8) {
	//if (Num_symbols >= (Hash / 10) * 9) {
	//if (Num_symbols >= Hash) {
	if (Num_symbols >= (Hash / 10) * 9) {
		grow_symbol();
	}
	h = HASH(x);
	u = STEP(x);
	while (Bucket[h]) {
		if (strcmp(Bucket[h], s) == 0) {
			++Duplicates;
			return (symbol_t)Bucket[h];
		}
		//printf("Collision %s %s %ld %ld\n", Bucket[h], s, h, u);
		++Collisions;
		h += u;
		if (h >= Hash) h %= Hash;
	}
	++Num_symbols;
	t = save_symbol(s);
	Bucket[h] = t;
	return (symbol_t)t;
}

void stat_symbol (void)
{
	printf("stat:\n"
		"\tTotalSpace=%lu\n"
		"\tNum Symbols=%lu\n"
		"\tCollisons=%lu\n"
		"\tDuplicates=%lu\n",
		Total_space, Num_symbols, Collisions, Duplicates);
}

void dump_symbol (void)
{
	const char	**b;

	printf("dump:\n");
	for (b = Bucket; b < &Bucket[Hash]; b++) {
		if (*b) {
			printf("\t%s\n", *b);
		}
	}
}
