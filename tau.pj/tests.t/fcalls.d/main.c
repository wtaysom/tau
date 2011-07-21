/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* fcalls tests the file system calls. By providing a list of mount
 * points or directories, several different file systems can be tested.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <debug.h>
#include <eprintf.h>
#include <puny.h>
#include <style.h>
#include <timer.h>

#include "fcalls.h"
#include "util.h"

char *Scratch;
char BigFile[]   = "bigfile";
char EmptyFile[] = "emptyfile";
char OneFile[]   = "onefile";

static bool Randomize = TRUE;
bool Verbose = FALSE;
snint BlockSize = 4096;
s64 SzBigFile = (1LL << 32) + (1LL << 13) + 137;

/* cr_file creates a file of specified size and Fills it with data. */
static void cr_file (char *name, u64 size) {
  u8 buf[BlockSize];
  int fd;
  u64 offset;

  fd = creat(name, 0666);
  for (offset = 0; size; offset += BlockSize) {
    unint n = BlockSize;
    if (n > size) n = size;
    Fill(buf, n, offset);
    write(fd, buf, n);
    size -= n;
  }
  close(fd);
}

/* init_test creates a scratch directory in the specified
 * test directory.
 *
 * It also creates some test files to help in testing. Because,
 * we want to test >4 Gigfiles but it takes time to create a file
 * that big, we create it here and allow its size to be set by
 * the user -- use a small size when developing the tests.
 */
void init_test (char *dir) {
  char *suffix;

  if (Randomize) srandom(nsecs());
  chdir(dir);
  chdirErr(ENOTDIR, "/etc/passwd");

  suffix = RndName(10);
  Scratch = Mkstr("scratch_", suffix, 0);
  free(suffix);
  mkdir(Scratch, 0777);
  chdir(Scratch);

  cr_file(BigFile, SzBigFile);
  cr_file(EmptyFile, 0);
  cr_file(OneFile, 1);
}

/* cleanup deletes the scratch directory eveything under it. */
void cleanup (void) {
  char *cmd;
  int rc;

  chdir("..");
  cmd = Mkstr("rm -fr ", Scratch, NULL);

  rc = system(cmd);
  if (rc == -1) PrError("%s:", cmd);
  free(cmd);
  free(Scratch);
}

void all_tests (char *dir) {
  void DirTest(void);
  void rw_test(void);

  init_test(dir);

  rw_test();
  DirTest();

  cleanup();
}

bool myopt (int c) {
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

void usage (void) {
  pr_usage("[-rvh] [<directory>]*\n"
    "\th - help\n"
    "\tb - Block size [default = %ld]\n"
    "\tr - Don't seed random number generator\n"
    "\tv - Verbose - display each file system call\n"
    "\tz - size of big file [default = 0x%llx]",
    BlockSize, SzBigFile);
}

int main (int argc, char *argv[]) {
  int i;

  Option.file_size = SzBigFile;
  Option.dir = ".";
  punyopt(argc, argv, myopt, "b:rv");
  SzBigFile = Option.file_size;

  if (SzBigFile < 40000) {
    PrError("Size of big file, %lld, should be greater than 40K",
        SzBigFile);
    return 2;
  }
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
