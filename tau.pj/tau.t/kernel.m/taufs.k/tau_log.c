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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>

#include <style.h>
#include <tau/debug.h>
#include <tau_blk.h>
#include <tau_fs.h>

// Things being ignored for now
// 1. Double buffering log structure
// 2. Timer to periodically force checkpoint for slow traffic
// 3. Because operations like rename, will require updates to B-trees
//	on other nodes, must break operation into smaller pieces that
//	are independently recoverable.

//XXX: Temporarily made visible
tau_spinlock_s	*Log_lock;

enum { STAGE_LIMIT = TAU_STAGE_SIZE / 2 };

static void init_stage (tau_stage_s *stage, u64 start)
{
	zero(*stage);
	tau_init_lock( &stage->stg_lock, "stage");
	init_MUTEX( &stage->stg_flush_mutex);
	init_MUTEX( &stage->stg_home_mutex);
	stage->stg_blknum = start;
}

static struct workqueue_struct *Home_q;

static int init_home_q (void)
{
	Home_q = create_workqueue("home");
	if (!Home_q) {
		return -ENOMEM;
	}
	return 0;
}

static void destroy_home_q (void)
{
	if (Home_q) {
		flush_workqueue(Home_q);
		destroy_workqueue(Home_q);
	}
}

static void home_stage (void *work)
{
	tau_stage_s	*stage = work;
FN;
	tau_home_stage(stage);
}

static void start_home_stage (tau_stage_s *stage)
{
	//stage->stg_state = STAGE_HOMING;
	INIT_WORK( &stage->stg_work, home_stage, stage);
	queue_work(Home_q, &stage->stg_work);
}

static void wait_for_users (tau_log_s *log)
{
FN;
	if (log->lg_users) {
		down( &log->lg_user_mutex);
	}
}

static inline bool is_full (tau_log_s *log)
{
	tau_stage_s	*stage = log->lg_current;
FN;
	if (stage->stg_numpages >= STAGE_LIMIT) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static void new_stage (tau_log_s *log)
{
	tau_stage_s	*stage;
FN;
	stage = log->lg_current + 1;
	if (stage == &log->lg_stage[TAU_STAGES]) {
		stage = log->lg_stage;
	}
	log->lg_current = stage;
PRd(stage->stg_home_mutex.count.counter);
	down( &stage->stg_home_mutex);
	stage->stg_numpages = 0;
	up( &stage->stg_home_mutex);
}

void tau_logbegin (struct inode *inode)
{
	struct super_block	*sb  = inode->i_sb;
	bag_s		*b   = sb->s_fs_info;
	tau_log_s		*log = &b->bag_log;
	tau_stage_s		*stage;
FN;
	tau_lock( &log->lg_lock, WHERE);
HERE;
	while (is_full(log)) {
HERE;
		if (log->lg_staging) {
HERE;
			tau_unlock( &log->lg_lock, WHERE);
			down( &log->lg_stage_mutex);
			up( &log->lg_stage_mutex);
			tau_lock( &log->lg_lock, WHERE);
HERE;
		} else {
HERE;
			log->lg_staging = TRUE;
			tau_unlock( &log->lg_lock, WHERE);

			down( &log->lg_stage_mutex);
			wait_for_users(log);

			stage = log->lg_current;
			tau_flush_stage(stage);
			start_home_stage(stage);
HERE;

			new_stage(log);
HERE;

			tau_lock( &log->lg_lock, WHERE);

			log->lg_staging = FALSE;

			up( &log->lg_stage_mutex);
HERE;
		}
	}
	++log->lg_users;
	tau_unlock( &log->lg_lock, WHERE);
HERE;
}

void tau_logend (struct inode *inode)
{
	struct super_block	*sb  = inode->i_sb;
	bag_s		*b   = sb->s_fs_info;
	tau_log_s		*log = &b->bag_log;
FN;
	tau_lock( &log->lg_lock, WHERE);
	--log->lg_users;
	if (log->lg_users) {
		tau_unlock( &log->lg_lock, WHERE);
	} else {
		tau_unlock( &log->lg_lock, WHERE);
		up( &log->lg_user_mutex);
	}
}

void tau_loginit (tau_log_s *log)
{
	unint	i;

	zero(*log);
	tau_init_lock( &log->lg_lock, "log");
	Log_lock = &log->lg_lock;

	for (i = 0; i < TAU_STAGES; i++) {
		init_stage( &log->lg_stage[i],
				TAU_START_LOG + i*TAU_STAGE_SIZE);
	}
	init_MUTEX( &log->lg_stage_mutex);
	init_MUTEX( &log->lg_user_mutex);
	log->lg_staging = FALSE;
	log->lg_users   = 0;
	log->lg_current = log->lg_stage;
}

static bool page_is_staged (tau_stage_s *stage, struct page *page)
{
	struct page	**end = &stage->stg_page[stage->stg_numpages];
	struct page	**p;
FN;
	for (p = stage->stg_page; p < end; p++) {
		if (page == *p) {
			return TRUE;
		}
	}
	return FALSE;
}

static void add_page_to_stage (tau_stage_s *stage, struct page *page)
{
	struct page	**end = &stage->stg_page[stage->stg_numpages];
FN;
	assert(stage->stg_numpages < TAU_STAGE_SIZE);
	++stage->stg_numpages;
	*end = page;
}

void tau_log (struct page *page)
{
	bag_s	*super = page->mapping->host->i_sb->s_fs_info;
	tau_stage_s	*stage = super->bag_log.lg_current;
FN;
	// XXX:Could assume page is in log, check if is (no locking), return
	// else get lock, check again, and insert.
	// Convert lookup to open hashing
	tau_lock( &stage->stg_lock, WHERE);
	if (!page_is_staged(stage, page)) {
		add_page_to_stage(stage, page);
	}
	tau_unlock( &stage->stg_lock, WHERE);
}

int tau_start_log (void)
{
	return init_home_q();
}

void tau_stop_log (void)
{
	destroy_home_q();
}
