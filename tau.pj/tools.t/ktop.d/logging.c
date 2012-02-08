/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdio.h>

#include "eprintf.h"
#include "ktop.h"
#include "reduce.h"
#include "util.h"

static FILE	*Fp;

void open_log (void)
{
	if (Log_file) {
		Fp = fopen(Log_file, "w");
		if (!Fp) fatal("Couldn't open log file %s:", Log_file);
	}
}

void close_log (void)
{
	if (Fp) fclose(Fp);
	Fp = NULL;
}

void log_pidcalls (void)
{
	static unint	interval = 0;
	Pidcall_s	*pc;
	int		pid;
	int		i;

	if (!Fp) return;
	++interval;
	for (i = 0; i < Num_rank; i++) {
		pc = Rank_pidcall[i];
		pid = get_pid(pc->pidcall);
		if (!pc->name) {
			pc->name = getpidname(pid);
		}
		fprintf(Fp, "%ld,%d,%d,%lld,%s,%s\n",
			interval, pid, pc->snap.count,
			pc->snap.total_time,
			Syscall[get_call(pc->pidcall)],
			pc->name);
	}
}

