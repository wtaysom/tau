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
atom_s *Start = Cir;
atom_s *End = &Cir[Q_SZ];
int Iterations = 0;
int Incs = 0;

void swap(atom_s *p)
{
	atom_s	t;

	if (p == Start) return;

	if (p == Cir) {
		t = *p;
		*p = End[-1];
		End[-1] = t;
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
	for (p = Start; p < End; p++) {
++Iterations;
		if (p->key == key) {
			++p->cnt;
			goto done;
		} else if (!p->key) {
			p->key = key;
			p->cnt = 1;
			goto done;
		}
	}
	for (p = Cir; p < Start; p++) {
++Iterations;
		if (p->key == key) {
			++p->cnt;
			goto done;
		} else if (!p->key) {
			p->key = key;
			p->cnt = 1;
			goto done;
		}
	}
	if (Start == Cir) {
		p = &End[-1];
	} else {
		p = &Start[-1];
	}
	p->key = key;
	p->cnt = 1;
done:
	swap(p);
}

void dump(void)
{
	atom_s *p;

	for (p = Start; p < End; p++) {
		printf("%2d %3d\n", p->key, p->cnt);
	}
	for (p = Cir; p < Start; p++) {
		printf("%2d %3d\n", p->key, p->cnt);
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
