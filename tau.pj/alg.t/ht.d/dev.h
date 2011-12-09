/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _DEV_H_
#define _DEV_H_ 1

#include "buf.h"
#include "crfs.h"

struct Dev_s {
	int	fd;
	char	*name;
};

Dev_s *dev_create(char *name);
Dev_s *dev_open(char *name);
void dev_flush(Buf_s *b);
void dev_fill(Buf_s *b);

#endif
