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
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
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

/*
 * Interfaces to Mersenne Twister pseudorandom number generator.
 * The global code is not thread safe. For threaded code, use the
 * *_r interfaces. Each thread should generates its own twister seed.
 */

enum { SIZE_TWISTER = 312 };	/* Must match NN in twister.c */

typedef struct Twister_s {
	snint	mti;	/* Next entry to use from seed table */
	u64	mt[SIZE_TWISTER];
} Twister_s;

/* initializes twister with a seed */
void init_twister_r(u64 seed, Twister_s *twister);
void init_twister(u64 seed);

/* initialize by an array with array-length
 * init_key is the array for initializing keys
 * key_length is its length
 */
void init_twister_by_array_r(u64 init_key[], u64 key_length,
				Twister_s *twister);
void init_twister_by_array(u64 init_key[], u64 key_length);

u64 twister_random_r(Twister_s *twister);
u64 twister_random(void);

void twister_random_seed_r(Twister_s *twister);
void twister_random_seed(void);

/* generates a random number on [0, 2^63-1]-interval */
s64 twister_srandom_r(Twister_s *twister);
s64 twister_srandom(void);

/* generates a random number on [0,1]-real-interval */
double twister_real1_r(Twister_s *twister);
double twister_real1(void);

/* generates a random number on [0,1)-real-interval */
double twister_real2_r(Twister_s *twister);
double twister_real2(void);

/* generates a random number on (0,1);-real-interval */
double twister_real3_r(Twister_s *twister);
double twister_real3(void);

/* Random number generator for a set of tasks */
void twister_reset_task_seed_r(void);
void twister_task_seed_r(Twister_s *twister);

/* generates a random, null terminated name using [a-z][A-F] */
char *twister_name_r(char *name, size_t length, Twister_s *twister);
char *twister_name(char *name, size_t length);

/* generates a random number on [0,uppper)-integer-interval */
static inline u64 twister_urand_r (u64 upper, Twister_s *twister)
{
	return twister_random_r(twister) % upper;
}

static inline u64 twister_urand (u64 upper)
{
	return twister_random() % upper;
}


#endif
