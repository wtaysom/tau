/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eprintf.h"
#include "ktop.h"
#include "reduce.h"
#include "util.h"

static FILE	*Fp;
static char	*File;

void open_log (const char *file)
{
	if (Fp) {
		warn("Log file already inuse %s", file);
	} else {
		Fp = fopen(file, "w");
		if (!Fp) fatal("Couldn't open log file %s:", file);
		File = strdup(file);
	}
}

void close_log (void)
{
	if (Fp) {
		fclose(Fp);
		free(File);
		Fp = NULL;
		File = NULL;
	}
}

void log_pidcalls (void)
{
	static unint	interval = 0;
	Pidcall_s	*pc;
	int		pid;
	int		i;
	int		rc;

	if (!Fp) return;
	++interval;
	for (i = 0; i < Num_rank; i++) {
		pc = Rank_pidcall[i];
		pid = get_pid(pc->pidcall);
		if (!pc->name) {
			pc->name = get_exe_path(pid);
		}
		rc = fprintf(Fp, "%ld,%d,%d,%lld,%s,%s\n",
			interval, pid, pc->snap.count,
			pc->snap.total_time,
			Syscall[get_call(pc->pidcall)],
			pc->name);
		if (rc < 0) {
			warn("Log failed %s:", File);
			close_log();
		}
	}
}

