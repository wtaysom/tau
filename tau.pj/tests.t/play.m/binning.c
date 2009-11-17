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
#include <stdlib.h>
#include <uuid/uuid.h>

#include <style.h>
#include <eprintf.h>
#include <debug.h>
#include <q.h>
#include <mylib.h>
#include <set.h>

void check (int nbags, int nreps, int nshards, int a[nreps][nshards])
{
	int	*b;
	int	i, j;

	b = ezalloc(nbags * sizeof(int));
	for (i = 0; i < nreps; i++) {
		for (j = 0; j < nshards; j++) {
			++b[a[i][j]];
		}
	}
	for (i = 0; i < nbags; i++) {
		printf("%d. %d\n", i, b[i]);
	}
}

void prbags (int nbags, int nreps, int nshards, int a[nreps][nshards])
{
	int	i, j, k;

	for (i = 0; i < nbags; i++) {
		printf("%d.", i);
		for (j = 0; j < nreps; j++) {
			for (k = 0; k < nshards; k++) {
				if (i == a[j][k]) {
					printf(" %3d", k);
				}
			}
		}
		printf("\n");
	}
}

void prshards (int nreps, int nshards, int a[nreps][nshards])
{
	int	i, j;

	for (i = 0; i < nshards; i++) {
		printf("%3d.", i);
		for (j = 0; j < nreps; j++) {
			printf(" %2d", a[j][i]);
		}
		printf("\n");
	}
}

#if 0
void prime_bin (int nbags, int nshards, int nreps)
{
	int	prime   = 1610612741;
	int	bprime  = 805306457;
	int	total;
	int	i;
	int	b;
	int	p;
	int	*a;

	nshards = findprime(nshards);
	printf("nshards=%d\n", nshards);
	total = nreps * nshards;
	a = ezalloc(total * sizeof(int));
	for (i = 0; i < total; i++) {
		a[i] = -1;
	}
	b = 0;
	p = 0;
	for (i = 0; i < total; i++) {
		p += prime;
		p %= total;
		if (a[p] != -1) {
			printf("Already set a[%d]=%d\n", p, a[p]);
			break;
		}
		b += bprime;
		b %= nbags;
		a[p] = b;
	}
	prshards(nbags, nreps, nshards, a);
	check(nbags, nreps, nshards, a);
	prbags(nbags, nreps, nshards, a);
}

void bybag (int nbags, int nshards, int nreps)
{
	int	prime   = 1610612741;
	int	bprime  = 805306457;
	int	total;
	int	i;
	int	j;
	int	b;
	int	p;
	int	*a;

	total = nreps * nshards;
	a = ezalloc(total * sizeof(int));
	for (i = 0; i < total; i++) {
		a[i] = -1;
	}
	b = 0;
	p = 0;
	for (i = 0; i < total; i++) {
		p += prime;
		p %= total;
		if (a[p] != -1) {
			printf("Already set a[%d]=%d\n", p, a[p]);
			break;
		}
		b += bprime;
		b %= nbags;
		a[p] = b;
	}
	for (i = 0; i < nshards; i++) {
		for (j = 0; j < nreps; j++) {
			b = a[i + j*nshards];
			printf(" %u", b);
		}
		printf("\n");
	}
	check(nbags, nreps, nshards, a);
	prbags(nbags, nreps, nshards, a);
}
#endif

void simple (int nbags, int nreps, int nshards, int a[nreps][nshards])
{
	const int prime = 50331653;
	int	i, j;
	int	b;

	b = 0;
	for (i = 0; i < nreps; i++) {
		b = i;
		for (j = 0; j < nshards; j++) {
			b += prime;
			b %= nbags;
			a[i][j] = b;
		}
	}
}

enum { MAX_TRYS = 100 };

static int try (set_s *s, int rep, int shard, int nshards, int a[][nshards])
{
	int	try;
	int	i;
	addr	x;

	for (try = 0; try < MAX_TRYS; try++) {
		if (!rand_pick_from_set(s, &x)) fatal("set empty");
		for (i = 0; ; i++) {
			if (i == rep) return x;
			if (a[i][shard] == x) {
				break;
			}
		}
	}
	return -1;
}

void rand_bin (int nbags, int nreps, int nshards, int a[nreps][nshards])
{
	int	i;
	int	j;
	int	b;
	int	again;
	int	x;
	set_s	s;

	init_set( &s);
	for (again = 0; again < MAX_TRYS; again++) {
		for (b = 0, i = 0; ; i++) {
			if (i == nreps) {
				return;
			}
			reinit_set( &s);
			for (j = 0; j < nshards; j++) {
				add_to_set( &s, b);
				if (++b == nbags) b = 0;
			}
			for (j = 0; j < nshards; j++) {
				x = try(&s, i, j, nshards, a);
				if (x == -1) {
					goto tryagain;
				}
				remove_from_set( &s, x);
				a[i][j] = x;
			}
		}
tryagain:;
	}
	return simple(nbags,  nreps, nshards,a);
}

guid_t *gen_bags (unint nbags)
{
	guid_t	*bags;
	unint	i;

	bags = ezalloc(nbags * sizeof(guid_t));

	for (i = 0; i < nbags; i++) {
		uuid_generate(bags[i]);
	}
	return bags;
}

unint numshards (unint nbags, unint nreps)
{
	int	n = nbags * nreps * 10;
	int	i;

	for (i = 0; (1<<i) < n; i++) {
		if (i == 30) {
			fatal("too many shards=%d nbags=%d nreps=%d\n",
				n, nbags, nreps);
		}
	}
	return 1<<i;
}

char *strguid (guid_t guid)
{
	static char	out[100];

	uuid_unparse(guid, out);
	return out;
}

void pr_guids (int nreps, int nshards, int a[nreps][nshards], guid_t *bags)
{
	
	int	i, j;

	for (i = 0; i < nshards; i++) {
		printf("%d.", i);
		for (j = 0; j < nreps; j++) {
			printf(" %s", strguid(bags[a[j][i]]));
		}
		printf("\n");
	}
}

int main (int argc, char *argv[])
{
	int	nbags   = 10;
	int	nshards = 1<<4;
	int	nreps   = 3;
	int	total;
	void	*a;
	guid_t	*bags;

	if (argc > 1) {
		nbags = atoi(argv[1]);
	}
	if (argc > 2) {
		nreps = atoi(argv[2]);
	}
	if (argc > 3) {
		nshards = atoi(argv[3]);
	}
	if (nreps > nbags) fatal("reps %d > %d bags", nreps, nbags);
	if (!nshards) nshards = numshards(nbags, nreps);

	bags = gen_bags(nbags);
	total = nreps * nshards;
	a = ezalloc(total * sizeof(int));

	rand_bin(nbags, nreps, nshards, a);

	check(nbags, nreps, nshards, a);
	prbags(nbags, nreps, nshards, a);
	prshards(nreps, nshards, a);
	pr_guids(nreps, nshards, a, bags);
	return 0;
}
