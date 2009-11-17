/*
 * MurmurHash - http://tanjent.livejournal.com/756623.html
 *            - http://murmurhash.googlepages.com
 * By: Austin Appleby
 * License: public domain
 *
 * Faster than crc32 but not as by much as claimed by the author.
 * I have not studied this for collisions in file names. Paul Taysom
 */
unsigned int murmurhash (const unsigned char * data, int len, unsigned int h)
{
	const unsigned int m = 0x7fd652ad;
	const int r = 16;

	h += 0xdeadbeef;

	while(len >= 4)
	{
		h += *(unsigned int *)data;
		h *= m;
		h ^= h >> r;

		data += 4;
		len -= 4;
	}

	switch(len)
	{
	case 3:
		h += data[2] << 16;
	case 2:
		h += data[1] << 8;
	case 1:
		h += data[0];
		h *= m;
		h ^= h >> r;
	};

	h *= m;
	h ^= h >> 10;
	h *= m;
	h ^= h >> 17;

	return h;
}
