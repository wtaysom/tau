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

typedef struct buf_s	buf_s;

enum { BUF_MAGIC = 0xace };
enum { B_INVALID, B_CLEAN, B_DIRTY, B_FLUSHING };

struct buf_s {
	struct page	*b_page;
	u64		b_blknum;
	u64		b_ref;
	u32		b_magic;
	u32		b_state;
};

static kmem_cache_t *Tau_buf_cachep;
static unint	Buf_alloc;
static unint	Buf_free;

void tau_pr_page (const char *fn, unsigned line, struct page *p)
{
	tau_pr(fn, line, "page %p=%lx\n", p, p->flags);
}

static inline void tau_put_page (struct page *page)
{
FN;
	if (!page) return;

	page_cache_release(page);
}

static inline buf_s *tau_alloc_buf (void)
{
	buf_s	*buf;
FN;
	buf = kmem_cache_alloc(Tau_buf_cachep, SLAB_KERNEL);
	if (!buf) return NULL;

	buf->b_blknum = 0;
	++Buf_alloc;
	return buf;
}

static inline void tau_destroy_buf (buf_s *buf)
{
FN;
	assert(buf);
	if (buf) kmem_cache_free(Tau_buf_cachep, buf);
	++Buf_free;
}


static buf_s *tau_assign_buf (struct page *page)
{
	buf_s	*buf;
FN;
	buf = tau_alloc_buf();
	if (!buf) {
		return NULL;
	}
	SetPagePrivate(page);
	page->private = (addr)buf;
	buf->b_page = page;
	buf->b_state = B_INVALID;
	return buf;
}

void tau_remove_buf (struct page *page)
{
	buf_s   *buf = (buf_s *)page->private;
FN;
	ClearPagePrivate(page);
	page->private = 0;
	assert(buf);
	tau_destroy_buf(buf);
}

static void tau_buf_init_once (
	void		*b,
	kmem_cache_t	*cachep,
	unsigned long	flags)
{
	buf_s	*buf = b;

	buf->b_blknum = 0;
	buf->b_magic = BUF_MAGIC;
	buf->b_state = B_CLEAN;
}

int init_tau_buf_cache (void)
{
FN;
	Tau_buf_cachep = kmem_cache_create("tau_buf_cache",
				sizeof(buf_s),
				0, SLAB_HWCACHE_ALIGN|SLAB_RECLAIM_ACCOUNT,
				tau_buf_init_once, NULL);
	if (Tau_buf_cachep == NULL) {
		return -ENOMEM;
	}
	return 0;
}

void destroy_tau_buf_cache (void)
{
FN;
	if (!Tau_buf_cachep) return;
	if (Buf_alloc != Buf_free) {
		eprintk("alloc=%ld free=%ld", Buf_alloc, Buf_free);
	}
	if (kmem_cache_destroy(Tau_buf_cachep)) {
		eprintk("not all structures were freed");
	}
}

static int zero_new_page (struct inode *inode, struct page *page)
{
FN;
	memset(page_address(page), 0, PAGE_CACHE_SIZE);
	SetPageUptodate(page);
	ClearPageLocked(page);
	return 0;
}

void *tau_bdata (buf_s *buf)
{
	struct page	*page = buf->b_page;

	return page_address(page);
}

void tau_bput (void *data)
{
	struct page	*page = virt_to_page(data);
	buf_s		*buf = (buf_s *)page->private;
FN;
	if (buf) {
		assert(buf->b_magic == BUF_MAGIC);
		if (--buf->b_ref) return;
	}
	page_cache_release(page);
}

void tau_bdirty (void *data)
{
	struct page	*page = virt_to_page(data);
	buf_s		*buf = (buf_s *)page->private;
FN;
	assert(buf->b_magic == BUF_MAGIC);
	buf->b_state = B_DIRTY;
}

u64 tau_bnum (void *data)
{
	struct page	*page = virt_to_page(data);
	buf_s		*buf = (buf_s *)page->private;
FN;
	assert(buf->b_magic == BUF_MAGIC);
	return buf->b_blknum;
}

int tau_bflush (void *data)
{
	struct page	*page = virt_to_page(data);
	buf_s		*buf = (buf_s *)page->private;
	int		rc;
FN;
	buf->b_state = B_FLUSHING;
	rc = tau_flush_page(page);
	buf->b_state = B_CLEAN;
	return rc;
}

void tau_blog (void *data)
{
	struct page	*page = virt_to_page(data);
	buf_s		*buf = (buf_s *)page->private;
FN;
	buf->b_state = B_FLUSHING;
	tau_log(page);
}

void *tau_bget (struct address_space *mapping, u64 blknum)
{
	struct page	*page;
	buf_s		*buf;
FN;
	if (!blknum) return NULL;

	page = read_cache_page(mapping, blknum,
			(filler_t*)mapping->a_ops->readpage, NULL);
	if (IS_ERR(page)) {
		eprintk("mapping=%p blknum=%llx err=%ld\n",
				mapping, blknum, PTR_ERR(page));
		return NULL;
	}
	wait_on_page_locked(page);
//XXX: Page needs to marked upto date in both read and write paths
//XXX: Checked is for the file system to run its own private audits
//XXX: of the file block data - for metadata verification.
	if (!PageUptodate(page)) goto fail;
	if (!PageChecked(page)) {
		SetPageChecked(page);
	}
	if (PageError(page)) goto fail;
	buf = (void *)page->private;
	assert(buf);
	assert(buf->b_magic == BUF_MAGIC);

	return page_address(page);
fail:
	tau_put_page(page);
	return NULL;
}

void *tau_bnew (struct address_space *mapping, u64 blknum)
{
	struct page	*page;
	buf_s		*buf;
FN;
	if (!blknum) return NULL;

	page = read_cache_page(mapping, blknum,
			(filler_t*)zero_new_page, NULL);
	if (IS_ERR(page)) {
		eprintk("mapping=%p blknum=%llx err=%ld\n",
				mapping, blknum, PTR_ERR(page));
		return NULL;
	}
	assert(!page->private);

	// Page mapped in zero_new_page
	if (!PageUptodate(page)) goto fail;
	if (!PageChecked(page)) {
		SetPageChecked(page);
	}
	if (PageError(page)) goto fail;

	buf = tau_assign_buf(page);
	if (!buf) goto fail;

	buf->b_blknum = blknum;
	buf->b_state  = B_DIRTY;

	return page_address(page);
fail:
	eprintk("should never happen: mapping=%p blknum=%llx err=%ld\n",
		mapping, blknum, PTR_ERR(page));
	tau_put_page(page);
	return NULL;
}

int tau_fill_page (struct page *page, u64 pblk)
{
	buf_s	*buf;
FN;

	if (page->private) {
		buf = (buf_s *)page->private;
assert(buf->b_magic == BUF_MAGIC);
assert(buf->b_blknum == pblk); // XXX: if private is set, do I need to do this?
	} else {
		buf = tau_assign_buf(page);
		if (!buf) return -ENOMEM;
	}
	buf->b_blknum = pblk; // XXX: if private is set, do I need to do this?

	tau_fill(page, pblk);

	wait_on_page_locked(page);	// Wait for page to fill
	buf->b_state = B_CLEAN;

	return 0;
}

int tau_readpage (struct file *file, struct page *page)
{	// Where do I get <file> from? It is NULL
FN;
	assert(file == NULL);
	tau_fill_page(page, page->index);
	return 0;	
}

int tau_invalidatepage (struct page *page, unsigned long offset)
{
FN;
	tau_remove_buf(page);
	return 1;// return 0 if we don't release it
}
