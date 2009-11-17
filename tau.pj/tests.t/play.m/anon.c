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

#include <style.h>
#include <debug.h>

typedef struct s1_s {
	u32	a;
	u32	b;
} s1_s;

typedef struct s2_s {
	//Can't do anounynmous structures like Plan 9 C
	//s1_s;
	struct {
		u32	a;
		u32	b;
	};
	u32	c;
} s2_s;

int main (int argc, char *argv[])
{
	s2_s	q;

	q.a = 3;
	q.b = 4;
	q.c = 5;
	prmem("q", &q, sizeof(q));
	return 0;
}
