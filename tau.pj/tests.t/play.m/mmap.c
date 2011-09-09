/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <sys/mman.h>

#include <debug.h>
#include <eprintf.h>

enum { SIZE = 1<<24 };

int main (int argc, char *argv[]) {
  void *m;
  m = mmap(NULL, SIZE, PROT_WRITE | PROT_READ,
           MAP_ANONYMOUS | MAP_PRIVATE /*| MAP_HUGETLB*/,
           -1, 0);
PRp(m);
  if (m == MAP_FAILED) fatal("mmap:");
  int rc = munmap(m, SIZE);
  if (rc) fatal("munmap:");
  return 0;
}
