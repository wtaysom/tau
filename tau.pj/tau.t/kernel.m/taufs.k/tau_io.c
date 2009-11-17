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
#include <linux/bio.h>

#include <style.h>
#include <tau/debug.h>
#include <tau_blk.h>
#include <tau_fs.h>

enum {	SECTOR_SHIFT = 9 };

struct {
	u64	start_fill;	//TODO: change to atomic like nss
	u64	end_fill;	// Want to be atomic so I can spot
	u64	start_flush;	// missing I/Os.
	u64	end_flush;
} IO;

static void io_err (struct bio *bio, int err)
{
	if (err == -EOPNOTSUPP) {
		set_bit(BIO_EOPNOTSUPP, &bio->bi_flags);
		printk(KERN_ERR "Error in tau_io EOPNOTSUPP\n");
	}
}

static struct bio *new_bio (struct page *page, u64 blknum)
{
	struct inode	*inode = page->mapping->host;
	struct bio	*bio;
FN;
	bio = bio_alloc(GFP_NOIO, 1);

	assert(blknum);
	bio->bi_sector = blknum << (PAGE_CACHE_SHIFT - SECTOR_SHIFT);
	bio->bi_bdev   = inode->i_sb->s_bdev;
	bio->bi_vcnt   = 1;
	bio->bi_idx    = 0;
	bio->bi_size   = PAGE_CACHE_SIZE;
	bio->bi_io_vec[0].bv_page   = page;
	bio->bi_io_vec[0].bv_len    = PAGE_CACHE_SIZE;
	bio->bi_io_vec[0].bv_offset = 0;


	bio->bi_private = NULL;	// May need to fill in something
				// to track operation.
	return bio;
}

static int tau_end_fill (
	struct bio	*bio,
	unsigned	byte_done,
	int		err)
{
	struct page	*page = bio->bi_io_vec[0].bv_page;
FN;
	++IO.end_fill;	//TODO: change to atomic like nss
	if (bio->bi_size) return 1;
	io_err(bio, err);
	if (bio_flagged(bio, BIO_UPTODATE)) {
		SetPageUptodate(page);
	}
	unlock_page(page);

	bio_put(bio);
	return  0;
}

void tau_fill (struct page *page, u64 blknum)
{
	struct bio	*bio;
FN;
	++IO.start_fill;
	bio = new_bio(page, blknum);
	bio->bi_end_io = tau_end_fill;
//	lock_page(bio->bi_io_vec[0].bv_page);	// Why do I need a lock for fill
						// and not for flush? Because
						// fill is modifying content.
						// I think page is locked coming
						// in here.
	bio_get(bio);
	submit_bio(READ, bio);
	bio_put(bio);
}

static int tau_end_flush (	/* See mpage_end_io_write */
	struct bio	*bio,
	unsigned	byte_done,
	int		err)
{
//	struct page	*page;
FN;
	++IO.end_flush;
	if (bio->bi_size) return 1;
	io_err(bio, err);

//	page = bio->bi_io_vec[0].bv_page;
//PRpage(page);
//	end_page_writeback(page);
	bio_put(bio);
	return 0;
}

int tau_flush_page (struct page *page)
{
	struct bio	*bio;
FN;
//PRpage(page);
	assert(page->private);
//	set_page_writeback(page);
//	unlock_page(page);

	++IO.start_flush;
	bio = new_bio(page, page->index);
	bio->bi_end_io = tau_end_flush;

	bio_get(bio);
	submit_bio(WRITE, bio);
	bio_put(bio);
//XXX: Not waiting for page to flush.

	return  0;
}

static int tau_end_flush_stage (
	struct bio	*bio,
	unsigned	byte_done,
	int		err)
{
	tau_stage_s	*stage = bio->bi_private;
FN;
	++IO.end_flush;
	if (bio->bi_size) return 1;
	io_err(bio, err);

	up( &stage->stg_flush_mutex);

	bio_put(bio);
	return 0;
}

//XXX: eventually, pack multiple writes into a single bio structure
//XXX: it might be nice to do this as part of work-to-do. since this
//XXX: is called at the beginning of a command so nothing is locked,
//XXX: it might be OK where it is.
//XXX: I've been thinking of a "fast" work-to-do queue that gets
//XXX: processed on exit from NSS. Uses exiting thread's context and state.

void tau_flush_stage (tau_stage_s *stage)
{
	struct bio	*bio;
	struct page	**vpage = stage->stg_page;
	unint		n = stage->stg_numpages;
	unint		i;
FN;
	if (!n) return;
	sema_init( &stage->stg_flush_mutex, -(n-1));
	sema_init( &stage->stg_home_mutex, -(n-1));
	for (i = 0; i < n; i++) {
		bio = new_bio(vpage[i], stage->stg_blknum + i);
		bio->bi_end_io  = tau_end_flush_stage;
		bio->bi_private = stage;

		++IO.start_flush;
		bio_get(bio);
		submit_bio(WRITE, bio);
		bio_put(bio);
	}
}

static int tau_end_home_stage (
	struct bio	*bio,
	unsigned	byte_done,
	int		err)
{
	tau_stage_s	*stage = bio->bi_private;
FN;
	++IO.end_flush;
	if (bio->bi_size) return 1;
	io_err(bio, err);

	up( &stage->stg_home_mutex);

	bio_put(bio);
	return 0;
}

void tau_home_stage (tau_stage_s *stage)
{
	struct bio	*bio;
	struct page	*page;
	struct page	**vpage = stage->stg_page;
	unint		n = stage->stg_numpages;
	unint		i;
FN;
	if (!n) return;
	for (i = 0; i < n; i++) {
		page = vpage[i];
		bio = new_bio(page, page->index);
		bio->bi_end_io  = tau_end_home_stage;
		bio->bi_private = stage;

		++IO.start_flush;
		bio_get(bio);
		submit_bio(WRITE, bio);
		bio_put(bio);
	}
}
