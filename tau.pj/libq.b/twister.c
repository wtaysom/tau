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

/*
 * Changes made in this instance of the code:
 * 1. Replaced global variables with global structure Twister
 * 2. Names of functions changed.
 * 3. Task friendly interfaces added
 * 4. Kernel indentation rules
 * 5. unsigned long long -> u64
 * 6. Spacing around operators
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include <eprintf.h>
#include <twister.h>

#define NN SIZE_TWISTER
#define MM (NN / 2)
#define MATRIX_A 0xB5026F5AA96619E9ULL
#define UM 0xFFFFFFFF80000000ULL /* Most significant 33 bits */
#define LM 0x7FFFFFFFULL /* Least significant 31 bits */

static Twister_s Twister = { NN+1, { 0 }};

static void fill_random_seed (void *seed, int numbytes)
{
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd == -1) fatal("open /dev/urandom:");
	ssize_t rc = read(fd, seed, numbytes);
	if (rc != numbytes) fatal("read /dev/urandom:");
	close(fd);
}

/* initializes twister with a seed */
void init_twister_r (u64 seed, Twister_s *twister)
{
	u64 *mt = twister->mt;
	int mti;

	mt[0] = seed;
	for (mti = 1; mti < NN; mti++) {
		mt[mti] = (6364136223846793005ULL *
			(mt[mti-1] ^ (mt[mti-1] >> 62)) + mti);
	}
	twister->mti = mti;
}

void init_twister (u64 seed)
{
	init_twister_r(seed, &Twister);
}

/* initialize by an array with array-length
 * init_key is the array for initializing keys
 * key_length is its length
 */
void init_twister_by_array_r (u64 init_key[], u64 key_length,
				Twister_s *twister)
{
	u64 i, j, k;
	u64 *mt = twister->mt;

	init_twister_r(19650218ULL, twister);
	i = 1;
	j = 0;
	k = (NN > key_length ? NN : key_length);
	for (; k; k--) {
		mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 62)) *
			3935559000370003845ULL)) + init_key[j] + j; /* non linear */
		i++; j++;
		if (i >= NN) {
			mt[0] = mt[NN - 1];
			i = 1;
		}
		if (j >= key_length) j=0;
	}
	for (k = NN - 1; k; k--) {
		mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 62)) *
			2862933555777941757ULL)) - i; /* non linear */
		i++;
		if (i >= NN) {
			mt[0] = mt[NN - 1];
			i = 1;
		}
	}
	mt[0] = 1ULL << 63; /* MSB is 1; assuring non-zero initial array */
}

void init_twister_by_array (u64 init_key[], u64 key_length)
{
	init_twister_by_array_r(init_key, key_length, &Twister);
}

/* generates a random number on [0, 2^64-1]-interval */
u64 twister_random_r (Twister_s *twister)
{
	int i;
	u64 x;
	u64 *mt = twister->mt;
	static u64 mag01[2] = {0ULL, MATRIX_A};

	if (twister->mti >= NN) { /* generate NN words at one time */

		/* if init_genrand64() has not been called, */
		/* a default initial seed is used	 */
		if (twister->mti == NN + 1) {
			init_twister_r(5489ULL, twister);
		}
		for (i = 0; i < NN - MM; i++) {
			x = (mt[i] & UM) | (mt[i + 1] & LM);
			mt[i] = mt[i + MM] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];
		}
		for (;i < NN - 1; i++) {
			x = (mt[i] & UM)|(mt[i + 1]&LM);
			mt[i] = mt[i + (MM - NN)] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];
		}
		x = (mt[NN - 1] & UM) | (mt[0] & LM);
		mt[NN - 1] = mt[MM - 1] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];

		twister->mti = 0;
	}

	x = mt[twister->mti++];

	x ^= (x >> 29) & 0x5555555555555555ULL;
	x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
	x ^= (x << 37) & 0xFFF7EEE000000000ULL;
	x ^= (x >> 43);

	return x;
}

u64 twister_random (void)
{
	return twister_random_r(&Twister);
}

void twister_random_seed_r (Twister_s *twister)
{
	u64 seed[SIZE_TWISTER];

	fill_random_seed(seed, sizeof(seed));
	init_twister_by_array_r(seed, SIZE_TWISTER, twister);
}

void twister_random_seed (void)
{
	twister_random_seed_r(&Twister);
}

/* generates a random number on [0, 2^63-1]-interval */
s64 twister_srandom_r (Twister_s *twister)
{
	return (s64)(twister_random_r(twister) >> 1);
}

s64 twister_srandom (void)
{
	return twister_srandom_r(&Twister);
}

/* generates a random number on [0,1]-real-interval */
double twister_real1_r (Twister_s *twister)
{
	return (twister_random_r(twister) >> 11) * (1.0/9007199254740991.0);
}

double twister_real1 (void)
{
	return twister_real1_r(&Twister);
}

/* generates a random number on [0,1)-real-interval */

double twister_real2_r (Twister_s *twister)
{
	return (twister_random_r(twister) >> 11) * (1.0/9007199254740992.0);
}

double twister_real2 (void)
{
	return twister_real2_r(&Twister);
}

/* generates a random number on (0,1)-real-interval */

double twister_real3_r (Twister_s *twister)
{
	return ((twister_random_r(twister) >> 12) + 0.5) *
		(1.0/4503599627370496.0);
}

double twister_real3 (void)
{
	return twister_real3_r(&Twister);
}

enum { BIG_PRIME = 824633720837ULL };

static u64 Task_seed = BIG_PRIME;
static pthread_mutex_t Task_seed_lock = PTHREAD_MUTEX_INITIALIZER;

void twister_reset_task_seed_r (void)
{
	pthread_mutex_lock(&Task_seed_lock);
	Task_seed = BIG_PRIME;
	pthread_mutex_unlock(&Task_seed_lock);
}

void twister_task_seed_r (Twister_s *twister)
{
	pthread_mutex_lock(&Task_seed_lock);
	Task_seed *= BIG_PRIME;
	pthread_mutex_unlock(&Task_seed_lock);

	init_twister_r(Task_seed, twister);
}

/* generates a random, null terminated name using [a-z][A-F] */
char *twister_name_r (char *name, size_t length, Twister_s *twister)
{
	enum { CHAR_SHIFT = 5,
		CHAR_SIZE = (1 << CHAR_SHIFT),
		CHAR_MASK = CHAR_SIZE - 1 };
	static const char Char_set[] = "abcdefghijklmnopqrstuvwxyzABCDEF";
	CHECK_CONST(CHAR_SIZE < sizeof(Char_set));
	char *c = name;
	ssize_t i = length - 1;
	u64 x;

	x = twister_random_r(twister);
	while (i-- > 0) {
		*c++ = Char_set[x & CHAR_MASK];
		x >>= CHAR_SHIFT;
		if (!x) x = twister_random_r(twister);
	}
	*c = '\0';
	return name;
}

char *twister_name (char *name, size_t length)
{
	return twister_name_r(name, length, &Twister);
}
