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
#include <stdlib.h>

#include <style.h>
#include <debug.h>
#include <eprintf.h>
#include <agent.h>

typedef struct bond_s {
	agent_s		*bn_to_signal;	/* Agent to signal after flush is done*/
	qlink_t		 bn_signal;	/* Bonds to signal after flush	*/
	agent_s		*bn_to_flush;	/* Agent to flush		*/
	dqlink_t	 bn_flush;	/* Other bonds waiting for flush*/
} bond_s;

cir_q	Delay_q;

/*
 * Agent 'to_signal' is dependent on 'to_flush' occuring(written) first.  So
 * a good way to remember the relationships recorded by this function is
 * that AGENT
 *      'to_flush' will occur 1st.
 *      'to_signal' will occur 2nd.
 *
 * Agent 'to_signal' gets placed on the signal list of agent 'to+flush'.  I.E.
 * when 'to_flush' occurs then 'to_signal' Agent gets signaled.
 *
 * Agent 'to_flush' gets placed on the flush list of the agent 'to_signal'.  I.E.
 * if agent 'to_signal' needs to be flushed, "to_signal's" flush list is used
 * to find and then tell 'to_flush' that it should flush.
 */
void bind (agent_s *to_signal, agent_s *to_flush) /* dependent, independent */
{
	bond_s	*bond;
 
	bond = ezalloc(sizeof(*bond));
	bond->bn_to_signal   = to_signal;
	bond->bn_to_flush = to_flush;
	push_stk( &to_flush->ag_signal, bond, bn_signal);
	enq_dq( &to_signal->ag_flush_list, bond, bn_flush);
}

static int is_flush_bond (void *obj, void *arg)
{
	bond_s	*bond = container(obj, bond_s, bn_flush);

	return bond == arg;
}

static int is_sig_bond (void *obj, void *arg)
{
	bond_s	*bond = container(obj, bond_s, bn_signal);

	return bond == arg;
}

void check_bond (bond_s *bond)
{
	agent_s	*to_signal;
	agent_s	*to_flush;
	int	rc;

	if (is_qmember(bond->bn_flush)) {
		to_signal = bond->bn_to_signal;
		rc = foreach_dq( &to_signal->ag_flush_list, is_flush_bond, bond);
		if (!rc) {
			assert(!"Bond not on flush list");
		}
	}
	to_flush = bond->bn_to_flush;
	rc = foreach_stk( &to_flush->ag_signal, is_sig_bond, bond);
	if (!rc) {
		assert(!"Bond not on signal list");
	}
}

static int is_sig_agent (void *obj, void *arg)
{
	bond_s	*bond = container(obj, bond_s, bn_flush);

	check_bond(bond);

	return bond->bn_to_signal != arg;
}

void check_agent (agent_s *agent)
{
	int	rc;

	rc = foreach_dq( &agent->ag_flush_list, is_sig_agent, agent);
	if (rc) {
		assert(!"Bond doesn't point to agent");
	}
}

void assert_no_bonds (agent_s *agent)
{
	assert(is_empty_dq( &agent->ag_flush_list));
	assert(is_empty_stk( &agent->ag_signal));
}
			
void signal_agents (stk_q *stk)
{
	bond_s	*bond;
	agent_s	*agent;
FN;
	for (;;) {
		pop_stk(stk, bond, bn_signal);
		if (!bond) break;
		agent = bond->bn_to_signal;
PRd(agent->ag_wait_for);
		if (is_qmember(bond->bn_flush)) {
			rmv_dq( &bond->bn_flush);
		} else {
			--agent->ag_wait_for;
		}
		if (!agent->ag_wait_for) {
// Not needed yet			agent->ag_flush(agent);
		}
		free(bond);
	}
}

/*
 * Unlike the NSS version of default_flush, this agent flushing code does not
 * lock the agent or play with its state, we assume the specific code for each
 * agent type handles this for themselves.
 *
 * XXX
 * This version is recursive which is not be a good thing.
 * Need to think about this - could setup a queue for things to be flushed
 * including all children. But they may be on lists already - interesting
 * locking problem.
 */
void flush_agent (agent_s *agent)
{
	bond_s	*bond;
FN;
check_agent(agent);
	for (;;) {
		deq_dq( &agent->ag_flush_list, bond, bn_flush);
		if (!bond) break;
		check_bond(bond);
		++agent->ag_wait_for;
		flush_agent(bond->bn_to_flush);
	}
	assert(agent->ag_wait_for == 0);//XXX: no asynchronous flush yet
	agent->ag_flush(agent);
	signal_agents( &agent->ag_signal);		
}

void delay_agent (agent_s *agent)
{
FN;
	check_agent(agent);
	enq_cir( &Delay_q, agent, ag_delay_q);
}

void run_agents (void)
{
	agent_s	*agent;

	for (;;) {
		deq_cir( &Delay_q, agent, ag_delay_q);
		if (!agent) break;
FN;
		flush_agent(agent);
	}
}

void init_agent (agent_s *agent, flush_f flush)
{
	init_stk( &agent->ag_signal);
	init_dq( &agent->ag_flush_list);
	agent->ag_wait_for = 0;
	agent->ag_state = AG_IDLE;
	agent->ag_flush = flush;
}
