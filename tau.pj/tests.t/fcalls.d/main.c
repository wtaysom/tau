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
char BigFile[] = "bigfile";
char NilFile[] = "nilfile";
char OneFile[] = "onefile";

static bool Randomize = TRUE;
bool Verbose = FALSE;

static void cr_file (char *name, u64 size)
{
	char	buf[SZ_BLOCK];
	int	fd;
	u64	offset;

	fd = creat(name, 0666);
	for (offset = 0; size; offset += SZ_BLOCK) {
		unint n = SZ_BLOCK;
		if (n > size) n = size;
		fill(buf, n, offset);
		write(fd, buf, n);
		size -= n;
	}
	close(fd);
}

/*
 * init_test creates a scratch directory in the given directory
 * for the test.
 */
void init_test (char *dir)
{
	char	*suffix;

	if (Randomize) srandom(nsecs());
	chdir(dir);
	chdirErr(ENOTDIR, "/etc/passwd");

	suffix = rndname(10);
	Scratch = mkstr("scratch_", suffix, 0);
	free(suffix);
	mkdir(Scratch, 0777);
	chdir(Scratch);

	cr_file(BigFile, SZ_BIG_FILE);
	cr_file(NilFile, 0);
	cr_file(OneFile, 1);
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
	pr_usage("[-rv] [<directory>]*\n"
		"\tr - Don't see random number generator\n"
		"\tv - Verbose");
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
