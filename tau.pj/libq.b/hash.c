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
 * This module is used to: Generic Hashing.
 *
 * Ideally, this routine should be in a library that is then loaded with
 * another library that as the malloc/free/realloc code in it.  So it
 * can at load time pick-up the right routines.  This requires creating
 * yet another library which I'm not ready to do yet.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bit.h>
#include "hash.h"

/*
 * The generic hash code provides a basic set of functions for
 * managing hash tables.
 *
 * The code assumes each record has HashRecord_s at the beginning of
 * each record stored in the hash table.  By careful use of the macros
 * offsetof (STRUCT), the HashRecord_s structure can be placed anywhere
 * in the users data structures.  The user supplies the HashTable_s
 * structure, but the system allocates space for the actual table.
 *
 * hashInit - initializes the hash table
 *	numBuckets - number of buckets to use (we take the first power of
 *		2 >= numBuckets).
 *	isMatch - returns True if the key for the record mathes the key
 *		passed in.
 *	hash - generates a hash of the key (see crc.c and crc.h).
 *		Even though the term CRC is used in the code,
 *		it does not have to be a CRC.  CRC just make
 *		a very nice hash value.
 *	alloc - allocates the record, if it already exits, it can
 *		do nothing.
 *	destory - used to free the allocated record.
 */
HashTable_s *hashInit (
	HashTable_s	*ht,
	unint		numBuckets,
	bool		(*isMatch)(void *key, HashRecord_s *rec),
	u32		(*hash)(void *key),
	void		*(*alloc)(void *key, void *data),
	void		(*destroy)(void *rec))
{
	HashRecord_s	**table;
	unint		highBit;
	unint		size;

	if ((isMatch == NULL)
		|| (hash == NULL)
		|| (alloc == NULL)
		|| (destroy == NULL))
	{
		return NULL;
	}
	highBit = flsl(numBuckets);
	if (highBit > 28)	// You've got to be kidding
	{
		return NULL;
	}
	numBuckets = (1<<highBit);

	size = sizeof(HashRecord_s *) * numBuckets;
	table = malloc(size);
	if (table == NULL)
	{
		return NULL;
	}
	bzero(table, size);

	ht->mask       = numBuckets - 1;
	ht->numRecords = 0;
	ht->isMatch    = isMatch;
	ht->hash       = hash;
	ht->alloc      = alloc;
	ht->destroy    = destroy;
	ht->table      = table;

	return ht;
}

/*
 * hashLookup looks for a record that matches the crc first
 * and then the key.
 */
HashRecord_s *hashLookup (
	HashTable_s	*ht,
	u32		crc,
	void		*key)
{
	unint			bucket;
	HashRecord_s	*next;
	HashRecord_s	*prev;

	bucket = crc & ht->mask;
	next = ht->table[bucket];

	if (next == NULL)
	{
		return NULL;
	}
	if (crc == next->crc)
	{
		if (ht->isMatch(key, next))
		{
			return next;
		}
	}
	for (;;)
	{
		prev = next;
		next = prev->next;

		if (next == NULL)
		{
			return NULL;
		}
		if (crc == next->crc)
		{
			if (ht->isMatch(key, next))
			{
				prev->next = next->next;
				next->next = ht->table[bucket];
				ht->table[bucket] = next;
				return next;
			}
		}
	}
}

/*
 * hashFind computes the crc and then calls hashLookup
 * which returns the record if it is found.
 */
void *hashFind (HashTable_s *ht, void *key)
{
	return hashLookup(ht, ht->hash(key), key);
}

/*
 * hashDelete deletes the record matching the key
 */
void hashDelete (HashTable_s *ht, void *key)
{
	HashRecord_s	*rec;

	rec = hashLookup(ht, ht->hash(key), key);
	if (rec != NULL)
	{
		ht->table[rec->crc & ht->mask] = rec->next;
		ht->destroy(rec);
		--ht->numRecords;
	}
}

/*
 * hashInsert tries to insert a new record.  If an record
 * already exits that matches the key, that record is
 * returned.
 */
void *hashInsert (HashTable_s *ht, void *key, void *data)
{
	u32		crc;
	unint		bucket;
	HashRecord_s	*rec;

	crc = ht->hash(key);
	rec = hashLookup(ht, crc, key);
	if (rec != NULL)
	{
		return rec;
	}
	rec = ht->alloc(key, data);
	if (rec == NULL)
	{
		return NULL;
	}
	bucket = crc & ht->mask;
	rec->next = ht->table[bucket];
	ht->table[bucket] = rec;
	rec->crc = crc;

	++ht->numRecords;

	return rec;
}

/*
 * hashApply applies the given function, myFunc, to each record
 * in the table and passes along the metaData supplied
 */
