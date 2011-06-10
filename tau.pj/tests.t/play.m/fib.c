#include <stdio.h>
#include <unistd.h>

#include <style.h>

int main (int argc, char *argv[]) {
	u64	x = 1;
	u64	y = 1;
	u64	t;
	u64	i;

	//printf("plot \"-\" ");
	for (i = 0; i < 40; i++) {
		printf("%llu %llu\n", i, x);
		fflush(stdout);
		sleep(1);
		t = y;
		y = x + y;
		x = t;
	}
	return 0;
}
