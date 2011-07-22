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

#ifndef _HASH_H_
#define _HASH_H_

#include "crc.h"
#include "style.h"

typedef struct HashRecord_s {
	struct HashRecord_s	*next;
	u32			crc;
} HashRecord_s;

typedef struct HashTable_s {
	unint		mask;	/* num buckets - 1 (num buckets is power of two) */
	unint		numRecords;
	bool		(*isMatch)(void *key, HashRecord_s *rec);
	u32		(*hash)(void *key);
	void		*(*alloc)(void *key, void *data);
	void		(*destroy)(void *rec);
	HashRecord_s	**table;
} HashTable_s;

typedef struct HashStats_s
{
	unint	totalRecords;
	unint	numBuckets;
	unint	maxChain;
	unint	minChain;
#ifdef UNIX
	float	avgChain;
#endif
} HashStats_s;

	/*
	 * Hash function prototypes
	 */
extern HashTable_s *hashInit(
	HashTable_s	*ht,
	unint		numBuckets,
	bool		(*isMatch)(void *key, HashRecord_s *rec),
	u32		(*hash)(void *key),
	void		*(*alloc)(void *key, void *data),
	void		(*destroy)(void *rec));

extern int hashApply(
	HashTable_s	*ht,
	int		(*myFunc)(HashRecord_s *rec, void *metaData),
	void		*metaData);

extern HashStats_s *hashStats(
	HashTable_s	*ht,
	HashStats_s	*stats);

extern void hashDelete(HashTable_s *ht, void *key);
extern void *hashFind(HashTable_s *ht, void *key);
extern void *hashInsert(HashTable_s *ht, void *key, void *data);
extern HashTable_s *hashGrow(HashTable_s *ht, unint factor);
extern HashTable_s *hashShrink (HashTable_s *ht, unint factor);
extern void hashDestroy(HashTable_s *ht);

#endif
