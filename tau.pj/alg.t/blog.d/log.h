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

#ifndef _LOG_H_
#define _LOG_H_ 1

#ifndef _BLK_H_
#include <blk.h>
#endif

struct log_s {
	dev_s		*lg_dev;
	dev_s		*lg_sys;
	u64		lg_next;
};

void init_log   (log_s *log, char *log_device, char *sys_device);
void take_chkpt (log_s *log);
int  replay_log (log_s *log);

#endif
