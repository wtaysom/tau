/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdio.h>
#include <stdlib.h>

#include <eprintf.h>
#include <style.h>

void cleanup (void)
{
	printf("Cleaning up\n");
}

int main (int argc, char *argv[])
{
	Fatal_cleanup = cleanup;
	fatal("Test cleanup");
	return 0;
}

