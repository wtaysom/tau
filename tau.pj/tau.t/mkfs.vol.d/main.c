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

#include <sys/errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <uuid/uuid.h>

#include <style.h>
#include <q.h>
#include <tau/msg.h>
#include <tau/msg_util.h>
#include <tau/switchboard.h>
#include <tau/fsmsg.h>
#include <tau/tag.h>
#include <tau/disk.h>
#include <debug.h>
#include <eprintf.h>
#include <mystdlib.h>
#include <set.h>

#include "bagio.h"

enum {	DEFAULT_SHARDS_PER_BAG = 4,
	DEFAULT_NUM_REPLICAS = 3 };

typedef struct vol_s {
	char		vl_name[TAU_NAME];
	guid_t		vl_guid;
} vol_s;

typedef struct bag_s {
	dqlink_t	bg_link;
	char		bg_name[TAU_NAME];
	guid_t		bg_guid_bag;
	guid_t		bg_guid_vol;
	u64		bg_num_blocks;
	ki_t		bg_key;
} bag_s;

static d_q	Bag_list = static_init_dq(Bag_list);
static bag_s	**Bag_array;
static vol_s	Volume;

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

static void simple (int nbags, int nreps, int nshards, int a[nreps][nshards])
{
	enum { prime = 50331653 };
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

static void rand_bin (int nbags, int nreps, int nshards, int a[nreps][nshards])
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

static int numshards (int nbags, int shards_per_bag, int nreplicas)
{
	int	n = nbags * nreplicas * shards_per_bag;
	int	nshards;
	int	i;

	for (i = 0; (1<<i) < n; i++) {
		if (i == 30) {
			fatal("too many shards=%d: nbags=%d"
				" shards_per_bag=%d nreplicas=%d\n",
				n, nbags, shards_per_bag, nreplicas);
		}
	}
	nshards = 1<<i;
	return nshards;
}

void make_bag_array (int nbags)
{
	bag_s	*bag = NULL;
	int	i;

	Bag_array = emalloc(nbags * sizeof(guid_t));

	for (i = 0; i < nbags; i++) {
		next_dq(bag, &Bag_list, bg_link);
		if (!bag) eprintf("ran out of bags");
		Bag_array[i] = bag;
	}
}

void test_bags (int nbags)
{
	tau_bag_s	*tbag;
	bag_s		*bag;
	int		i;

	for (i = 0; i < nbags; i++) {
		bag = Bag_array[i];
		tbag = bget(bag->bg_key, TAU_SUPER_BLK);
		if (!tbag) eprintf("couldn't read bag block");
		if (uuid_compare(bag->bg_guid_bag, tbag->tb_guid_bag) != 0) {
			eprintf("guid mismatch");
		}
		bput(tbag);
	}
}

void put_shard (bag_s *bag, int shard_no, int nshards, int replica_no, int nreps)
{
	fsmsg_s	m;
	int	rc;

	m.m_method = PUT_SHARD;
	m.sh_shard_no     = shard_no;
	m.sh_replica_no   = replica_no;
	m.sh_num_shards   = nshards;
	m.sh_num_replicas = nreps;

	rc = call_tau(bag->bg_key, &m);
	if (rc) fatal("key=%d shard=%d replica=%d",
			bag->bg_key, shard_no, replica_no);
}


void bag_write_shards (bag_s *bag, int nreps, int nshards, int a[nreps][nshards])
{
	bag_s	*b;
	int	shard;
	int	rep;

	for (rep = 0; rep < nreps; rep++) {
		for (shard = 0; shard < nshards; shard++) {
			b = Bag_array[a[rep][shard]];
			if (uuid_compare(bag->bg_guid_bag, b->bg_guid_bag) == 0) {
				put_shard(bag, shard, nshards, rep, nreps);
			}
		}
	}
}

void write_shards (int nreps, int nshards, int a[nreps][nshards], int nbags)
{
	int	i;

	for (i = 0; i < nbags; i++) {
		bag_write_shards(Bag_array[i], nreps, nshards, a);
	}
}

void assign_vol_id (guid_t vol_id, int nbags, int nshards, int nreps)
{
	fsmsg_s	m;
	int	i;
	int	rc;

	for (i = 0; i < nbags; i++) {
		m.m_method = FINISH_BAG;
		uuid_copy(m.vol_guid, vol_id);
		m.vol_state        = TAU_BAG_VOLUME;
		m.vol_num_bags     = nbags;
		m.vol_num_shards   = nshards;
		m.vol_num_replicas = nreps;
		rc = call_tau(Bag_array[i]->bg_key, &m);
		if (rc) fatal("call %d", i);
	}
}

void build_volume (guid_t vol_guid, int shards_per_bag, int nreplicas)
{
	int	nbags;
	int	nshards;
	int	total;
	void	*shards;

	nbags = cnt_dq( &Bag_list);
	if (nreplicas > nbags) fatal("reps %d > %d bags", nreplicas, nbags);
	nshards = numshards(nbags, shards_per_bag, nreplicas);
	total = nreplicas * nshards;

	shards = ezalloc(total * sizeof(int));

	rand_bin(nbags, nreplicas, nshards, shards);

	prshards(nreplicas, nshards, shards);

	make_bag_array(nbags);

	test_bags(nbags);

	write_shards(nreplicas, nshards, shards, nbags);

	assign_vol_id(vol_guid, nbags, nshards, nreplicas);
}

static void new_bag (ki_t key, guid_t vol_guid)
{
	fsmsg_s	m;
	bag_s	*bag;
	int	rc;

	if (!key) {
		fatal("no key");
		return;
	}
	m.m_method = STAT_BAG;
	rc = call_tau(key, &m);
	if (rc) {
		fatal("call %d", rc);
		destroy_key_tau(key);
		return;
	}
	if (uuid_compare(vol_guid, m.bag_guid_vol) != 0) {
		destroy_key_tau(key);
		return;
	}
	if (m.bag_state != TAU_BAG_NEW) {
		fatal("bag in wrong state %lld", m.bag_state);
		return;
	}

	bag = ezalloc(sizeof(*bag));

	uuid_copy(bag->bg_guid_vol, m.bag_guid_vol);
	uuid_copy(bag->bg_guid_bag, m.bag_guid_bag);
	bag->bg_key = key;

	enq_dq( &Bag_list, bag, bg_link);
}

void get_bags (guid_t vol_guid)
{
	u64	sequence = 0;
	ki_t	key;
	int	rc;

	for (;;) {
		rc = sw_next("bag", &key, sequence, &sequence);
		if (rc == DESTROYED) break;
		if (rc) eprintf("get_bags failed %d", rc);
		new_bag(key, vol_guid);
	}
}

void switchboard (void)
{
	int	rc;

	rc = sw_register(getprogname());
	if (rc) eprintf("sw_register %d", rc);
}

void prompt (void)
{
	printf("Press a key to continue: ");
	getchar();
}

static void usage (void)
{
	printf("usage: %s [-d?] -r <num replicas> -u <guid>\n"
		"\td - debug\n"
		"\tr - number of replicas\n"
		"\ts - number of shards per bag\n"
		"\tu - volume guid (bags should be created with this guid)\n"
		"\t? - usage\n",
		getprogname());
	exit(2);
}

int main (int argc, char *argv[])
{
	int	c;
	int	rc;
	int	nreps          = DEFAULT_NUM_REPLICAS;
	int	shards_per_bag = DEFAULT_SHARDS_PER_BAG;
	guid_t	vol_guid;
	bool	have_vol_guid = FALSE;

	setprogname(argv[0]);
	debugon();

	while ((c = getopt(argc, argv, "dr:s:u:?")) != -1) {
		switch (c) {
		case 'd':
			debugon();
			fdebugon();
			break;
		case 'r':
			nreps = atoi(optarg);
			if (nreps > TAU_MAX_REPLICAS) {
				fatal("too many replicas %d", nreps);
			}
			if (nreps < 1) {
				fatal("too few replicas %d", nreps);
			}
			break;
		case 's':
			shards_per_bag = atoi(optarg);
			break;
		case 'u':
			rc = uuid_parse(optarg, vol_guid);
			if (rc) {
				printf("couldn't parse %s\n", optarg);
				exit(2);
			}
			have_vol_guid = TRUE;
			break;
		case '?':
		default:
			usage();
			break;
		}
	}
	if (!have_vol_guid) {
		printf("must supply guid for volume\n");
		usage();
	}
	binit(7);
	init_msg_tau(getprogname());
	switchboard();
	uuid_copy(Volume.vl_guid, vol_guid);
	get_bags(vol_guid);
	build_volume(vol_guid, shards_per_bag, nreps);
	return 0;
}
