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
#include <style.h>

void to_base64 (const void *data, int length, char *out)
{
	char	base[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			 "abcdefghijklmnopqrstuvwxyz"
			 "0123456789_:";
	const u8	*p = data;
	char		*c = out;
	unint		r = 0;
	unint		x;

	while (length) {
		x = *p & 0x3f;
		*c++ = base[x];
		r = 2;
		x = *p >> 6;
		if (--length == 0) break;

		++p;
		x |= (*p & 0xf) << 2;
		*c++ = base[x];
		r = 4;
		x = *p >> 4;
		if (--length == 0) break;

		++p;
		x |= (*p &0x3) << 4;
		*c++ = base[x];
		r = 6;
		x = *p >> 2;
		if (--length == 0) break;

		++p;
		*c++ = base[x];
		r = 0;
		x = 0;
	}
	if (r > 0) {
		*c++ = base[x];
	}
	*c = '\0';	
}

static int value (char c)
{
	if (('A' <= c) && (c <= 'Z')) return c - 'A';
	if (('a' <= c) && (c <= 'z')) return c - 'a' + 26;
	if (('0' <= c) && (c <= '9')) return c - '0' + 52;
	if (c == '_') return 62;
	if (c == ':') return 63;
	return -1;
}

int from_base64 (const char *in, void *data)
{
	const char	*c = in;
	u8		*p = data;
	int		length = 0;
	int		x;

	for (;;) {
		if (!*c) return length;

		x = value(*c++);
		if (x < 0) return -1;
		*p = x;
		if (!*c) return ++length;

		x = value(*c++);
		if (x < 0) return -1;
		*p |= ((x & 0x3) << 6);
		++p;
		++length;
		if (!*c) return length;
		*p = x >> 2;	// No partial bytes

		x = value(*c++);
		if (x < 0) return -1;
		*p |= ((x & 0xf) << 4);
		++length;
		if (!*c) return length;
		++p;
		*p = x >> 4;

		x = value(*c++);
		if (x < 0) return -1;
		*p |= (x << 2);
		++p;
		++length;
	}
}
