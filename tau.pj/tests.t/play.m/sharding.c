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
#include <uuid/uuid.h>

#include <style.h>
#include <eprintf.h>
#include <debug.h>

guid_t **pack (
	unint	numbags,
	guid_t	*bag,
	unint	numshards,
	unint	numreplicas)
{
	unint	r;
	unint	s;
	unint	b;
	guid_t	*shard;
	guid_t	**replica;

	if (numreplicas > numbags) {
		fatal("not enough bags %lu > %lu", numreplicas, numbags);
	}
	replica = ezalloc(numreplicas * sizeof(guid_t *));
	for (r = b = 0; r < numreplicas; r++) {
		shard = ezalloc(numshards * sizeof(guid_t));
		replica[r] = shard;
		if (b == 0) b = r;
		for (s = 0; s < numshards; s++) {
			uuid_copy(shard[s], bag[b++]);
			if (b == numbags) b = 0;
		}
	}
	return replica;
}

void pr_guids (unint n, guid_t guid[n])
{
	char	out[100];
	unint	i;

	for (i = 0; i < n; i++) {
		uuid_unparse(guid[i], out);
		printf("%4lu. %s\n", i, out);
	}
}

void pr_replicas (unint numreplicas, guid_t *replica[numreplicas], unint numshards)
{
	char	out[100];
	guid_t	*shard;
	unint	i, j;

	for (i = 0; i < numshards; i++) {
		printf("%4lu.", i);
		for (j = 0; j < numreplicas; j++) {
			shard = replica[j];
			uuid_unparse(shard[i], out);
			printf(" %s", out);
		}
		printf("\n");
	}
}

guid_t *gen_bags (unint numbags)
{
	guid_t	*bags;
	unint	i;

	bags = ezalloc(numbags * sizeof(guid_t));

	for (i = 0; i < numbags; i++) {
		uuid_generate(bags[i]);
	}
	return bags;
}

unint	num_shards (unint nbags, unint nreps)
{
	unint	n = nbags * nreps * 10;
	unint	i;

	for (i = 0; (1<<i) < n; i++)
		;
	return 1<<i;
}

int main (int argc, char *argv[])
{
	unint	numbags = 3;
	unint	numreplicas = 2;
	unint	numshards;
	guid_t	*bag;
	guid_t	**replicas;

	setprogname(argv[0]);
	if (argc > 1) {
		numbags = atol(argv[1]);
	}
	if (argc > 2) {
		numreplicas = atol(argv[2]);
	}
	numshards = num_shards(numbags, numreplicas);

	bag = gen_bags(numbags);

	pr_guids(numbags, bag); printf("\n");

	replicas = pack(numbags, bag, numshards, numreplicas);

	pr_replicas(numreplicas, replicas, numshards);
	return 0;
}
