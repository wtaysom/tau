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
/*
 * Dynamic arrays - An array structured as a tree that can grow dynamically
 */

#include <stdio.h>
#include <stdlib.h>

#include <style.h>
#include <eprintf.h>
#include <debug.h>
#include <mylib.h>
#include <timer.h>

enum {	VOID_SHIFT  = (sizeof(void *) == 8) ? 3 : 2,
	ALLOC_SHIFT = 5, // Should be about 12 in real life
	ALLOC_SIZE  = 1 << ALLOC_SHIFT,
	SHIFT       = ALLOC_SHIFT - VOID_SHIFT,
	SLOTS       = 1 << SHIFT,
	MASK        = SLOTS - 1 };

typedef struct dyna_s {
	unint	depth;
	unint	shift;
	unint	mask;
	void	**node;
} dyna_s;

dyna_s init_dyna (unint size)
{
	dyna_s	da;
	unint	shift;
	unint	slots;

	for (shift = 0; size > (1 << shift); shift++)
		;
	if (ALLOC_SIZE/2 < (1 << shift)) {
		eprintf("size of dynamic array element too big %ld\n", size);
	}
	slots = 1 << (ALLOC_SHIFT - shift);
	da.shift = shift;
	da.mask  = slots - 1;
	da.depth = 0;
	da.node  = ezalloc(ALLOC_SIZE);

	return da;
}

void pr_indent (unint indent)
{
	while (indent--) {
		printf("    ");
	}
}

void pr_mem (u8 *x, unint size, unint indent)
{
	u8	*end;

	pr_indent(indent);
	for (end = x + size; x < end; x++) {
		printf("%2x ", *x);
	}
	printf("\n");
}

void pr_node (unint depth, void **node, dyna_s *da, unint indent)
{
	unint	i;

	if (!node) return;

	pr_indent(indent);
	printf("%p\n", node);
	if (depth) {
		for (i = 0; i < SLOTS; i++) {
			pr_node(depth - 1, node[i], da, indent+1);
		}
	} else {
		unint	size = (1 << da->shift);

		for (i = 0; i < ALLOC_SIZE; i += size) {
			pr_mem((u8 *)((addr)node + i), size, indent+1);
		}
	}
}

void pr_dyna (dyna_s *da)
{
	pr_node(da->depth, da->node, da, 0);
}


void *ith (dyna_s *da, unint i)
{
	unint	depth  = da->depth;
	unint	shift  = (ALLOC_SHIFT - da->shift) + depth * SHIFT;
	unint	max    = 1 << shift;
	void	**node = da->node;
	void	*next;
	unint	j;

	while (i >= max) {
		void	**new;

		new    = ezalloc(ALLOC_SIZE);
		new[0] = node;
		da->node = new;
		da->depth = ++depth;
		node = new;
		shift += SHIFT;
		max = 1 << shift;
	}
	for (; depth; depth--) {
		shift -= SHIFT;
		j = (i >> shift) & MASK;
		next = node[j];
		if (!next) {
			next = ezalloc(ALLOC_SIZE);
			node[j] = next;
		}
		node = next;
	}
	return (void *)((addr)node + ((i & da->mask) << da->shift));
}

int main (int argc, char *argv[])
{
	int	n = 10000;
	dyna_s	da;
	int	*x;
	int	i;

#if 1
	unint	start, finish;

	if (argc > 1) {
		n = atoi(argv[1]);
	}
	da = init_dyna(sizeof(*x));
	start = nsecs();
	for (i = 0; i < n; i++) {
		x = ith( &da, i & 0xfffff);
		*x = i;
	}
	finish = nsecs();
	printf("%g nsecs/op\n", (double)(finish - start) / n);

#else
	int	y;

	debugon();
	fdebugon();

	printf("SHIFT=%u\n", SHIFT);
	da = init_dyna(sizeof(*x));
	printf("shift=%lu\n", da.shift);
	for (i = 0; i < 20000; i++) {
		y = urand(20000) + 1;
		//y = i;
		x = ith( &da, y);
		if (*x) {
			printf("Collision %u %u %u\n", i, *x, y);
		}
		*x = i;
	}
	pr_dyna( &da);
#endif
	return 0;
}
