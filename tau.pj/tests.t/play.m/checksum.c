#include <stdio.h>
#include <stdlib.h>

#include <style.h>
#include <eprintf.h>
#include <timer.h>
#include <mystdlib.h>
#include <crc.h>

/*
 * 1. XOR is as fast or faster than SUM.
 * 2. From Wikipedia, XOR is slightly better then SUM when detecting
 *	2 bit errors.
 * 3. Unrolling loop doesn't seem to make a difference
 * 4. CRC is ~13 times slower on my x86_64 machine
 * 5. CRC was only ~2 times slower on my G4
 * 6. crc64 is 25% slower than crc32 on a 64 bit x86
 */

unint checksum (void *vector, unint size)
{
	unint	*v = vector;
	unint	*end = &v[size/sizeof(unint)];
	unint	sum = 0;

	while (v < end) {
		sum ^= *v++;
	}
	return sum;
}

void fill (void *vector, unint size)
{
	unint	*v = vector;
	unint	*end = &v[size/sizeof(unint)];
	//unint	i = 0;

	while (v < end) {
		// *v++ = i++;
		*v++ = random();
	}
}

void corrupt (void *vector, unint size)
{
	u8	*v = vector;
	unint	i;

	i = urand(size);
	v[i] ^= 1;
}

enum { SIZE = 1 << 24 };

unint	Vect[SIZE];

int main (int argc, char *argv[])
{
	unint	sum;
	u64	start;
	u64	finish;

	setprogname(argv[0]);

	fill(Vect, sizeof(Vect));

	start = nsecs();
	sum = checksum(Vect, sizeof(Vect));
	//sum = crc64(Vect, sizeof(Vect));
	finish = nsecs();

	printf("%g", ((double)finish - (double)start) / 1000000000);
	printf(" %lx", sum);

	corrupt(Vect, sizeof(Vect));

	sum = checksum(Vect, sizeof(Vect));
	//sum = crc64(Vect, sizeof(Vect));
	printf(" %lx\n", sum);

	return 0;
}
