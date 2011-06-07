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

enum {	LOG_MAGIC      = 0x106,
	CHKPT_MAGIC    = 0xcccc,
	CHKPT_INTERVAL = 7,
	CHKPT_BLK      = 1,
	START_BLK      = 2,
	MAX_REC        = BLK_SIZE/2 };

typedef struct log_block_s {
	__u16	lb_magic;
	__u16	lb_next;
	__u8	lb_rec[0];
} __attribute__ ((__packed__)) log_block_s;

typedef struct chkpt_block_s {
	__u16	cb_magic;
	__u16	cb_reserved[3];
	__u64	cb_block;
} __attribute__ ((__packed__)) chkpt_block_s;

static inline snint len_data (snint total)
{
	return total - offsetof(logrec_s, lr_data);
}

static void flush_checkpoint (agent_s *agent)
{
	checkpoint_s	*chkpt = container(agent, checkpoint_s, ck_agent);
	chkpt_block_s	*chkpt_block;
	buf_s		*buf;
FN;
	buf = bget(chkpt->ck_log->lg_dev, CHKPT_BLK);
	chkpt_block = buf->b_data;
	chkpt_block->cb_block = chkpt->ck_block;
	bdirty(buf);
	bput(buf);
}

static void record_checkpoint (log_s *log)
{
	checkpoint_s	*chkpt;
FN;
	if (++log->lg_chkpt_num == NUM_CHKPTS) {
		log->lg_chkpt_num = 0;
	}
	chkpt = &log->lg_chkpt[log->lg_chkpt_num];
	delay_agent( &chkpt->ck_agent);
	chkpt->ck_block = log->lg_block;
}

static buf_s *new_log_block (log_s *log)
{
	buf_s		*lbuf;
	log_block_s	*lb;
FN;
	lbuf = bget(log->lg_dev, log->lg_block);
	lb = lbuf->b_data;
	lb->lb_magic = LOG_MAGIC;
	lb->lb_next  = 0;
	return lbuf;
}

static buf_s *next_log_block (log_s *log)
{
FN;
	++log->lg_block;
	if (log->lg_block % CHKPT_INTERVAL == START_BLK) {
		record_checkpoint(log);
	}
	return new_log_block(log);
}

int rec_log (
	log_s		*log,
	logrec_s	*logrec,
	void		*data)
{
	buf_s		*lbuf;
	log_block_s	*lb;
	logrec_s	*rec;
	snint		free_space;
	snint		total;
FN;
	total = logrec->lr_len + offsetof(logrec_s, lr_data);
	if (total > MAX_REC) {
		eprintf("Log record too big %d\n", logrec->lr_len);
		return -1;
	}
	lbuf = bget(log->lg_dev, log->lg_block);
	lb = lbuf->b_data;

check_agent( &log->lg_chkpt[log->lg_chkpt_num].ck_agent);
	free_space = BLK_SIZE - offsetof(log_block_s, lb_rec) - lb->lb_next;
	if (free_space < total) {
		bput(lbuf);
		lbuf = next_log_block(log);
		lb = lbuf->b_data;
	}
	logrec->lr_lsn = log->lg_lsn++;
	rec = (logrec_s *)&(lb->lb_rec[lb->lb_next]);
	*rec = *logrec;
	memcpy(rec->lr_data, data, logrec->lr_len);

	lb->lb_next += total;

	bdirty(lbuf);
	bput(lbuf);

	return 0;
}

void pr_logrec (logrec_s *r)
{
	unint	i;

	printf("rec # %llx slot %d fn %d len %d:%llx %llx %llx %llx:",
		r->lr_lsn, r->lr_slot, r->lr_fn, r->lr_len,
		r->lr_user[0], r->lr_user[1], r->lr_user[2], r->lr_user[3]);
	if (r->lr_len) {
		for (i = 0; i < r->lr_len; i++) {
			printf(" %.2x", r->lr_data[i]);
		}
	}
	printf("\n");
}

