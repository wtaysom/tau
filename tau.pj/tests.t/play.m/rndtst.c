#include <stdlib.h>
#include <stdio.h>
#include <style.h>
#include <mylib.h>

int main (int argc, char *argv[])
{
	unint	n = 1000000;
	unint	i;

	if (argc > 1) {
		n = atoi(argv[1]);
	}
#if 0
	for (i = 0; i < n; i++) {
		range(i);
	}
#endif
	for (i = 0; i < n; i++) {
		double	x;
		u64	y;

		x = drand48();
		x = x*x*x*x*x;
		y = 1/x * 4000.0;
		printf("%10lld\n", y);
	}
	return 0;
}
