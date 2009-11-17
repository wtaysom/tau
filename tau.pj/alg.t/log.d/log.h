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

#ifndef _AGENT_H_
#include <agent.h>
#endif

enum { MAX_SLOTS = 16, NUM_CHKPTS = 2 };

typedef struct logrec_s	logrec_s;

typedef int	(*log_fn)(log_s *log, logrec_s *logrec);

typedef struct log_slot_s {
	u64	sl_num_functions;
	log_fn	*sl_functions;
} log_slot_s;

typedef struct checkpoint_s {
	u64	ck_block;
	log_s	*ck_log;
	agent_s	ck_agent;
} checkpoint_s;

struct log_s {
	dev_s		*lg_dev;
	dev_s		*lg_sys;
	u64		lg_block;
	u64		lg_lsn;
	log_slot_s	lg_slot[MAX_SLOTS];
	unint		lg_chkpt_num;
	checkpoint_s	lg_chkpt[NUM_CHKPTS];
};

struct logrec_s {
	__u64	lr_lsn;
	__u64	lr_user[4];
	__u16	lr_len;
	__u8	lr_slot;
	__u8	lr_fn;
	__u8	lr_data[0];
} __attribute__ ((__packed__));

static inline void check_chkpt (log_s *log)
{
FN;
	check_agent( &log->lg_chkpt[log->lg_chkpt_num].ck_agent);
}

static inline void bind_chkpt (log_s *log, buf_s *buf)
{
	check_agent( &log->lg_chkpt[log->lg_chkpt_num].ck_agent);
	bind( &log->lg_chkpt[log->lg_chkpt_num].ck_agent, &buf->b_agent);
	check_agent( &log->lg_chkpt[log->lg_chkpt_num].ck_agent);
}

void init_log     (log_s *log, char *log_device, char *sys_device);
int  add_slot_log (log_s *log, u8 slot, log_fn *functions);
int  replay_log   (log_s *log);
int  rec_log      (log_s *log, logrec_s *logrec, void *data);

#endif
