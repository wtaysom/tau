/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "fcalls.h"
#include "util.h"

/* Basic read/write tests
 */

/* Simple test used to debug testing infrastructure */
static void Simple (void) {
  enum { BUF_SIZE = 227,   /* Weird number of bytes for buffer */
         REMAINDER = 17};  /* Weird remainder of bytes,
                            * not related to buffer.
                            */
  char buf[BUF_SIZE];
  char file[] = "file_a";
  int fd;

  fd = open(file, O_CREAT | O_TRUNC | O_RDWR, 0666);
  Fill(buf, sizeof(buf), 0);
  write(fd, buf, sizeof(buf));
  Fill(buf, REMAINDER, sizeof(buf));
  write(fd, buf, REMAINDER);

  lseek(fd, 0, 0);

  read(fd, buf, sizeof(buf));
  IsSame(buf, sizeof(buf), 0);
  readCheckSize(fd, buf, sizeof(buf), REMAINDER);
  IsSame(buf, REMAINDER, sizeof(buf));
  close(fd);
}

/* Normal read/lseek tests */
static void rdseek (void) {
  u8 buf[BlockSize];
  int fd;
  s64 offset;
  u64 size;

  /* Read BigFile and verify contents */
  fd = open(BigFile, O_RDWR, 0);
  size = SzBigFile;
  for (offset = 0; size; offset += BlockSize) {
    unint n = BlockSize;
    if (n > size) n = size;
    read(fd, buf, n);
    IsSame(buf, n, offset);
    size -= n;
  }

  /* Try reading passed eof */
  readCheckSize(fd, buf, sizeof(buf), 0);

  /* Start read before eof but go beyond eof */
  offset = SzBigFile - 47;
  lseek(fd, offset, SEEK_SET);
  readCheckSize(fd, buf, BlockSize, 47);
  IsSame(buf, 47, offset);

  /* Seek to middle of file and verify contents */
  offset = SzBigFile / 2;
  lseek(fd, offset, SEEK_SET);
  read(fd, buf, BlockSize);
  IsSame(buf, BlockSize, offset);

  /* Seek from current position forward 2 blocks */
  offset += 3 * BlockSize;
  lseekCheckOffset(fd, 2 * BlockSize, SEEK_CUR, offset);
  read(fd, buf, BlockSize);
  IsSame(buf, BlockSize, offset);

  /* Seek from eof back 3 blocks */
  offset = SzBigFile - 3 * BlockSize;
  lseekCheckOffset(fd, -(3 * BlockSize), SEEK_END, offset);
  read(fd, buf, BlockSize);
  IsSame(buf, BlockSize, offset);

  /* Seek beyond eof */
  offset = SzBigFile + BlockSize;
  lseek(fd, offset, SEEK_SET);
  readCheckSize(fd, buf, BlockSize, 0);

  /* Seek bad whence */
  lseekErr(EINVAL, fd, 0, 4);

  /* Seek negative offset */
  lseekErr(EINVAL, fd, -3, SEEK_SET);

  close(fd);

  /* Seek on closed fd */
  lseekErr(EBADF, fd, 0, SEEK_SET);

  /* 2nd close is an error */
  closeErr(EBADF, fd);
}

typedef struct segment_s {
  s64 offset;
  s64 length;
} segment_s;

segment_s Segment[] = {
  { 1, 1 },
  { 4095, 45 },
#ifndef __APPLE__
  { 1LL<<20, 1<<13 },
  { 1LL<<40, 1<<10 },
#if 0
  { 1LL<<50, 1<<12 }, /* TODO(taysom): this gives Invalid argument
                       * should create test of this case
                       * and others that I can think of like -1
                       */
#endif
#endif
  { -1, -1 }};

segment_s Hole[] = {
  { 0, 1 },
  { 2197, 3 },
#ifndef __APPLE__
  { 1LL<<25, 1<<14 },
  { 1LL<<35, 1<<16 },
  { 1LL<<47, 1<<16 },
#endif
  { -1, -1 }};

static void write_segment (int fd, segment_s seg) {
  u8 buf[BlockSize];
  s64 offset = seg.offset;
  u64 size;
  unint n = BlockSize;

  lseek(fd, offset, SEEK_SET);
  for (size = seg.length; size; size -= n) {
    if (n > size) n = size;
    Fill(buf, n, offset);
    write(fd, buf, n);
    offset += n;
  }
}

static void check_segment (int fd, segment_s seg) {
  u8 buf[BlockSize];
  s64 offset = seg.offset;
  u64 size;
  unint n = BlockSize;

  lseek(fd, offset, SEEK_SET);
  for (size = seg.length; size; size -= n) {
    if (n > size) n = size;
    read(fd, buf, n);
    IsSame(buf, n, offset);
    offset += n;
  }
}

static void is_zeros (void *buf, u64 n) {
  u8 *b = buf;
  u8 *end = &b[n];

  for (b = buf; b < end; b++) {
    if (*b) {
      PrError("should be zero but is 0x%2x", *b);
    }
  }
}

static void check_hole (int fd, segment_s seg) {
  u8 buf[BlockSize];
  u64 size;
  unint n = BlockSize;

  lseek(fd, seg.offset, SEEK_SET);
  for (size = seg.length; size; size -= n) {
    if (n > size) n = size;
    read(fd, buf, n);
    is_zeros(buf, n);
  }
}

void wtseek (void) {
  char *name;
  int fd;
  s64 i;

  name = RndName(8);
  fd = creat(name, 0666);
  for (i = 0; Segment[i].offset >= 0; i++) {
    write_segment(fd, Segment[i]);
  }
  close(fd);
  fd = open(name, O_RDONLY, 0);
  for (i = 0; Segment[i].offset >= 0; i++) {
    check_segment(fd, Segment[i]);
  }
  for (i = 0; Segment[i].offset >= 0; i++) {
    check_hole(fd, Hole[i]);
  }
  close(fd);
  free(name);
}

void rw_test (void) {
  Simple();
  rdseek();
  wtseek();
}
