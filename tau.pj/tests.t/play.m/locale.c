/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

#include <stdio.h>
#include <locale.h>

int main (int argc, char *argv[])
{
	setlocale(LC_NUMERIC, "en_US");
	printf("%d\n", 100000);
	printf("%'d\n", 100000);
	return 0;
}
