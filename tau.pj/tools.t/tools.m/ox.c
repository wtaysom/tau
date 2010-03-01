/*
 * This is a hex dump that matches the output that dump gives on windows machines
 * Works only for little endian machines - don't know how dump behaves on big
 * endian machines anyway.
 *
 * It appears to be least significant nibble first
 */
#include <stdio.h>

#include <style.h>
#include <eprintf.h>

enum { BYTES_PER_LINE = 16 };
 void dump (FILE *f)
 {
 	int	c;
	int	i;
	int	line = 0;

	printf("         0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15\n");
 	for (i = 0;;) {
		c = getc(f);
		if (c == EOF) {
			// Finish line
			if (i) putchar('\n');
			return;
		}
		if (i == 0) {
			printf("%6x ", line);
			line += 16;
		}
		printf(" %1x%1x", c & 0xf, (c >> 4) & 0xf);
		++i;
		if (i == 16) {
			putchar('\n');
			i = 0;
		}
	}
 }

 int main (int argc, char *argv[])
 {
	FILE	*f;
	int	i;

 	if (argc == 1) {	// Use stdin
		dump(stdin);
	} else {
		for (i = 1; i < argc; i++) {
			f = fopen(argv[i], "r");
			if (!f) {
				fatal("couldn't fopen %s:", argv[i]);
			}
			dump(f);
		}
	}
	return 0;
}
