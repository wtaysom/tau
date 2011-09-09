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
bool IsRoot;

static bool Randomize = TRUE;
bool Verbose = FALSE;
bool Flaky = FALSE;
bool Test_sparse = FALSE;
snint BlockSize = 4096;
s64 SzBigFile = (1LL << 32) + (1LL << 13) + 137;

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
  IsRoot = (geteuid() == 0);
    
  chdir(dir);
  chdirErr(ENOTDIR, "/etc/passwd");

  suffix = RndName(10);
  Scratch = Mkstr("scratch_", suffix, 0);
  free(suffix);
  mkdir(Scratch, 0777);
  chdir(Scratch);

  CrFile(BigFile, SzBigFile);
  CrFile(EmptyFile, 0);
  CrFile(OneFile, 1);
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

  init_test(dir);

  void StatTest(void);   StatTest();
  void FsTest(void);     FsTest();
  void OpenTest(void);   OpenTest();
#if __linux__
  void OpenatTest(void); OpenatTest();
#endif
  void RwTest(void);     RwTest();
  void DirTest(void);    DirTest();

  cleanup();
}

bool myopt (int c) {
  switch (c) {
  case 'b':
    BlockSize = strtoll(optarg, NULL, 0);
    break;
  case 'f':
    Flaky = TRUE;
    break;
  case 'r':
    Randomize = FALSE;
    break;
  case 's':
    Test_sparse = TRUE;
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
  pr_usage("[-prsvh] [<directory>]*\n"
    "\th - help\n"
    "\tb - Block size [default = %ld]\n"
    "\tf - Flaky - run the flaky tests (may fail)\n"
    "\tp - Print results\n"
    "\tr - Don't seed random number generator\n"
    "\ts - run tests that require sparse file support\n"
    "\tv - Verbose - display each file system call\n"
    "\tz - size of big file [default = 0x%llx]",
    BlockSize, SzBigFile);
}

int main (int argc, char *argv[]) {
  extern void DumpRecords(void);
  int i;

  Option.file_size = SzBigFile;
  Option.dir = ".";
  punyopt(argc, argv, myopt, "b:frsvy");
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
  if (Option.print) DumpRecords();
  return 0;
}
