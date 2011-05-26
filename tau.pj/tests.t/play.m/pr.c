/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

#include <stdio.h>

char S[] = "abcdefg";

int main (int argc, char *argv[]) {
	int	i;

	for (i = 0; i < sizeof(S) + 3; i++) {
		printf("%.*s\n", i, S);
	}
	return 0;
}
