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

#ifndef _AGENT_H_
#define _AGENT_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

#ifndef _Q_H_
#include <q.h>
#endif

enum { AG_IDLE, AG_WAITING, AG_FLUSHING };

typedef struct agent_s	agent_s;

typedef void (*flush_f)(agent_s *agent);

struct agent_s {
	qlink_t	ag_delay_q;	/* link for delay queue			*/
	stk_q	ag_signal;	/* agents to signal after I'm flushed	*/
	d_q	ag_flush_list;	/* agents to flush before I can flush	*/
	u32	ag_wait_for;	/* num signals I'm waiting for		*/
	u32	ag_state;	/* current state			*/
	flush_f	ag_flush;	/* function to flush agent		*/
};

void init_agent    (agent_s *agent, flush_f flush);
void flush_agent   (agent_s *agent);
void delay_agent   (agent_s *agent);
void run_agents    (void);
void signal_agents (stk_q *stk);
void bind          (agent_s *to_signal, agent_s *to_flush);
void check_agent   (agent_s *agent);
void assert_no_bonds (agent_s *agent);


#endif
