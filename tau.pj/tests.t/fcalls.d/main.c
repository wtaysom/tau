/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <style.h>
#include <eprintf.h>
#include <puny.h>
#include <timer.h>

#include <fcalls.h>
#include <util.h>

void rw_test(void);

/*
 * init_test creates test files that can be used by any test.
 * Some of these take a long time to create, so we only do it once.
 */

char *Scratch;
char BigFile[] = "scratch/bigfile";
char NilFile[] = "scratch/nilfile";
char OneFile[] = "scratch/onefile";

static bool Randomize = TRUE;
bool Verbose = FALSE;

/*
 * init_test creates a scratch directory in the given directory
 * for the test.
 */
void init_test (char *dir)
{
	if (Randomize) srandom(nsecs());
	chdir(dir);
	chdirErr(ENOTDIR, "/etc/passwd");

	Scratch = rndname(8);
	mkdir(Scratch, 0777);
	chdir(Scratch);
}

/*
 * cleanup cleans up the scratch direcorty by removing it and
 * eveything under it.
 */
void cleanup (void)
{
	char	*cmd;
	int	rc;

	chdir("..");
	cmd = mkstr("rm -fr ", Scratch, NULL);

	rc = system(cmd);
	if (rc == -1) error("%s:", cmd);
	free(cmd);
	free(Scratch);
	if (Verbose) printf("Verbose\n");
}	

void all_tests (char *dir)
{
	void rw_test(void);

	init_test(dir);

	rw_test();

	cleanup();
}

bool myopt (int c)
{
	switch (c) {
	case 'r':
		Randomize = FALSE;
		break;
	case 'v':
		Verbose = TRUE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void usage (void)
{
	pr_usage("-r [<directory>]*");
}

int main (int argc, char *argv[])
{
	int	i;

	punyopt(argc, argv, myopt, "rv");
	if (argc == optind) {
		all_tests(Option.dir);
	} else for (i = optind; i < argc; i++) {
		all_tests(argv[i]);
	}
	return 0;
}
