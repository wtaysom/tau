 /*
  * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
  * Use of this source code is governed by a BSD-style license that
  * can be found in the LICENSE file.
  */

#include <stdio.h>

int main (int argc, char *argv[]) {
	char	a[] = "0123456789";
	char	b[] = "0123456789";
	char	c[] = "0123456789";
	int	rc;

	rc = snprintf(a, 5, "abcd");
	printf("%d %s\n", rc, a);
	rc = snprintf(b, 5, "abcde");
	printf("%d %s\n", rc, b);
	rc = snprintf(c, 5, "abcdef");
	printf("%d %s\n", rc, c);
	return 0;
}