static int replay_record (log_s *log, logrec_s *rec)
{
	u8		slot_num;
	u8		fn_num;
	log_slot_s	*slot;
	log_fn		fn;
FN;
pr_logrec(rec);
	slot_num = rec->lr_slot;
	if (slot_num >= MAX_SLOTS) {
		eprintf("slot_num %d >= %d", slot_num, MAX_SLOTS);
		return qERR_BAD_SLOT;
	}
	slot = &log->lg_slot[slot_num];
	if (!slot->sl_num_functions || !slot->sl_functions) {
		eprintf("functions null for slot %d", slot_num);
		return qERR_BAD_SLOT;
	}
	fn_num = rec->lr_fn;
	if (fn_num >= slot->sl_num_functions) {
		eprintf("fn_num %d >= %d", fn_num, slot->sl_num_functions);
		return qERR_BAD_SLOT;
	}
	fn = slot->sl_functions[fn_num];
	if (!fn) {
		eprintf("null function at %d slot %d", fn_num, slot_num);
		return qERR_BAD_SLOT;
	}
	return fn(log, rec);
}

int replay_log (log_s *log)
{
	u64		blk;
	buf_s		*lbuf;
	buf_s		*ckbuf;
	chkpt_block_s	*chkpt_block;
	log_block_s	*lb;
	logrec_s	*rec;
	int		rc;
	snint		size;
	snint		i;
FN;
	ckbuf = bget(log->lg_dev, CHKPT_BLK);
	chkpt_block = ckbuf->b_data;
	blk = chkpt_block->cb_block;
	bput(ckbuf);
	for (;; blk++) {
		lbuf = bget(log->lg_dev, blk);
		if (!lbuf) {
			return 0;
		}
		lb = lbuf->b_data;
		if (lb->lb_magic != LOG_MAGIC) {
			bput(lbuf);
			return 0;
		}
		for (i = 0; i < lb->lb_next; i += size) {
			rec = (logrec_s *)&lb->lb_rec[i];
			if (!rec->lr_lsn) break;
			size = offsetof(logrec_s, lr_data) + rec->lr_len;
			rc = replay_record(log, rec);
			if (rc) {
				bput(lbuf);
				return rc;
			}
		}
		bput(lbuf);
	}
}

int add_slot_log (log_s *log, u8 slot_num, log_fn *functions)
{
	unint		num_fn;
	log_fn		*fn;
	log_slot_s	*slot;
FN;
	if (slot_num >= MAX_SLOTS) return qERR_BAD_SLOT;
	slot = &log->lg_slot[slot_num];
	if (slot->sl_num_functions || slot->sl_functions) return qERR_BAD_SLOT;

	for (num_fn = 0, fn = functions; *fn; fn++) {
		++num_fn;
	}
	slot->sl_num_functions = num_fn;
	slot->sl_functions = functions;
	return 0;
}

void init_checkpoint (checkpoint_s *chkpt, log_s *log, u64 start)
{
	init_agent( &chkpt->ck_agent, flush_checkpoint);
	chkpt->ck_block = start;
	chkpt->ck_log   = log;
}

void init_log (log_s *log, char *log_device, char *sys_device)
{
	buf_s		*lbuf;
	buf_s		*kbuf;
	chkpt_block_s	*chkpt_block;
FN;
	zero(*log);

	log->lg_dev = bopen(log_device, NULL);
	if (!log->lg_dev) eprintf("Couldn't open %s:", log_device);

	log->lg_sys = bopen(sys_device, log);
	if (!log->lg_sys) eprintf("Couldn't open %s:", sys_device);

	kbuf = bget(log->lg_dev, CHKPT_BLK);
	chkpt_block = kbuf->b_data;
	if (chkpt_block->cb_magic != CHKPT_MAGIC) {
		chkpt_block->cb_magic = CHKPT_MAGIC;
		chkpt_block->cb_block = START_BLK;
		bdirty(kbuf);

		log->lg_block = START_BLK;
		log->lg_lsn   = 1;
		lbuf = new_log_block(log);
		bdirty(lbuf);
		bput(lbuf);
	}
	init_checkpoint( &log->lg_chkpt[0], log, chkpt_block->cb_block);
	init_checkpoint( &log->lg_chkpt[1], log, chkpt_block->cb_block);
	bput(kbuf);
}

