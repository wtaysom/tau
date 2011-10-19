/* 
   A C-program for MT19937-64 (2004/9/29 version).
   Coded by Takuji Nishimura and Makoto Matsumoto.

   This is a 64-bit version of Mersenne Twister pseudorandom number
   generator.

   Before using, initialize the state by using init_genrand64(seed)  
   or init_by_array64(init_key, key_length).

   Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   References:
   T. Nishimura, ``Tables of 64-bit Mersenne Twisters''
     ACM Transactions on Modeling and 
     Computer Simulation 10. (2000) 348--357.
   M. Matsumoto and T. Nishimura,
     ``Mersenne Twister: a 623-dimensionally equidistributed
       uniform pseudorandom number generator''
     ACM Transactions on Modeling and 
     Computer Simulation 8. (Jan. 1998) 3--30.

   Any feedback is very welcome.
   http://www.math.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove spaces)
*/

#ifndef _TWISTER_H_
#define _TWISTER_H_ 1

#include <style.h>

/* initializes mt[NN] with a seed */
void init_genrand64(unsigned long long seed);

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
void init_by_array64(unsigned long long init_key[],
			unsigned long long key_length);

/*
 * Interfaces to Mersenne Twister pseudorandom number generator.
 * The global code is not thread safe. For threaded code, use the
 * *_r interfaces. Each thread should generates its own twister seed.
 *
 * Uses global, shared tables:
 *
 * twister_random - Returns a 64 bit pseudorandom number
 * twister_urand - Returns pseudorandom number in range [0, upper)
 * twister_seed - Pass in a Twister_s structure where the mt array
 *	has been filled with your chosen seed volues.
 * twister_random_seed - Seed twister with /dev/urandom.
 *
 *
 * The *_r versions let eash task has its own independent random
 * number generator.
 *
 * twister_random_r - Returns a 64 bit pseudorandom number
 * twister_urand - Returns pseudorandom number in range [0, upper)
 * twister_seed_r - Generates a seed from the supplied twister structure
 *	The field mt needs to set with 312 64 bit values.
 * twister_task_seed_r - generate deterministic seed per task
 * twister_random_seed_r - returns a structure based on /dev/urandom
 */

enum { SIZE_TWISTER = 312 };	/* Must match NN in twister.c */	

typedef struct Twister_s {
	snint	mti;	/* Next entry to use from seed table */
	u64	mt[SIZE_TWISTER];
} Twister_s;

u64 twister_random(void);
void twister_seed(Twister_s *initial);
void twister_random_seed(void);
u64 twister_random_2(void);

u64 twister_random_r(Twister_s *twister);
Twister_s twister_seed_r(Twister_s *initial);
Twister_s twister_task_seed_r(void);
Twister_s twister_random_seed_r(void);

/* generates a random number on [0, 2^63-1]-interval */
s64 twister_int63(void);
s64 twister_int63_r(Twister_s *twister);

/* generates a random number on [0,1]-real-interval */
double twister_real1(void);
double twister_real1_r(Twister_s *twister);

/* generates a random number on [0,1)-real-interval */
double twister_real2(void);
double twister_real2_r(Twister_s *twister);

/* generates a random number on (0,1)-real-interval */
double twister_real3(void);
double twister_real3_r(Twister_s *twister);

/* generates a random, null terminated name using [a-z][A_F] */
char *twister_name(char *name, size_t length);
char *twister_name_r(char *name, size_t length, Twister_s *twister);


static inline u64 twister_urand_r (u64 upper, Twister_s *twister)
{
	return twister_random_r(twister) % upper;
}

static inline u64 twister_urand (u64 upper)
{
	return twister_random() % upper;
}


#endif
