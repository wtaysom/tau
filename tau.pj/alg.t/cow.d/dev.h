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

#ifndef _DEV_H_
#define _DEV_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

typedef struct dev_s	dev_s;

dev_s *dev_open    (const char *name);
void   dev_close   (dev_s *dev);
void   dev_read    (dev_s *dev, u64 blkno, void *data);
void   dev_write   (dev_s *dev, u64 blkno, void *data);
u64    dev_devsize (dev_s *dev);	/* device size in blocks */

#endif
