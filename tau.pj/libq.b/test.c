/*
 * Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdio.h>

const char TestMsg[] = "\nTEST FAILED: %s\n";

int testError (char *err)
{
	printf(TestMsg, err);
	return 0;
}
