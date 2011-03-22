/*
 * Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdio.h>
#include <string.h>

int fine_ge(int n, char *s, char c) {
	int x, left, right;

	left = 0;
	right = n - 1;
	while (left <= right) {
		x = (left + right) / 2;
		if (c >= s[x]) {
			left = x + 1;
		} else {
			right = x - 1;
		}
	}
	return right;
}

int find_le(int n, char *s, char c) {
	int x, left, right;

	left = 0;
	right = n - 1;
	while (left <= right) {
		x = (left + right) / 2;
		if (c <= s[x]) {
			right = x - 1;
		} else {
			left = x + 1;
		}
	}
	return left;
}

void find(char *s, char c) {
	int n = strlen(s);
	int ge = fine_ge(n, s, c);
	int le = find_le(n, s, c);

	printf("%s\n", s);
	printf(" %d %c <= %c\n", le, c, s[le]);
	printf(" %d %c >= %c\n", ge, c, s[ge]);
}

int main(int argc, char *argv[]) {
	char *s = "bdetuv";

	printf(" >= <=\n");
	find(s, 'd');
	find(s, 'a');
	find(s, 'z');
	find(s, 'p');

	return 0;
}
