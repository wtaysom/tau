/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include "fcalls.h"
#include "util.h"

void OpenTest (void) {
  int fd;
  char *name = RndName(9);
  CrFile(name, 1377);

  int fd1 = open(name, O_RDONLY, 0600);
  int fd2 = open(name, O_RDONLY, 0600);
  close(fd1);
  close(fd2);

  fd = openErr(EEXIST, name, O_CREAT | O_EXCL | O_TRUNC | O_WRONLY, 0600);

  unlink(name); 
  free(name);
}
