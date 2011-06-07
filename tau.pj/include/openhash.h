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
 * Implents an open address hashing based using double hashing.
 * Before including header file, must define NUM_BUCKETS and openhash_t
 */
#ifndef _OPENHASH_H_
#define _OPENHASH_H_ 1

#define _STEP_MASK	((NUM_BUCKETS < 0xf) ? 0x1 : ((NUM_BUCKETS < 0xff) ? 0xf : 0xff))

#define OPEN_HASH(_x)	((_x) % NUM_BUCKETS)
#define STEP_HASH(_x)	(((_x) & _STEP_MASK) + 1)

static inline void openhash (openhash_t *bucket, openhash_t x)
{
	unsigned long	h = OPEN_HASH(x);
	unsigned long	u = STEP_HASH(x);

	while (bucket[h]) {
		h += u;
		if (h >= NUM_BUCKETS) h -= NUM_BUCKETS;
	}
	bucket[h] = x;
}

static inline unsigned long openfindhash (openhash_t *bucket, openhash_t x)
{
	unsigned long	h = OPEN_HASH(x);
	unsigned long	u = STEP_HASH(x);

	while (bucket[h]) {
		if (bucket[h] == x) return h;
		h += u;
		if (h >= NUM_BUCKETS) h -= NUM_BUCKETS;
	}
	return NUM_BUCKETS;
}

static inline unsigned long openchainlength (openhash_t *bucket, openhash_t x)
{
	unsigned long	h = OPEN_HASH(x);
	unsigned long	u = STEP_HASH(x);
	unsigned long	i;

	for (i = 1; bucket[h] != x; i++) {
		if (!bucket[h]) return NUM_BUCKETS;
		h += u;
		if (h >= NUM_BUCKETS) h -= NUM_BUCKETS;
	}
	return i;
}

#endif
