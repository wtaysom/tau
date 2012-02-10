/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include <eprintf.h>

static FILE *Fp;

void open_file (char *file)
{
	if (Fp) fatal("File already open");
	Fp = fopen(file, "r");
	if (!Fp) fatal("couldn't open %s", file);
}

void close_file (void)
{
	fclose(Fp);
	Fp = NULL;
}

int get (void)
{
	return getc(Fp);
}

void unget (int c)
{
	ungetc(c, Fp);
}
