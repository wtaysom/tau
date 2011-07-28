/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#define _GNU_SOURCE

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/user.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include <debug.h>

#include "fcalls.h"
#include "util.h"

void OpenTest (void) {
  char buf[BlockSize];
  char *name = RndName(9);
  CrFile(name, 1377);

  /* Open file twice */
  int fd1 = open(name, O_RDONLY, 0600);
  int fd2 = open(name, O_RDONLY, 0600);
  close(fd1);
  close(fd2);

  /* Try to create and existing file */
  openErr(EEXIST, name, O_CREAT | O_EXCL | O_TRUNC | O_WRONLY, 0600);

  /* Append mode on file, !(O_APPEND -> O_WRONLY) */
  char msg1[] = "Now is the time";
  char msg2[] = " for things to happen.";
  char msg3[] = " Well it happened";
  fd1 = open(name, O_TRUNC | O_APPEND | O_RDWR, 0);
  fd2 = open(name, O_APPEND | O_RDWR, 0);
  write(fd1, msg1, sizeof(msg1));
  write(fd2, msg2, sizeof(msg2));
  write(fd1, msg3, sizeof(msg3));
  pread(fd2, buf, sizeof(msg1), 0);
  IsEq(buf, msg1, sizeof(msg1));
  pread(fd1, buf, sizeof(msg2), sizeof(msg1));
  IsEq(buf, msg2, sizeof(msg2));
  pread(fd2, buf, sizeof(msg3), sizeof(msg1) + sizeof(msg2));
  IsEq(buf, msg3, sizeof(msg3));
  close(fd1);
  close(fd2);

  /* Append mode requires write mode, but read mode works */
  fd1 = open(name, O_APPEND, 0);
  read(fd1, buf, sizeof(msg1));
  IsEq(buf, msg1, sizeof(msg1));
  writeErr(EBADF, fd1, msg1, sizeof(msg1));
  close(fd1);

  /* No mode allows read but not writes, because 0 is O_RDONLY */
  fd1 = open(name, 0, 0);
  read(fd1, buf, sizeof(msg1));
  IsEq(buf, msg1, sizeof(msg1));
  writeErr(EBADF, fd1, msg1, sizeof(msg1));
  close(fd1);

  /* Check appended files after closing */
  fd1 = open(name, O_RDONLY, 0);
  fd2 = open(name, O_RDONLY, 0);
  pread(fd2, buf, sizeof(msg1), 0);
  IsEq(buf, msg1, sizeof(msg1));
  pread(fd1, buf, sizeof(msg2), sizeof(msg1));
  IsEq(buf, msg2, sizeof(msg2));
  pread(fd2, buf, sizeof(msg3), sizeof(msg1) + sizeof(msg2));
  IsEq(buf, msg3, sizeof(msg3));
  close(fd1);
  close(fd2);

  /* O_DIRECT */
  int i;
  void *b;
  int rc = posix_memalign(&b, PAGE_SIZE, PAGE_SIZE);
  if (rc) PrError("posix_memalign:");
  fd1 = open(name, O_TRUNC | O_RDWR | O_DIRECT, 0);
  for (i = 0; i < 4; i++) {
    Fill(b, PAGE_SIZE, i * PAGE_SIZE);
    write(fd1, b, PAGE_SIZE);
  }
  lseek(fd1, 0, SEEK_SET);
  for (i = 0; i < 4; i++) {
    read(fd1, b, PAGE_SIZE);
    IsSame(b, PAGE_SIZE, i * PAGE_SIZE);
  }
  lseek(fd1, 0, SEEK_SET);
  writeErr(EINVAL, fd1, ((char *)b + 1), PAGE_SIZE);
  writeErr(EINVAL, fd1, b, PAGE_SIZE - 1);
  readErr(EINVAL, fd1, ((char *)b + 1), PAGE_SIZE);
  readErr(EINVAL, fd1, b, PAGE_SIZE - 1);
  close(fd1);

  /* O_DIRECT, backward seeks */
  fd1 = open(name, O_TRUNC | O_RDWR | O_DIRECT, 0);
  for (i = 3; i >= 0; i--) {
    Fill(b, PAGE_SIZE, i * PAGE_SIZE);
    pwrite(fd1, b, PAGE_SIZE, i * PAGE_SIZE);
  }
  lseek(fd1, 0, SEEK_SET);
  for (i = 3; i >= 0; i--) {
    pread(fd1, b, PAGE_SIZE, i * PAGE_SIZE);
    IsSame(b, PAGE_SIZE, i * PAGE_SIZE);
  }
  close(fd1);

  /* O_SYNC */
  fd1 = open(name, O_TRUNC | O_RDWR | O_SYNC, 0);
  for (i = 0; i < 4; i++) {
    Fill(b, PAGE_SIZE, i * PAGE_SIZE);
    write(fd1, b, PAGE_SIZE);
  }
  lseek(fd1, 0, SEEK_SET);
  for (i = 0; i < 4; i++) {
    read(fd1, b, PAGE_SIZE);
    IsSame(b, PAGE_SIZE, i * PAGE_SIZE);
  }
  close(fd1);

  free(b);
  unlink(name); 
  free(name);
}
