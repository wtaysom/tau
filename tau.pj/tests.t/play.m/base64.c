/****************************************************************************
 |
 | Copyright (c) 2009 Novell, Inc.
 |
 | Permission is hereby granted, free of charge, to any person obtaining a
 | copy of this software and associated documentation files (the "Software"),
 | to deal in the Software without restriction, including without limitation
 | the rights to use, copy, modify, merge, publish, distribute, sublicense,
 | and/or sell copies of the Software, and to permit persons to whom the
 | Software is furnished to do so, subject to the following conditions:
 |
 | The above copyright notice and this permission notice shall be included
 | in all copies or substantial portions of the Software.
 |
 | THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 | OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 | MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
 | NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 | DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 | OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 | USE OR OTHER DEALINGS IN THE SOFTWARE.
 +-------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include <style.h>
#include <eprintf.h>
#include <mylib.h>
#include <debug.h>

void fill_buf (void *buf, unint length, int seed)
{
	u8	*b = buf;
	unint	i;

	for (i = 0; i < length; i++) {
		*b++ = seed + i;//i*(147+seed);//(i & 1) ? 43 : 71;//random();
	}
}

void background (void *buf, unint length, int seed)
{
	u8	*b = buf;
	unint	i;

	for (i = 0; i < length; i++) {
		*b++ = seed;// + i;
	}
}

void check_background (void *buf, unint length, int seed, unint start, unint end)
{
	u8	*b = buf;
	unint	i;

	seed &= 0xff;
	for (i = 0; i < length; i++, b++) {
		if ((i < start) || (i >= end)) {
			if (*b != seed/* + i*/) {
				prbytes("background", buf, length);
				warn("%ld. start=%ld end=%ld",
					i, start, end);
				return;
			}
		}
	}
}


void compare_bufs (u8 *a_buf, u8 *b_buf, unint length)
{
	u8	*a = a_buf;
	u8	*b = b_buf;
	unint	i;

	for (i = 0; i < length; i++) {
		if (a_buf[i] != b_buf[i]) {//if (*a != *b) {
			warn("%ld. %ld %p %p %x != %x", i, length, a, b, *a, *b);
			prbytes("a", a_buf, length);
			prbytes("b", b_buf, length);
			return;
		}
		++a;
		++b;
	}
}

void usage (void)
{
	eprintf("Usage: %s [-u <uuid>]\n", getprogname());
}

int main (int argc, char *argv[])
{
#if 0
	int	c;
	uuid_t	uu;
	bool	uuid = FALSE;
	int	rc;

	setprogname(argv[0]);
	for (;;) {
		c = getopt(argc, argv, "u:");
		if (c == -1) break;
		switch (c) {
		case 'u':
			rc = uuid_parse(optarg, uu);
			if (rc) {
				printf("couldn't parse %s\n", optarg);
				exit(2);
			}
			uuid = TRUE;
			break;
		default:
			usage();
			break;
		}
	}
	if (!uuid) {
		uuid_generate(uu);
	}
	uuid_unparse(uu, out);
	printf("uuid=%s\n", out);

	u64	x;
	u64	y;
	for (i = 0; i < n; i++) {
		x = random();
		to_base64( &x, sizeof(x), out);
		length = from_base64(out, &y);
		printf("%llx %llx %s %d\n", x, y, out, length);
	}
#endif
	u64	i;
	char	out[10000];
	u64	n = 200000;//000;
	int	length;
	int	size;
	int	start;
	u8	a[127];
	u8	b[2 * sizeof(a)];

	for (i = 0; i < n; i++) {
		start = urand(sizeof(a));
		size = urand(sizeof(a)) + 1;
		fill_buf(a, size, 'A' + i);
		background(b, sizeof(b), 'P' + i);
		to_base64(a, size, out);
//printf("%d out=%s\n", size, out);
		length = from_base64(out, &b[start]);
		//prbytes("a", a, size);
		//prbytes("b", b, size);
		if (length != size) warn("bad length = %d", length);
		compare_bufs(a, &b[start], length);
		check_background(b, sizeof(b), 'P'+i, start, start+size);
		//prbytes("b", b, sizeof(b));
	}
	return 0;
}
