/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static int	Fd;

#ifdef __LP64__

#include "mdb_dev.h"

static char	Name[] = FULL_DEV_NAME;

void initkmem (void)
{
	if (Fd) return;

	Fd = open(Name, O_RDONLY);
	if (Fd == -1) {
		perror(Name);
		fprintf(stderr,"You probably need to load kmdb.ko"
				" (insmod kmdb.ko).\n"
				"Don't forget to unload kmdb when"
				" you are finished (rmmod kmdb).\n");
		exit(2);
	}
}

int readkmem (unsigned long address, void *data, int size)
{
	int	n;
	Mdb_s	mdb;

	if (!Fd) initkmem();

	mdb.mdb_cmd  = MDB_READ;
	mdb.mdb_addr = address;
	mdb.mdb_buf  = (unsigned long)data;
	mdb.mdb_size = size;

	n = read(Fd, &mdb, sizeof(mdb));
	if (n == -1) {
		perror("readkmem");
	}
	return n;
}

int writekmem (unsigned long address, void *data, int size)
{
	int	n;
	Mdb_s	mdb;

	if (!Fd) initkmem();

	mdb.mdb_cmd  = MDB_WRITE;
	mdb.mdb_addr = address;
	mdb.mdb_buf  = (unsigned long)data;
	mdb.mdb_size = size;

	n = read(Fd, &mdb, sizeof(mdb));
	if (n == -1) {
		perror("writekmem");
	}
	return n;
}

unsigned long pid2task (unsigned pid)
{
	int	n;
	Mdb_s	mdb;

	if (!Fd) initkmem();

	mdb.mdb_cmd  = MDB_PID2TASK;
	mdb.pid_pid  = pid;

	n = read(Fd, &mdb, sizeof(mdb));
	if (n == -1) {
		perror("pid2task");
		mdb.pid_task = 0;
	}
	return mdb.pid_task;
}

#else

static char	Name[] = "/dev/kmem";
//static char	Name[] = "/dev/mem";

void initkmem (void)
{
	Fd = open(Name, O_RDWR);
	if (Fd == -1) {
		perror(Name);
		exit(errno);
	}
}

int readkmem (unsigned long address, void *data, int size)
{
	long	rc;
	int	n;

	if (!Fd) initkmem();

	if ((rc = lseek(Fd, address, SEEK_SET)) == (off_t)-1) {
		perror("readkmem lseek");
	}
	n = read(Fd, data, size);
	if (n == -1) {
		perror("readkmem");
	}
	return n;
}

int writekmem (unsigned long address, void *data, int size)
{
	int		written;

	if (!Fd) initkmem();

	if (lseek(Fd, address, SEEK_SET) == (off_t)-1) {
		perror("writekmem lseek");
	}
	written = write(Fd, data, size);
	if (written == -1) {
		perror("writekmem");
	}
	return written;
}

unsigned long pid2task (unsigned pid)
{
	fprintf(stderr, "pid2task not implemented for this target\n");
	return 0;
}

#endif
