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
 * Linear hashing using dynamic arrays. Based off of Per-Ake Larson's
 * "Dynamic Hash Tables," Communications of the ACM, April 1988 Vol 31
 * No. 4, pp 446-457.
 * Dynamic arrays - An array structured as a tree that can grow dynamically
 */

#include <stdio.h>
#include <stdlib.h>

#include <style.h>
#include <eprintf.h>
#include <debug.h>
#include <mylib.h>
#include <timer.h>
#include <crc.h>

enum {	VOID_SHIFT  = (sizeof(void *) == 8) ? 3 : 2,
	ALLOC_SHIFT = 4, // Should be about 12 in real life
	ALLOC_SIZE  = 1 << ALLOC_SHIFT,
	SHIFT       = ALLOC_SHIFT - VOID_SHIFT,
	SLOTS       = 1 << SHIFT,
	MASK        = SLOTS - 1 };

typedef struct dyna_s {
	unint	dy_depth;
	unint	dy_numbuckets;
	unint	dy_numrecs;
	unint	dy_split;
	void	**dy_node;
} dyna_s;

dyna_s init_dyna (void)
{
	dyna_s	dy;

	dy.dy_depth      = 1;
	dy.dy_numrecs    = 0;
	dy.dy_split      = 0;
	dy.dy_numbuckets = SLOTS;
	dy.dy_node = ezalloc(ALLOC_SIZE);

	return dy;
}

void pr_indent (unint indent)
{
	while (indent--) {
		printf("    ");
	}
}

void pr_node (unint depth, void **node, dyna_s *dy, unint indent)
{
	unint	i;

	pr_indent(indent);
	printf("%p\n", node);
	if (depth && node) {
		for (i = 0; i < SLOTS; i++) {
			pr_node(depth - 1, node[i], dy, indent+1);
		}
	}
}

void pr_dyna (dyna_s *dy)
{
	pr_node(dy->dy_depth, dy->dy_node, dy, 0);
	printf("numrecs=%lu split=%lu\n", dy->dy_numrecs, dy->dy_split);
}
	

void *ith (dyna_s *dy, unint i)
{
	unint	depth  = dy->dy_depth;
	unint	shift  = depth * SHIFT;
	unint	max    = 1 << shift;
	void	**node = dy->dy_node;
	void	*next;
	unint	j;

	while (i >= max) {
		void	**new;

		new    = ezalloc(ALLOC_SIZE);
		new[0] = node;
		dy->dy_node = new;
		dy->dy_depth = ++depth;
		node = new;
		shift += SHIFT;
		max = 1 << shift;
	}		
	for (; depth > 1; depth--) {
		shift -= SHIFT;
		j = (i >> shift) & MASK;
		next = node[j];
		if (!next) {
			next = ezalloc(ALLOC_SIZE);
			node[j] = next;
		}
		node = next;
	}	
	return &node[i & MASK];
}

typedef struct x_s	x_s;
struct x_s {
	x_s	*x_next;
	u32	x_val;
};

dyna_s	Buckets;

x_s **hash (u32 x)
{
	unint	h;
	unint	i;
	unint	mask;

#if 0
	h = crc32( &x, sizeof(x));
	h = x;
	h = x * 1610612741;
#endif
	h = x * 1610612741;
	mask = Buckets.dy_numbuckets-1;
	i = h & mask;
	if (i < Buckets.dy_split) {
		mask = (mask << 1) | 1;
		i = h & mask;
	}
	return ith( &Buckets, i);
}

x_s *find (u32 x)
{
	x_s	**head;
	x_s	*xs;

	head = hash(x);
	for (xs = *head; xs && (xs->x_val != x); xs = xs->x_next)
		;
	return xs;
}

void rehash (x_s **head)
{
	x_s	*xs;
	x_s	*next;

	xs = *head;
	*head = NULL;
	for (; xs; xs = next) {
		next = xs->x_next;
		head = hash(xs->x_val);
		xs->x_next = *head;
		*head = xs;
	}
}

void grow (void)
{
	unint	n = Buckets.dy_numbuckets;
	x_s	**head;
	unint	i;

	if (Buckets.dy_numrecs < (Buckets.dy_numbuckets + Buckets.dy_split)) return;
	i = Buckets.dy_split;
	n = i + SLOTS;
	Buckets.dy_split += SLOTS;
	for (; i < n; i++) {
		head = ith( &Buckets, i);
		rehash(head);
	}
	if (Buckets.dy_split == Buckets.dy_numbuckets) {
		Buckets.dy_numbuckets <<= 1;
		Buckets.dy_split = 0;
	}
}

void store (u32 x)
{
	x_s	**head;
	x_s	*xs;

	grow();
	xs = ezalloc(sizeof(x_s));
	xs->x_val = x;
	head = hash(x);
	xs->x_next = *head;
	*head = xs;
	++Buckets.dy_numrecs;
}

void shrink (void)
{
	unint	n = Buckets.dy_numbuckets;
	x_s	**head;
	unint	i;

	if (Buckets.dy_numrecs > (Buckets.dy_numbuckets + Buckets.dy_split)) return;
	n = Buckets.dy_numbuckets + Buckets.dy_split;
	if (Buckets.dy_split == 0) {
		if (Buckets.dy_numbuckets == SLOTS) return;
		n = Buckets.dy_numbuckets;
		Buckets.dy_numbuckets >>= 1;
		Buckets.dy_split = Buckets.dy_numbuckets;
	}
	i = n - SLOTS;
PRd(n);
	Buckets.dy_split -= SLOTS;
	for (; i < n; i++) {
		head = ith( &Buckets, i);
		rehash(head);
	}
}

void delete (u32 x)
{
	x_s	**head;
	x_s	*xs;
	x_s	*prev;

	shrink();
	head = hash(x);
	xs = *head;
	if (!xs) return;
	if (xs->x_val == x) {
		*head = xs->x_next;
		--Buckets.dy_numrecs;
		free(xs);
	}
	for (;;) {
		prev = xs;
		xs = xs->x_next;
		if (!xs) return;
		if (xs->x_val == x) {
			prev->x_next = xs->x_next;
			--Buckets.dy_numrecs;
			free(xs);
			return;
		}
	}
}

void dump (void)
{
	x_s	**head;
	x_s	*xs;
	int	n;
	int	i;

PRd(Buckets.dy_numbuckets);
PRd(Buckets.dy_split);
	n = Buckets.dy_numbuckets + Buckets.dy_split;
	for (i = 0; i < n; i++) {
		printf("%3d.", i);
		head = ith( &Buckets, i);
		for (xs = *head; xs; xs = xs->x_next) {
			printf(" %2d", xs->x_val);
		}
		printf("\n");
	}
}

int main (int argc, char *argv[])
{
	int	n = 100;
	u32	x;
	int	i;

	if (argc > 1) {
		n = atoi(argv[1]);
	}
	Buckets = init_dyna();
	for (i = 0; i < n; i++) {
		//x = range(n);
		x = i;
		if (find(x)) {
			//delete(x);
		} else {
			store(x);
		}
	}
	dump();
	for (i = 0; i < n*4/8; i++) {
		x = i;
		delete(x);
	}
	//pr_dyna( &Buckets);
	dump();
	return 0;
}
