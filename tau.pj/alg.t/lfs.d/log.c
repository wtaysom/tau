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

#include <string.h>
#include <stdio.h>

#include <style.h>
#include <eprintf.h>
#include <debug.h>
#include <blk.h>
#include <tauerr.h>
#include <log.h>

enum {	LOG_MAGIC = 0xace1ace1,
	LOG_BLK   = 1,
	START_BLK = 2 };

typedef struct log_block_s {
	__u64	lb_blknum;
	__u32	lb_magic;
	__u32	lb_reserved[1];
	__u64	lb_checkpt;
} __attribute__ ((__packed__)) log_block_s;

int replay_log (log_s *log)
{
	u64		blk;
	buf_s		*buf;
	head_s		*head;
	log_block_s	*lb;
FN;
	buf = bget(log->lg_dev,LOG_BLK);
	lb = buf->b_data;
	blk = lb->lb_checkpt;
	bput(buf);
	for (;; blk++) {
		buf = bget(log->lg_dev, blk);
		if (!buf) return 0;

		head = buf->b_data;
		bmorph(buf, log->lg_sys, head->h_blknum);
	}
}

void take_chkpt (log_s *log)
{
	buf_s		*buf;
	log_block_s	*lb;
FN;
	bsync(log->lg_sys);
	buf = bget(log->lg_dev,LOG_BLK);
	lb = buf->b_data;
	lb->lb_checkpt = log->lg_next;
	bdirty(buf);
	bput(buf);
}

void init_log (log_s *log, char *log_device, char *sys_device)
{
	buf_s		*buf;
	log_block_s	*lb;
FN;
	zero(*log);

	log->lg_dev = bopen(log_device, NULL);
	if (!log->lg_dev) eprintf("Couldn't open %s:", log_device);

	log->lg_sys = bopen(sys_device, log);
	if (!log->lg_sys) eprintf("Couldn't open %s:", sys_device);

	log->lg_next = START_BLK;

	buf = bget(log->lg_dev, LOG_BLK);
	if (!buf) {
		buf = bnew(log->lg_dev, LOG_BLK);
	}
	lb = buf->b_data;
	if (lb->lb_magic != LOG_MAGIC) {
		lb->lb_blknum  = LOG_BLK;
		lb->lb_magic   = LOG_MAGIC;
		lb->lb_checkpt = START_BLK;
		bdirty(buf);
	}
	bput(buf);
}

