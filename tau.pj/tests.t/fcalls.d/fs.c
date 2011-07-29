/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifdef __linux__
#include <sys/statvfs.h>
#include <sys/vfs.h>
#else
#include <sys/param.h>
#include <sys/mount.h>
#endif
#include <fcntl.h>

#include <debug.h>

#include "fcalls.h"
#include "util.h"

void FsTest (void) {
  struct statfs s1;
  struct statfs s2;
  struct statvfs v1;
  struct statvfs v2;
  int fd = open(".", O_RDONLY, 0);
  statfs(".", &s1);
  fstatfs(fd, &s2);
  IsEq(&s1, &s2, sizeof(s1));
  statvfs(".", &v1);
  fstatvfs(fd, &v2);
  IsEq(&v1, &v2, sizeof(v1));
  close(fd);
}
