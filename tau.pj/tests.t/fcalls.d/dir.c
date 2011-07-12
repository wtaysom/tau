/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

/*
 * Test directory operations
 */

#include "fcalls.h"
#include "util.h"

static void simple (void)
{
	int	num_children = 20;
	int	i;
	char 	*child[num_children];
	char	*parent = rndname(10);
	DIR	*dir;
	struct dirent *ent;

	mkdir(parent, 0777);
	chdir(parent);
	for (i = 0; i < num_children; i++) {
		child[i] = rndname(10);
		mkdir(child[i], 0777);
	}
	dir = opendir(".");
	for (;;) {
		ent = readdir(dir);
		if (!ent) break;
	}
	closedir(dir);
	for (i = 0; i < num_children; i++) {
		rmdir(child[i]);
		child[i] = 0;
	}
	chdir("..");
	rmdir(parent);
}
	
void dir_test (void)
{
	simple();
}
