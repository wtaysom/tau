/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdio.h>
#include <stdlib.h>

#include <mylib.h>
#include <style.h>

enum {	Q_SZ = 200,
	RANGE = 1000,
	INCS  = 100 };

typedef struct atom_s {
	int	key;
	int	cnt;
} atom_s;

atom_s Cir[Q_SZ];
atom_s *Head = Cir;
atom_s *Tail = Cir;
int Iterations = 0;
int Incs = 0;

void swap(atom_s *p)
{
	atom_s	t;

	if (p == Head) return;

	if (p == Cir) {
		t = *p;
		*p = Cir[Q_SZ - 1];
		Tail[Q_SZ - 1] = t;
		return;
	}
	t = *p;
	*p = p[-1];
	p[-1] = t;
}

void inc(int key)
{
	atom_s *p;

	++Incs;
	for (p = Head; p != Tail; ) {
	++Iterations;
		if (p->key == key) {
			++p->cnt;
			swap(p);
			return;
		}
		if (++p == &Cir[Q_SZ]) {
			p = Cir;
		}
	}
	if (Head == Cir) {
		Head = &Cir[Q_SZ - 1];
	} else {
		--Head;
	}
	if (Head == Tail) {
		if (Tail == Cir) {
			Tail = &Cir[Q_SZ - 1];
		} else {
			--Tail;
		}
	}
	p = Head;
	p->key = key;
	p->cnt = 1;
}

void dump(void)
{
	atom_s *p;

	for (p = Head; p != Tail; ) {
		printf("%2d %3d\n", p->key, p->cnt);
		if (++p == &Cir[Q_SZ]) {
			p = Cir;
		}
	}
	printf("Incs=%d Iterations=%d  %g iter/inc\n",
		Incs, Iterations, (double)Iterations/(double)Incs);
}

int main(int argc, char *argv[])
{
	int n = INCS;
	int r = RANGE;
	int i;
	int x;

	if (argc > 1) {
		r = atoi(argv[1]);
	}
	if (argc > 2) {
		n = atoi(argv[2]);
	}
	for (i = 0; i < n; i++) {
		x = exp_dist(r);
//printf("%d ", x);
		inc(exp_dist(r));
	}
//printf("\n");
	dump();
	return 0;
}
