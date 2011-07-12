/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */
/*
 * fcalls tests the file system call. By providing a list of mount
 * points or directories, several different file systems can be tested.
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

char *Scratch;
char BigFile[] = "bigfile";
char NilFile[] = "nilfile";
char OneFile[] = "onefile";

static bool Randomize = TRUE;
bool	Verbose = FALSE;
unint	BlockSize = 4096;
s64	SzBigFile = (1LL << 32) + (1LL << 13) + 137;

/* cr_file creates a file of specified size and fills it with data. */
static void cr_file (char *name, u64 size)
{
	u8	buf[BlockSize];
	int	fd;
	u64	offset;

	fd = creat(name, 0666);
	for (offset = 0; size; offset += BlockSize) {
		unint n = BlockSize;
		if (n > size) n = size;
		fill(buf, n, offset);
		write(fd, buf, n);
		size -= n;
	}
	close(fd);
}

/*
 * init_test creates a scratch directory in the specified
 * test directory.
 *
 * It also creates some test files to help in testing. Because,
 * we want to test >4 Gigfiles but it takes time to create a file
 * that big, we create it here and allow its size to be set by
 * the user -- set in small when developing the tests.
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

	cr_file(BigFile, SzBigFile);
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
	void dir_test(void);
	void rw_test(void);

	init_test(dir);

	rw_test();
	dir_test();

	cleanup();
}

bool myopt (int c)
{
	switch (c) {
	case 'b':
		BlockSize = strtoll(optarg, NULL, 0);
		break;
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
		"\tb - Block size [%ld]\n"
		"\tr - Don't seed random number generator\n"
		"\tv - Verbose\n"
		"\tz - size of big file [0x%llx]",
		BlockSize, SzBigFile);
}

int main (int argc, char *argv[])
{
	int	i;

	Option.file_size = SzBigFile;
	Option.dir = ".";
	punyopt(argc, argv, myopt, "b:rv");
	SzBigFile = Option.file_size;
	
	if (SzBigFile < (1LL << 32)) {
		fprintf(stderr, "Size of big file only 0x%llx\n", SzBigFile);
	}
	if (argc == optind) {
		all_tests(Option.dir);
	} else for (i = optind; i < argc; i++) {
		all_tests(argv[i]);
	}
	return 0;
}
