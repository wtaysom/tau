/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <fcalls.h>
#include <util.h>

/*
 * Basic read/write tests
 */

/* Simple test used to debug testing infrastructure */
static void simple (void)
{
	char	buf[227];
	char	file[] = "file_a";
	int	fd;

	fd = open(file, O_CREAT | O_TRUNC | O_RDWR, 0666);
	fill(buf, sizeof(buf), 0);
	write(fd, buf, sizeof(buf));
	fill(buf, 17, sizeof(buf));
	write(fd, buf, 17);

	lseek(fd, 0, 0);

	read(fd, buf, sizeof(buf));
	is_same(buf, sizeof(buf), 0);
	readSz(fd, buf, sizeof(buf), 17);
	is_same(buf, 17, sizeof(buf));
	close(fd);
}

/* Normal read/lseek tests */
static void rdseek (void)
{
	u8	buf[SZ_BLOCK];
	int	fd;
	s64	offset;
	u64	size;

	/* Read BigFile and verify contents */
	fd = open(BigFile, O_RDWR, 0);
	size = SZ_BIG_FILE;
	for (offset = 0; size; offset += SZ_BLOCK) {
		unint n = SZ_BLOCK;
		if (n > size) n = size;
		read(fd, buf, n);
		is_same(buf, n, offset);
		size -= n;
	}

	/* Try reading passed eof */
	readSz(fd, buf, sizeof(buf), 0);

	/* Start read before eof but go beyond eof */
	offset = SZ_BIG_FILE - 47;
	lseek(fd, offset, SEEK_SET);
	readSz(fd, buf, SZ_BLOCK, 47);
	is_same(buf, 47, offset);
	
	/* Seek to middle of file and verify contents */
	offset = SZ_BIG_FILE / 2;
	lseek(fd, offset, SEEK_SET);
	read(fd, buf, SZ_BLOCK);
	is_same(buf, SZ_BLOCK, offset);

	/* Seek from current position forward 2 blocks */
	offset += 3 * SZ_BLOCK;
	lseekOff(fd, 2 * SZ_BLOCK, SEEK_CUR, offset);
	read(fd, buf, SZ_BLOCK);
	is_same(buf, SZ_BLOCK, offset);

	/* Seek from eof back 3 blocks */
	offset = SZ_BIG_FILE - 3 * SZ_BLOCK;
	lseekOff(fd, -(3 * SZ_BLOCK), SEEK_END, offset);
	read(fd, buf, SZ_BLOCK);
	is_same(buf, SZ_BLOCK, offset);

	/* Seek beyond eof */
	offset = SZ_BIG_FILE + SZ_BLOCK;
	lseek(fd, offset, SEEK_SET);
	readSz(fd, buf, SZ_BLOCK, 0);

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
	s64	offset;
	s64	length;
} segment_s;

segment_s Segment[] = {
	{ 1, 1 },
	{ 4095, 45 },
	{ 1LL<<20, 1<<13 },
	{ 1LL<<40, 1<<10 },
	{ -1, -1 }};

segment_s Hole[] = {
	{ 0, 1 },
	{ 2197, 3 },
	{ 1LL<<25, 1<<14 },
	{ 1LL<<35, 1<<16 },
	{ -1, -1 }};

static void write_segment (int fd, segment_s seg)
{
	u8	buf[SZ_BLOCK];
	s64	offset = seg.offset;
	u64	size;
	unint	n = SZ_BLOCK;

	lseek(fd, offset, SEEK_SET);
	for (size = seg.length; size; size -= n) {
		if (n > size) n = size;
		fill(buf, n, offset);
		write(fd, buf, n);
		offset += n;
	}
}

static void check_segment (int fd, segment_s seg)
{
	u8	buf[SZ_BLOCK];
	s64	offset = seg.offset;
	u64	size;
	unint	n = SZ_BLOCK;

	lseek(fd, offset, SEEK_SET);
	for (size = seg.length; size; size -= n) {
		if (n > size) n = size;
		read(fd, buf, n);
		is_same(buf, n, offset);
		offset += n;
	}
}

static void is_zeros (void *buf, u64 n)
{
	u8	*b = buf;
	u8	*end = &b[n];

	for (b = buf; b < end; b++) {
		if (*b) {
			error("should be zero but is 0x%2x", *b);
		}
	}
}
	
static void check_hole (int fd, segment_s seg)
{
	u8	buf[SZ_BLOCK];
	u64	size;
	unint	n = SZ_BLOCK;

	lseek(fd, seg.offset, SEEK_SET);
	for (size = seg.length; size; size -= n) {
		if (n > size) n = size;
		read(fd, buf, n);
		is_zeros(buf, n);
	}
}

void wtseek (void)
{
	char	*name;
	int	fd;
	s64	i;

	name = rndname(8);
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
}

void rw_test (void)
{
	simple();
	rdseek();
	wtseek();
}