int hashApply (
	HashTable_s	*ht,
	int		(*myFunc)(HashRecord_s *rec, void *metaData),
	void		*metaData)
{
	int		rc;
	unsigned	i;
	unsigned	numBuckets = ht->mask + 1;
	HashRecord_s	**table = ht->table;
	HashRecord_s	*rec;

	for (i = 0; i < numBuckets; ++i)
	{
		rec = table[i];
		while (rec != NULL)
		{
			rc = myFunc(rec, metaData);
			if (rc != 0)
			{
				return rc;
			}
			rec = rec->next;
		}
	}
	return 0;
}

/*
 * Increases the number of hash buckets - the first power of
 * 2 less than or equal to the factor is used.
 */
HashTable_s *hashGrow (HashTable_s *ht, unint factor)
{
	unint		i;
	unint		highBit;
	unint		numBuckets = ht->mask + 1;
	HashRecord_s	**table = ht->table;
	HashRecord_s	*rec;
	HashRecord_s	*next;
	unint		newMask;
	unint		newNumBuckets;
	unint		bucket;

	highBit = flsl(factor);
	factor = (1 << highBit);

	newNumBuckets = numBuckets * factor;
	newMask = newNumBuckets - 1;
	table = realloc(table, newNumBuckets * sizeof(HashRecord_s *));
	if (table == NULL)
	{
		return NULL;
	}
	for (i = numBuckets; i < newNumBuckets; ++i)
	{
		table[i] = NULL;
	}
	for (i = 0; i < numBuckets; ++i)
	{
		rec = table[i];
		table[i] = NULL;
		while (rec != NULL)
		{
			bucket = (rec->crc & newMask);
			next = rec->next;
			rec->next = table[bucket];
			table[bucket] = rec;
			rec = next;
		}
	}
	ht->mask = newMask;
	ht->table = table;
	return ht;
}

/*
 * Decreases the number of hash buckets - the first power of
 * 2 less than or equal to the factor is used.
 */
HashTable_s *hashShrink (HashTable_s *ht, unint factor)
{
	unint		i;
	unint		highBit;
	unint		numBuckets = ht->mask + 1;
	HashRecord_s	**table = ht->table;
	HashRecord_s	*rec;
	HashRecord_s	*next;
	unint		newMask;
	unint		newNumBuckets;
	unint		bucket;

	highBit = flsl(factor);
	factor = (1 << highBit);

	newNumBuckets = numBuckets / factor;
	if (newNumBuckets == 0)
	{
		return NULL;
	}
	newMask = newNumBuckets - 1;
	ht->mask = newMask;

	for (i = newNumBuckets; i < numBuckets; ++i)
	{
		rec = table[i];
		table[i] = NULL;
		while (rec != NULL)
		{
			bucket = (rec->crc & newMask);
			next = rec->next;
			rec->next = table[bucket];
			table[bucket] = rec;
			rec = next;
		}
	}
	table = realloc(table, newNumBuckets * sizeof(HashRecord_s *));
	if (table == NULL)
	{
		return ht;
	}
	ht->table = table;
	return ht;
}

/*
 * Destory the hash table, all the users destroy function
 * for each record and then freeing the table.
 */
void hashDestroy (HashTable_s *ht)
{
	unint			i;
	unint			numBuckets = ht->mask + 1;
	HashRecord_s	**table = ht->table;
	HashRecord_s	*rec;
	HashRecord_s	*next;

	for (i = 0; i < numBuckets; ++i)
	{
		rec = table[i];
		while (rec != NULL)
		{
			next = rec->next;
			ht->destroy(rec);
			rec = next;
		}
	}
	free(table);
}

/*
 * Generate statistics for the hash table
 */
HashStats_s *hashStats (
	HashTable_s	*ht,
	HashStats_s	*stats)
{
	unint		maxChain = 0;
	unint		minChain = ~0;
	unint		total = 0;
	unint		sum;
	unint		i;
	unint		numBuckets = ht->mask + 1;
	HashRecord_s	**table = ht->table;
	HashRecord_s	*rec;

	for (i = 0; i < numBuckets; ++i)
	{
		sum = 0;
		rec = table[i];
		while (rec != NULL)
		{
			++sum;
			rec = rec->next;
		}
		total += sum;
		if (sum > maxChain)
		{
			maxChain = sum;
		}
		if (sum < minChain)
		{
			minChain = sum;
		}
	}
	stats->numBuckets   = numBuckets;
	stats->totalRecords = total;
	stats->maxChain = maxChain;
	stats->minChain = minChain;
#ifdef UNIX
	stats->avgChain = ((float)total) / numBuckets;
#endif
	return stats;
}

/*
 * Dumps the hash table (just the CRC values) to the screen
 */
void hashDump (HashTable_s *ht)
{
	unint		i;
	unint		numBuckets = ht->mask + 1;
	HashRecord_s	**table = ht->table;
	HashRecord_s	*rec;

	for (i = 0; i < numBuckets; ++i)
	{
		rec = table[i];
		while (rec != NULL)
		{
			//printf("%x ", rec->crc);
			rec = rec->next;
		}
		printf("\n");
	}
}
