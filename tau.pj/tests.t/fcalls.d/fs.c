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
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include <debug.h>

#include "fcalls.h"
#include "main.h"
#include "util.h"

void FsTest (void) {
#ifdef __linux__
  struct statfs s1;
  struct statfs s2;
  struct statvfs v1;
  struct statvfs v2;
  int fd = open(".", O_RDONLY, 0);
  statfs(".", &s1);
  fstatfs(fd, &s2);
  statvfs(".", &v1);
  fstatvfs(fd, &v2);
  if (Local_option.flaky) {
    /* Can't be sure file system values won't change between calls */
    CheckEq(&s1, &s2, sizeof(s1));
    CheckEq(&v1, &v2, sizeof(v1));
    Check(s1.f_bsize   == v1.f_bsize);
    Check(s1.f_blocks  == v1.f_blocks);
    Check(s1.f_bfree   == v1.f_bfree);
    Check(s1.f_bavail  == v1.f_bavail);
    Check(s1.f_files   == v1.f_files);
    Check(s1.f_ffree   == v1.f_ffree);
    Check(s1.f_namelen == v1.f_namemax);
  }
  close(fd);
#endif
}

void StatTest (void) {
  enum { FILE_SZ    = 1759,
         FILE_TRUNC = 17,
         FILE_GROW  = 10937,
         USER_A     = 12345,
         GROUP_A    = 23456,
         USER_B     = 65431,
         GROUP_B    = 76543 };
  struct stat s1;
  struct stat s2;
  char *name = RndName();
  CrFile(name, FILE_SZ);
  stat(name, &s1);
  int fd = open(name, O_RDONLY, 0);
  fstat(fd, &s2);
  CheckEq(&s1, &s2, sizeof(s1));
  Check(s1.st_size == FILE_SZ);
  close(fd);

  if (Local_option.is_root) {
    chown(name, USER_A, GROUP_A);
    stat(name, &s1);
    Check(s1.st_uid == USER_A);
    Check(s1.st_gid == GROUP_A);

    fd = open(name, O_RDWR, 0);
    fchown(fd, USER_B, GROUP_B);
    fstat(fd, &s1);
    Check(s1.st_uid == USER_B);
    Check(s1.st_gid == GROUP_B);
    close(fd);
  }
  /* Truncate can both shrink and grow a file */
  truncate(name, FILE_TRUNC);
  stat(name, &s1);
  Check(s1.st_size == FILE_TRUNC);
  truncate(name, FILE_GROW);
  stat(name, &s1);
  Check(s1.st_size == FILE_GROW);

  fd = open(name, O_RDWR, 0);
  ftruncate(fd, FILE_TRUNC);
  fstat(fd, &s1);
  Check(s1.st_size == FILE_TRUNC);
  ftruncate(fd, FILE_GROW);
  fstat(fd, &s1);
  Check(s1.st_size == FILE_GROW);
  close(fd);

  fd = open(name, O_RDONLY, 0);
  ftruncateErr(EINVAL, fd, FILE_TRUNC);
  fstat(fd, &s1);
  Check(s1.st_size == FILE_GROW);
  close(fd);

  /* Dup - fds share seek pointer */
  int fd1 = open(name, O_RDWR | O_TRUNC, 0);
  int fd2 = dup(fd1);
  char b[10];
  write(fd1, "abc", 3);
  lseek(fd1, 0, 0);
  read(fd1, b, 1);
  Check(b[0] == 'a');
  read(fd2, b, 1);
  Check(b[0] == 'b');
  close(fd1);
  close(fd2);
  fd2 = dupErr(EBADF, fd1);

  /* Dup2 - like dup but can pick fd number */
  /* TODO(taysom) add more tests */
  fd1 = open(name, O_RDWR | O_TRUNC, 0);
  fd2 = open(name, O_RDONLY, 0);
  fd2 = dup2(fd1, fd2);
  write(fd1, "abc", 3);
  lseek(fd1, 0, 0);
  read(fd1, b, 1);
  Check(b[0] == 'a');
  read(fd2, b, 1);
  Check(b[0] == 'b');
  close(fd1);
  close(fd2);

  /* Link */
  char *name2 = RndName();
  link(name, name2);
  stat(name, &s1);
  Check(s1.st_nlink == 2);
  unlink(name2);
  free(name2);
  stat(name, &s1);
  Check(s1.st_nlink == 1);
  /* TODO(taysom): need to try 64K links */

  unlink(name);
  free(name);
}
