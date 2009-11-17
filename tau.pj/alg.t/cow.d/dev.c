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

#define _XOPEN_SOURCE 500

#include <sys/types.h>
//#define __APPLE__ 1
#ifndef __APPLE__
#include <linux/fs.h>	// BLKGETSIZE64
#endif

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <style.h>
#include <eprintf.h>
#include <debug.h>

#include "media.h"
#include "dev.h"

enum { DEFAULT_NUM_BUFS = 3 };

struct dev_s {
	int	d_fd;		/* Open file descriptor			*/
	char	*d_name;	/* Name of file or device being cached	*/
};

void dev_read (dev_s *dev, u64 blkno, void *data)
{
	int	rc;
FN;
//PRx(blkno);
	rc = pread(dev->d_fd, data, BLK_SIZE, blkno * BLK_SIZE);
	if (rc == -1) {
		fatal("bget pread of %s at %lld failed:", dev->d_name, blkno);
		return;
	}
}

void dev_write (dev_s *dev, u64 blkno, void *data)
{
	int	rc;
FN;
PRx(blkno);
	rc = pwrite(dev->d_fd, data, BLK_SIZE, blkno * BLK_SIZE);
	if (rc == -1) {
		fatal("bwrite pwrite of %s at %lld failed:",
				dev->d_name, blkno);
		return;
	}
}

dev_s *dev_open (const char *name)
{
	dev_s	*dev;
FN;
	dev = emalloc(sizeof(*dev));
	dev->d_fd = open(name, O_CREAT|O_RDWR, 0666);
	if (dev->d_fd == -1) {
		free(dev);
		fatal("bopen couldn't open %s:", name);
		return NULL;
	}
	dev->d_name = strdup(name);
	return dev;
}

void dev_close (dev_s *dev)
{
	close(dev->d_fd);
	free(dev);
	/* Should really go thru all bufs a get rid of ones with dev */
}

u64 dev_size (dev_s *dev)
{
#ifdef __APPLE__
	fprintf(stderr, "This is an Apple\n");
	return 0;
#else
	u64	size;
	int	rc;

	rc = ioctl(dev->d_fd, BLKGETSIZE64, &size);
	if (rc == -1) {
		return 0;
	}
	return size >> BLK_SHIFT;
#endif	
}
