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

#include <style.h>

/*
 * Primes from planetmath.org These primes are good for hash tables because
 * they lay about half way between powers of 2.
 * planetmath.org's table started at 53, I added 3, 7, 13, and 23.
 * Added the values from 3221225479 on.
 */
u64 Primes[] = {
	          3,
	          7,
	         13,
	         23,

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

	 3221225479ULL,
	 6442450939ULL,
	12884901877ULL,
	25769803751ULL,
	51539607551ULL,
       103079215087ULL,
       206158430209ULL,
       412316860387ULL,
       824633720837ULL,

		0};

u64 findprime (u64 x)
{
	int	i;

	for (i = 0;; i++) {
		if (!Primes[i]) return 0;
		if (x <= Primes[i]) return Primes[i];
	}
}
