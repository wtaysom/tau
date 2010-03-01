/*
 * Robert Sedgewick "Algorithms: 2nd Edition," pp 148-152.
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <style.h>
#include <mylib.h>

enum {	MAX_HEAP = 1<<16 };

typedef struct heap_s {
	int	n;
	int	a[MAX_HEAP];
} heap_s;

void newline (void)
{
	putchar('\n');
}

void pr_heap (heap_s *h)
{
	int	i;

	for (i = 1; i <= h->n; i++) {
		printf("%d. %d\n", i, h->a[i]);
	}
}

static void upheap (heap_s *h)
{
	int	*a = h->a;
	int	k = h->n;
	int	j = k/2;
	int	v;

	v = a[k];
	a[0] = INT_MIN;
	while (a[j] > v) {
		a[k] = a[j];
		k = j;
		j = k/2;
	}
	a[k] = v;
}

static void downheap (heap_s *h, int k)
{
	int	*a = h->a;
	int	n = h->n;
	int	v;
	int	j;

	v = a[k];
	while (k <= n/2) {
		j = k + k;
		if (j < n) {
			if (a[j] > a[j+1]) {
				++j;
			}
		}
		if (v < a[j]) {
			break;
		}
		a[k] = a[j];
		k = j;
	}
	a[k] = v;
}

void insert_heap (heap_s *h, int v)
{
	h->a[++(h->n)] = v;
	upheap(h);
}

int remove_heap (heap_s *h)
{
	int	*a = h->a;
	int	v;

	v = a[1];
	a[1] = a[h->n];
	--(h->n);
	downheap(h, 1);
	return v;
}

int main (int argc, char *argv[])
{
	heap_s	h;
	int	i;
	int	x;

	zero(h);
	for (i = 0; i < 100; i++) {
		x = range(9);
		insert_heap( &h, x);
	}
	pr_heap( &h);
	newline();
	for (i = 0; i < 100; i++) {
		x = remove_heap( &h);
		printf("%d. %d\n", i, x);
	}
	return 0;
}
