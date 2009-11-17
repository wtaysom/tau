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
#include <pdebug.h>
#include <pi.h>

enum {	SECTOR_SHIFT = 9 };

struct {
	u64	start_fill;//TODO: change to atomic like nss
	u64	end_fill;
	u64	start_flush;
	u64	end_flush;
} IO;

static struct bio *new_bio (buf_s *buf)
{
	struct page	*page  = buf->b_page;
	struct inode	*inode = page->mapping->host;
	struct bio	*bio;
FN;
	bio = bio_alloc(GFP_NOIO, 1);

	bio->bi_sector = buf->b_blkno << (PAGE_CACHE_SHIFT - SECTOR_SHIFT);
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

static void io_err (struct bio *bio, int err)
{
	if (err == -EOPNOTSUPP) {
		set_bit(BIO_EOPNOTSUPP, &bio->bi_flags);
		printk(KERN_ERR "Error in pi_end_fill EOPNOTSUPP\n");
	}
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))

static int pi_end_fill (
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
	return 0;
}

#else

static void pi_end_fill (
	struct bio	*bio,
	int		err)
{
	struct page	*page = bio->bi_io_vec[0].bv_page;
FN;
	++IO.end_fill;	//TODO: change to atomic like nss
	if (bio->bi_size) return;
	io_err(bio, err);
	if (bio_flagged(bio, BIO_UPTODATE)) {
		SetPageUptodate(page);
	}
	unlock_page(page);

	bio_put(bio);
}

#endif

void pi_fill (buf_s *buf)
{
	struct bio	*bio;
FN;
	++IO.start_fill;
	bio = new_bio(buf);
	bio->bi_end_io = pi_end_fill;
//	lock_page(bio->bi_io_vec[0].bv_page);	// Why do I need a lock for fill
						// and not for flush? Because
						// fill is modifying content.
						// I think page is locked coming
						// in here.
	bio_get(bio);
	submit_bio(READ, bio);
	bio_put(bio);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))

static int pi_end_flush (	/* See mpage_end_io_write */
	struct bio	*bio,
	unsigned	byte_done,
	int		err)
{
	struct page	*page;
FN;
	++IO.end_flush;
	if (bio->bi_size) return 1;
	io_err(bio, err);

	page = bio->bi_io_vec[0].bv_page;
	end_page_writeback(page);
	bio_put(bio);
	return 0;
}

#else

static void pi_end_flush (	/* See mpage_end_io_write */
	struct bio	*bio,
	int		err)
{
	struct page	*page;
FN;
	++IO.end_flush;
	if (bio->bi_size) return;
	io_err(bio, err);

	page = bio->bi_io_vec[0].bv_page;
	end_page_writeback(page);
	bio_put(bio);
}

#endif

void pi_flush (buf_s *buf)
{
	struct bio	*bio;
FN;
	++IO.start_flush;
	bio = new_bio(buf);
	bio->bi_end_io = pi_end_flush;

	bio_get(bio);
	submit_bio(WRITE, bio);
	bio_put(bio);
}

int pi_fill_page (struct page *page, u64 pblk)
{
	buf_s	*buf;
FN;
	if (page->private) {
		buf = (buf_s *)page->private;
		assert(buf->b_magic == BUF_MAGIC);
	} else {
		buf = pi_assign_buf(page);
		if (!buf) return -ENOMEM;
	}
	buf->b_blkno = pblk; // XXX: if private is set, do I need to do this?

	pi_fill(buf);

	wait_on_page_locked(page);	// Wait for page to fill

	return 0;
}

int pi_flush_page (struct page *page)
{
	buf_s	*buf;
FN;
	assert(page->private);
	set_page_writeback(page);
	unlock_page(page);

	buf = (buf_s *)(page->private);

	pi_flush(buf);

	return 0;
}
