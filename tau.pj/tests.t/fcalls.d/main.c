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
#include "main.h"
#include "util.h"

char *Scratch;
char BigFile[]   = "bigfile";
char EmptyFile[] = "emptyfile";
char OneFile[]   = "onefile";
bool IsRoot;

MyOption_s My_option = {
  .size_big_file = (1LL << 32) + (1LL << 13) + 137,
  .block_size    = 4096,
  .exit_on_error = TRUE,
  .stack_trace   = TRUE,
  .is_root       = FALSE,
  .flaky         = FALSE,
  .verbose       = FALSE,
  .seed_rand     = FALSE,
  .test_sparse   = FALSE,
  .test_direct   = FALSE };

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

  if (My_option.seed_rand) srandom(nsecs());
  IsRoot = (geteuid() == 0);
    
  chdir(dir);
  chdirErr(ENOTDIR, "/etc/passwd");

  suffix = RndName(10);
  Scratch = Mkstr("scratch_", suffix, 0);
  free(suffix);
  mkdir(Scratch, 0777);
  chdir(Scratch);

  CrFile(BigFile, My_option.size_big_file);
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
    My_option.block_size = strtoll(optarg, NULL, 0);
    break;
  case 'f':
    My_option.flaky = TRUE;
    break;
  case 'o':
    My_option.test_direct = TRUE;
    break;
  case 'r':
    My_option.seed_rand = TRUE;
    break;
  case 's':
    My_option.test_sparse = TRUE;
    break;
  case 'v':
    My_option.verbose = TRUE;
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

void usage (void) {
  pr_usage("[-foprsvh] [<directory>]*\n"
    "\th - help\n"
    "\tb - Block size [default = %ld]\n"
    "\tf - My_option.flaky - run the flaky tests (may fail)\n"
    "\to - Test O_DIRECT\n"
    "\tp - Print results\n"
    "\tr - Don't seed random number generator\n"
    "\ts - Sparse file tests\n"
    "\tv - Verbose - display each file system call\n"
    "\tz - Size of big file [default = 0x%llx]",
    My_option.block_size, My_option.size_big_file);
}

int main (int argc, char *argv[]) {
  extern void DumpRecords(void);
  int i;

  Option.file_size = My_option.size_big_file;
  Option.dir = ".";
  punyopt(argc, argv, myopt, "b:forsv");
  My_option.size_big_file = Option.file_size;

  if (My_option.size_big_file < 40000) {
    PrError("Size of big file, %lld, should be greater than 40K",
            My_option.size_big_file);
    return 2;
  }
  if (My_option.size_big_file < (1LL << 32)) {
    fprintf(stderr, "Size of big file only 0x%llx\n",
            My_option.size_big_file);
  }
  if (argc == optind) {
    all_tests(Option.dir);
  } else for (i = optind; i < argc; i++) {
    all_tests(argv[i]);
  }
  if (Option.print) DumpRecords();
  return 0;
}
