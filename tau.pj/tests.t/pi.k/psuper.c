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
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/pagemap.h>
#include <linux/mpage.h>
#include <linux/statfs.h>
#include <linux/version.h>

#include <style.h>
#include <pdebug.h>
#include <pdisk.h>
#include <pi.h>

extern u8 Taysom;

#define PRpage(_x_)	pi_pr_page(__func__, __LINE__, _x_)
#define PRinode(_x_)	pi_pr_inode(__func__, __LINE__, _x_)

struct {
	unint	buf_alloc;	// Should be atomic types
	unint	buf_free;
	unint	inode_alloc;	// Should be atomic types
	unint	inode_free;
} Perf_inst;

void pi_pr_page (const char *fn, unsigned line, struct page *p)
{
	pi_pr(fn, line, "page %p=%lx\n", p, p->flags);
}

void pi_pr_inode (const char *fn, unsigned line, struct inode *inode)
{
	struct list_head	*head, *next;
	struct dentry		*alias;

	if (!inode) {
		pi_pr(fn, line, "<null inode>\n");
		return;
	}
	head = &inode->i_dentry;
	next = inode->i_dentry.next;
	if (next == head) {
		alias = NULL;
	} else {
		alias = list_entry(next, struct dentry, d_alias);
	}
	if (!alias) {
		pi_pr(fn, line, "****"
			"inode=%p ino=%lx count=%x state=%lx <no dentries>\n",
			inode, inode->i_ino, inode->i_count.counter,
			inode->i_state);
	} else {
		pi_pr(fn, line, "****"
			"inode=%p ino=%lx count=%x state=%lx\n"
			"\tdentry=%p flags=%x sb=%p count=%x\n",
			inode, inode->i_ino, inode->i_count.counter,
			inode->i_state,
			alias, alias->d_flags, alias->d_sb,
			alias->d_count.counter);
	}
//	dump_stack();
}

/* Convert file size in bytes to size in pages */
static inline unsigned long dir_pages (struct inode *inode)
{
	return (inode->i_size + PAGE_CACHE_SIZE-1) >> PAGE_CACHE_SHIFT;
}

static inline void zero_page (struct page *page)
{
	// Do I need to map page? Not on 64 bit.
	memset(page_address(page), 0, PAGE_CACHE_SIZE);
}

static struct inode_operations pi_file_inode_operations;
static struct inode_operations pi_dir_inode_operations;

u64 pi_alloc_block (struct super_block *sb, unint num_blocks)
{
	pi_super_info_s	*si = sb->s_fs_info;
	pi_super_block_s	*tsb = si->si_sb;
	u64			pblk;
FN;
	set_page_dirty(si->si_page);
PRx(si->si_page->flags);
	pblk = tsb->sb_nextblock;
	tsb->sb_nextblock += num_blocks;
	return pblk;	
}

void pi_free_block (struct super_block *sb, u64 blkno)
{
FN;
}

/**************************************************************************/
static struct kmem_cache *Perf_buf_cachep;

static inline buf_s *pi_alloc_buf (void)
{
	buf_s	*buf;
FN;
	buf = kmem_cache_alloc(Perf_buf_cachep, GFP_KERNEL);
	if (!buf) return NULL;

	++Perf_inst.buf_alloc;
	return buf;
}

static void pi_destroy_buf (buf_s *buf)
{
FN;
	assert(buf);
	if (buf) kmem_cache_free(Perf_buf_cachep, buf);
	++Perf_inst.buf_free;
}

enum { NUM_PAGES = 1024 };
struct page	*Mypages[NUM_PAGES];
struct page	**Next_mypage = Mypages;

void dump_pages (void)
{
	struct page	**pp;
	struct page	*p;

	for (pp = Mypages; pp < Next_mypage; pp++) {
		p = *pp;
		printk(KERN_INFO "page %p %d %lx\n",
			p, p->_count.counter, p->flags);
	}
}

static void add_page (struct page *page)
{
	if (Next_mypage == &Mypages[NUM_PAGES]) return;

	*Next_mypage++ = page;
}

static void del_page (struct page *page)
{
	struct page	**pp;

	for (pp = Mypages; pp < Next_mypage; pp++) {
		if (*pp == page) {
			*pp = *--Next_mypage;
			return;
		}
	}
}

buf_s *pi_assign_buf (struct page *page)
{
	buf_s	*buf;
FN;
	buf = pi_alloc_buf();
	if (!buf) {
		return NULL;
	}
	add_page(page);
	SetPagePrivate(page);
	page->private = (addr)buf;
	buf->b_page = page;
	return buf;
}

void pi_remove_buf (struct page *page)
{
	buf_s   *buf = (buf_s *)page->private;
FN;
	del_page(page);
	ClearPagePrivate(page);
	page->private = 0;
	assert(buf);
	pi_destroy_buf(buf);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
static void pi_buf_init_once (
	void			*b,
	struct kmem_cache	*cachep,
	unsigned long		flags)
#else
static void pi_buf_init_once (void *b)
#endif
{
	buf_s	*buf = b;
//FN;
	buf->b_blkno = 0;
	buf->b_magic = BUF_MAGIC;
// Fill in rest of fields including agent
}

static int init_pi_buf_cache (void)
{
FN;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
	Perf_buf_cachep = kmem_cache_create("pi_buf_cache",
				sizeof(buf_s),
				0, SLAB_HWCACHE_ALIGN|SLAB_RECLAIM_ACCOUNT,
				pi_buf_init_once, NULL);
#else
	Perf_buf_cachep = kmem_cache_create("pi_buf_cache",
				sizeof(buf_s),
				0, SLAB_HWCACHE_ALIGN|SLAB_RECLAIM_ACCOUNT,
				pi_buf_init_once);
#endif

	if (Perf_buf_cachep == NULL) {
		return -ENOMEM;
	}
	return 0;
}

static void destroy_pi_buf_cache (void)
{
FN;
	if (Perf_inst.buf_alloc != Perf_inst.buf_free) {
		printk(KERN_INFO "destroy_pi_buf_cache: alloc=%ld free=%ld\n",
			Perf_inst.buf_alloc, Perf_inst.buf_free);
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
	if (kmem_cache_destroy(Perf_buf_cachep)) {
		printk(KERN_INFO "destroy_pi_buf_cache: not all structures were freed\n");
	}
#else
	kmem_cache_destroy(Perf_buf_cachep);
#endif
}

/**************************************************************************/
static struct kmem_cache *Perf_inode_cachep;

static struct inode *pi_alloc_inode (struct super_block *sb)
{
	pi_inode_info_s	*tnode;
FN;
	tnode = kmem_cache_alloc(Perf_inode_cachep, GFP_KERNEL);
	if (!tnode) return NULL;

	++Perf_inst.inode_alloc;
	return &tnode->ti_vfs_inode;
}

static void pi_destroy_inode (struct inode *inode)
{
	pi_inode_info_s	*tnode = pi_inode(inode);
FN;
	assert(inode);
	kmem_cache_free(Perf_inode_cachep, tnode);
	++Perf_inst.inode_free;
}

static void pi_inode_init_once (void *t)
{
	pi_inode_info_s	*tnode = t;
FN;
	inode_init_once( &tnode->ti_vfs_inode);
}

static int init_pi_inode_cache (void)
{
FN;
	Perf_inode_cachep = kmem_cache_create("pi_inode_cache",
				sizeof(pi_inode_info_s),
				0, SLAB_HWCACHE_ALIGN|SLAB_RECLAIM_ACCOUNT,
				pi_inode_init_once);
	if (Perf_inode_cachep == NULL) {
		return -ENOMEM;
	}
	return 0;
}

static void destroy_tnode_cache (void)
{
FN;
	if (Perf_inst.inode_alloc != Perf_inst.inode_free) {
		printk(KERN_INFO "destroy_tnode_cache: alloc=%ld free=%ld\n",
			Perf_inst.inode_alloc, Perf_inst.inode_free);
	}
	kmem_cache_destroy(Perf_inode_cachep);
}
/**************************************************************************/

static inline void pi_put_page (struct page *page)
{
FN;
	if (!page) return;

	kunmap(page);
	page_cache_release(page);
}

static int pi_fill_new_page (struct inode *inode, struct page *page)
{
FN;
	kmap(page);	// need a matching unmap
	zero_page(page);
	SetPageUptodate(page);
	//ClearPageLocked(page);
	return 0;
}

static struct page *pi_new_page (struct inode *dir, u64 blkno)
{
	struct address_space	*mapping = dir->i_mapping;
	pi_inode_info_s	*tinfo = pi_inode(dir);
	struct page		*page;
	buf_s			*buf;
	u64			pblk;
	u64			extent;
FN;
	page = read_cache_page(mapping, blkno,
			(filler_t*)pi_fill_new_page, NULL);
	if (IS_ERR(page)) return page;

	assert(!page->private);

	pblk = pi_alloc_block(dir->i_sb, PI_ALU);
	if (!pblk) {
		pi_put_page(page);
		return ERR_PTR(-ENOSPC);
	}
	extent = page->index / PI_ALU;
	tinfo->ti_extent[extent].e_start  = pblk;
	tinfo->ti_extent[extent].e_length = PI_ALU;
	mark_inode_dirty(dir);

	buf = pi_assign_buf(page);
	if (!buf) {
		pi_put_page(page);
		return ERR_PTR(-ENOMEM);
	}
	buf->b_blkno = pblk;

	wait_on_page_locked(page); //TODO:why this instead of page_lock?
					// wait_on_page_locked does NOT
					// get a lock, it just waits for
					// the unlock.  lock_page, waits
					// for and locks the page.
	// Page mapped in pi_fill_new_page
	if (!PageUptodate(page)) goto fail;
	if (!PageChecked(page)) {
		SetPageChecked(page);
	}
	if (PageError(page)) goto fail;
	return page;
fail:
	pi_put_page(page);
	return ERR_PTR(-EIO);
}

static struct page *pi_get_page (struct inode *inode, u64 blkno)
{
	struct address_space	*mapping = inode->i_mapping;
	struct page		*page;
FN;
	page = read_cache_page(mapping, blkno,
			(filler_t*)mapping->a_ops->readpage, NULL);
	if (!IS_ERR(page)) {
		wait_on_page_locked(page);
		kmap(page);
		if (!PageUptodate(page)) goto fail;
		if (!PageChecked(page)) {
			SetPageChecked(page);
		}
		if (PageError(page)) goto fail;
	}
	return page;
fail:
	pi_put_page(page);
	return ERR_PTR(-EIO);
}

static int pi_writepage (struct page *page, struct writeback_control *wbc)
{
FN;
	return pi_flush_page(page);
}

static int pi_super_readpage (struct file *file, struct page *page)
{	// Where do I get <file> from? It is NULL
	// With a simple if test, I could merge this by
	// just seeing if the inode was the superblock inode
	// Will keep separate for now.
FN;
	assert(file == NULL);
	pi_fill_page(page, page->index);
	return 0;	
}

static int pi_readpage (struct file *file, struct page *page)
{
	struct inode		*inode = page->mapping->host;
	pi_inode_info_s	*tinfo = pi_inode(inode);
	u64			pblk;
	u64			extent;
	u64			offset;
FN;
dump_stack();
	extent = page->index / PI_ALU;
	offset = page->index % PI_ALU;
	if (extent >= PI_NUM_EXTENTS) {
		return -EFBIG;
	}
	pblk = tinfo->ti_extent[extent].e_start;
	if (pblk) {
		pi_fill_page(page, pblk + offset);
		// return unlocked
	} else {
		zero_page(page);
	}
	return 0;	
}

#if 0
static int pi_readpages (
	struct file		*file,
	struct address_space	*mapping,
	struct list_head	*pages,
	unsigned		nr_pages)
{
// mpage_readpages calls do_mpage_readpage, which expects get_block which
// usese buffer headers. If we don't supply this interface, we'll have to
// do our own readahead.
// Looks like some readahead is being done but it is different
FN;
	return mpage_readpages(mapping, pages, nr_pages, NULL);
}
#endif
	
static int pi_prepare_write (
	struct file	*file,
	struct page	*page,
	unsigned	from,	// XXX: from/to tells me which bytes
				// are going to be written. I can then
				// determine if we need to read the block
				// in and which bytes to zero.
	unsigned	to)
{
	struct inode	*inode = page->mapping->host;
	pi_inode_info_s	*tinfo = pi_inode(inode);
	u64		extent;
	u64		offset;
	u64		pblk;
	buf_s		*buf;
FN;
	extent = page->index / PI_ALU;
	offset = page->index % PI_ALU;
	if (extent >= PI_NUM_EXTENTS) {
		return -EFBIG;
	}
	kmap(page);
	pblk = tinfo->ti_extent[extent].e_start;
PRx(pblk);	//XXX: truncate, didn't
PRx(from);
PRx(to);
	if (pblk) {
HERE;
		SetPageMappedToDisk(page);
		pi_fill_page(page, pblk+offset);
		lock_page(page);
	} else {
HERE;
		pblk = pi_alloc_block(inode->i_sb, PI_ALU);
		if (!pblk) return -ENOSPC;
		tinfo->ti_extent[extent].e_start  = pblk;
		tinfo->ti_extent[extent].e_length = PI_ALU;
		mark_inode_dirty(inode);
		zero_page(page);

		assert(!page->private);
		buf = pi_assign_buf(page);
		if (!buf) {
			kunmap(page);
			return -ENOMEM;
		}
		buf->b_blkno = pblk+offset;

		SetPageMappedToDisk(page);
		SetPageUptodate(page);

 		set_page_dirty(page);
	}
PRx(page->flags);
	kunmap(page);
	return 0;	
}

static int pi_write_begin (
	struct file		*file,
	struct address_space	*mapping,
	loff_t			pos,
	unsigned		len,
	unsigned		flags,
	struct page		**pagep,
	void			**fsdata)
{
	struct page	*page;
	u64		index;
	unint		from;
	unint		to;

	index = pos >> PAGE_CACHE_SHIFT;
	from = pos & (PAGE_CACHE_SIZE - 1);
	to = from + len;

	page = grab_cache_page_write_begin(mapping, index, flags);
	if (!page) return -ENOMEM;
	*pagep = page;

	return pi_prepare_write(file, page, from, to);
}

static int pi_writepages (
	struct address_space		*mapping,
	struct writeback_control	*wbc)
{
FN;
	return mpage_writepages(mapping, wbc, NULL);
}

static void pi_invalidatepage (struct page *page, unsigned long offset)
{
FN;
	pi_remove_buf(page);
}

static int pi_releasepage (struct page *page, gfp_t gfp_mask)
{
FN;
	pi_remove_buf(page);

	return 1;// return 0 if we don't release it
		// NSS uses a pointer in Buffer_s to
		// determine if it can release it.
}

struct address_space_operations pi_aops = {
	.readpage	= pi_readpage,
//	.readpages	= pi_readpages,
	.writepage	= pi_writepage,
	.sync_page	= block_sync_page,
//	.prepare_write	= pi_prepare_write,
	.write_begin    = pi_write_begin,
	.write_end	= nobh_write_end,
	.invalidatepage = pi_invalidatepage,
	.releasepage    = pi_releasepage,
	.writepages	= pi_writepages,
	.set_page_dirty = __set_page_dirty_nobuffers,
};

struct address_space_operations pi_super_aops = {
	.readpage	= pi_super_readpage,
//	.readpages	= pi_readpages,
	.writepage	= pi_writepage,
	.sync_page	= block_sync_page,
//	.prepare_write	= pi_prepare_write,
	.write_begin    = pi_write_begin,
	.write_end	= nobh_write_end,
	.invalidatepage = pi_invalidatepage,
	.releasepage    = pi_releasepage,
	.writepages	= pi_writepages,
	.set_page_dirty = __set_page_dirty_nobuffers,
};

static int pi_insert (
	struct inode	*dir,
	u64		ino)
{
	struct page	*page;
	u64		blkno;
	unsigned	offset;
	u64		*ilist;
FN;
	blkno  = dir->i_size >> PI_BLK_SHIFT;
	offset = (dir->i_size & PI_OFFSET_MASK) / sizeof(u64);

	if (offset) {
		page = pi_get_page(dir, blkno);//TODO: make sure it can grow
						// file. What if it is a sparse
						// file, do we want to grow it?
	} else {
		page = pi_new_page(dir, blkno);
	}
	if (IS_ERR(page)) {
		return PTR_ERR(page);
	}

	dir->i_size += sizeof(u64);
	ilist = (u64 *)page_address(page);
	ilist[offset] = ino;

	set_page_dirty(page);

	//mark_inode_dirty(dir);

	return 0;
}

static int pi_readdir (struct file *fp, void *dirent, filldir_t filldir)
{
	struct dentry 		*dentry= fp->f_dentry;
	struct inode		*inode = dentry->d_inode;
	struct super_block	*sb = inode->i_sb;
	struct page		*page = NULL;
	unsigned long		npages = dir_pages(inode);
	pi_inode_info_s	*tnode;
	loff_t	pos = fp->f_pos;
	ino_t	ino;
	ino_t	*ilist;
	unint	offset;
	u64	blkno;
	int	done;
FN;
	if (pos == 0) {
		done = filldir(dirent, ".", sizeof(".")-1,
				0,
				fp->f_dentry->d_inode->i_ino,
				DT_DIR);
		if (done) return 0;
		++pos;
	}
	if (pos == 1) {
		done = filldir(dirent, "..", sizeof("..")-1,
				1,
				fp->f_dentry->d_parent->d_inode->i_ino,
				DT_DIR);
		if (done) {
			goto exit;
		}
		++pos;
	}
	blkno  = (pos-2) / PI_INOS_PER_BLK;
	offset = (pos-2) % PI_INOS_PER_BLK;
	for (; blkno < npages; blkno++, offset = 0) {
		page = pi_get_page(inode, blkno);
		if (IS_ERR(page)) goto exit;
//should look for eof
		ilist = (ino_t *)page_address(page);
		for (; offset < PI_INOS_PER_BLK; offset++, pos++) {
			ino = ilist[offset];
			if (!ino) continue;
			inode = iget(sb, ino);
			if (inode == NULL) goto exit;

			tnode = pi_inode(inode);
			done = filldir(dirent, tnode->ti_name,
					strlen(tnode->ti_name),
					ino, pos, inode->i_mode>>12);
			iput(inode);
			if (done) goto exit;
		}
		pi_put_page(page);
		page = NULL;
	}
exit:
	if (page && !IS_ERR(page)) pi_put_page(page);
	fp->f_pos = pos;
	return 0;
}

static int pi_release_file (struct inode *inode, struct file *filp)
{
FN;
PRinode(inode);
	return 0;
}
struct file_operations pi_file_operations = {
	.llseek		= generic_file_llseek,
	.read		= generic_file_read,
	.write		= generic_file_write,
	.aio_read	= generic_file_aio_read,
	.aio_write	= generic_file_aio_write,
	.readdir	= pi_readdir,
//	.ioctl		= pi_ioctl,
	.mmap		= generic_file_mmap,
	.open		= generic_file_open,
	.release	= pi_release_file,
//	.fsync		= pi_sync_file,
	.readv		= generic_file_readv,
	.writev		= generic_file_writev,
	.sendfile	= generic_file_sendfile,
};

static void fill_super_inode (struct super_block *sb, struct inode *inode)
{
	pi_inode_info_s	*tnode;
FN;
	assert(inode->i_ino == PI_SUPER_INO);

	inode->i_blocks  = 0;
	inode->i_mode    = S_IFDIR;
	inode->i_uid     = current->fsuid;
	inode->i_gid     = current->fsgid;
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_size    = 0;
	inode->i_sb      = sb;
	inode->i_rdev    = sb->s_dev;
	inode->i_blkbits = PI_BLK_SHIFT;
	inode->i_blksize = PI_BLK_SIZE;
	inode->i_op      = &pi_dir_inode_operations;
	inode->i_fop     = &pi_file_operations;
	inode->i_mapping->a_ops = &pi_super_aops;
		/*
		 * Handle pi's part of the inode
		 */
	tnode = pi_inode(inode);
	memset(tnode->ti_extent, 0, sizeof(tnode->ti_extent));
	strcpy(tnode->ti_name, "<superblock>");

	insert_inode_hash(inode);
}

static inline int init_pnode (
	pi_inode_info_s		*pnode,
	struct super_block	*sb,
	char		*name,
	u64		ino,
	u64		mode,
	u64		fsuid,
	u64		fsgid,
	u64		size,
	u64		blocks,
	struct timespec	atime,
	struct timespec	mtime,
	struct timespec ctime)
{
	struct inode		*inode = &pnode->ti_vfs_inode;
FN;
	inode->i_ino     = ino;
	inode->i_mode    = mode;
	inode->i_uid     = fsuid;
	inode->i_gid     = fsgid;
	inode->i_atime   = atime;
	inode->i_mtime   = mtime;
	inode->i_ctime   = ctime;
	inode->i_size    = size;
	inode->i_blocks  = blocks;
	inode->i_sb      = sb;
	inode->i_rdev    = sb->s_dev;
	inode->i_blkbits = PI_BLK_SHIFT;
	inode->i_blksize = PI_BLK_SIZE;
	if (S_ISREG(inode->i_mode)) {
		inode->i_op  = &pi_file_inode_operations;
		inode->i_fop = &pi_file_operations;
	} else if (S_ISDIR(inode->i_mode)) {
		inode->i_op  = &pi_dir_inode_operations;
		inode->i_fop = &pi_file_operations;
	} else {
		printk(KERN_ERR "Not ready for this mode %o\n", inode->i_mode);
	}
	if (inode->i_mapping) {
		inode->i_mapping->a_ops = &pi_aops;
	}
	memset(pnode->ti_extent, 0, sizeof(pnode->ti_extent));
	strcpy(pnode->ti_name, name);

	return 0;
}

static struct inode *pi_new_inode (
	struct super_block	*sb,
	struct dentry		*dentry,
	int			mode)
{
	struct inode	*inode;
	pi_inode_info_s	*pnode;
	struct timespec	time;
	ino_t		ino = 0;
FN;
	ino = pi_alloc_block(sb, 1);
	if (ino == 0) return ERR_PTR(-ENOSPC);

	inode = new_inode(sb);
	if (inode == NULL) {
		goto error;
	}
	pnode = pi_inode(inode);
	time = CURRENT_TIME;
	init_pnode(pnode, sb, (char *)dentry->d_name.name,
			ino, mode, current->fsuid,  current->fsgid,
			0, 0, time, time, time);

	insert_inode_hash(inode);
	mark_inode_dirty(inode);
	d_instantiate(dentry, inode);
PRinode(inode);

	return inode;
error:
	if (ino) pi_free_block(sb, ino);
	if (inode) {
		make_bad_inode(inode);
		iput(inode);
	} else {
		return ERR_PTR(-ENOMEM);
	}
	return ERR_PTR(-EINVAL);
}

static int pi_mknod (
	struct inode	*dir,
	struct dentry	*dentry,
	int		mode,
	dev_t		dev)
{
	struct inode	*inode;
FN;
	inode = pi_new_inode(dir->i_sb, dentry, mode);
	if (IS_ERR(inode)) {
		return PTR_ERR(inode);
	}
	if (!inode) {
		return -ENOSPC;
	}

	return pi_insert(dir, inode->i_ino);
}

static int pi_mkdir (
	struct inode	*dir,
	struct dentry	*dentry,
	int		mode)
{
FN;
	return pi_mknod(dir, dentry, mode | S_IFDIR, 0);
}

static int pi_create (
	struct inode		*dir,
	struct dentry		*dentry,
	int			mode,
	struct nameidata	*nd)
{
FN;
	return pi_mknod(dir, dentry, mode | S_IFREG, 0);
}

static struct dentry *pi_lookup (
	struct inode		*dir,
	struct dentry		*dentry,
	struct nameidata	*nd)
{
	struct inode		*inode = NULL;
	struct super_block	*sb = dir->i_sb;
	struct page		*page = NULL;
	pi_inode_info_s	*tnode;
	u64		npages = dir_pages(dir);
	loff_t		pos = 0;
	u64		ino;
	u64		*ilist;
	u64		blkno;
	unsigned	offset;
FN;
	for (blkno = 0, offset = 0; blkno < npages; blkno++) {
		page = pi_get_page(dir, blkno);
		if (IS_ERR(page)) goto exit;

		ilist = (u64 *)page_address(page);
		for (offset = 0; offset < PI_INOS_PER_BLK; offset++) {
			ino = ilist[offset];
			if (!ino) continue;

			inode = iget(sb, ino);
			if (inode == NULL) goto exit;

			tnode = pi_inode(inode);
			if (strcmp(dentry->d_name.name, tnode->ti_name) == 0) {
				pi_put_page(page);
				return d_splice_alias(inode, dentry);
			}
			iput(inode);
			++pos;
		}
		pi_put_page(page);
		page = NULL;
	}
dprintk("d_add");
	d_add(dentry, NULL);
exit:
	if (page && !IS_ERR(page)) pi_put_page(page);
	return NULL;
}

static struct inode_operations pi_dir_inode_operations = {
	.create = pi_create,
	.lookup = pi_lookup,
	.mkdir  = pi_mkdir,
	.mknod  = pi_mknod,
};

static struct inode_operations pi_file_inode_operations = {
	.lookup = pi_lookup,
};


static void pi_read_inode (struct inode *inode)
{
	struct super_block	*sb = inode->i_sb;
	pi_super_info_s		*si = sb->s_fs_info;
	struct page		*page;
	pi_inode_s		*iraw;
	pi_inode_info_s		*pnode = pi_inode(inode);
FN;
	if (inode->i_ino == PI_SUPER_INO) {
		fill_super_inode(sb, inode);
		return;
	}
	page = pi_get_page(si->si_isuper, inode->i_ino);
	if (IS_ERR(page)) {
		printk(KERN_ERR "pi_read_inode err reading inode=%ld %ld\n",
				inode->i_ino, PTR_ERR(page));
		goto error;
	}
	iraw = (pi_inode_s *)page_address(page);
	if ((iraw->t_ino != inode->i_ino)
	 && (iraw->t_hd.h_magic != PI_MAGIC_INODE))
	{
		eprintk("corrupt inode=%ld\n", inode->i_ino);
		goto error;
	}
	init_pnode(pnode, sb, iraw->t_name,
			inode->i_ino, iraw->t_mode,
			iraw->t_uid,  iraw->t_gid,
			iraw->t_size, iraw->t_blocks,
			iraw->t_atime, iraw->t_mtime, iraw->t_ctime);
	memcpy(pnode->ti_extent, iraw->t_extent, sizeof(iraw->t_extent));
	pi_put_page(page);
PRinode(inode);
	return;

error:
	if (!IS_ERR(page)) pi_put_page(page);
	make_bad_inode(inode);
}

static int pi_write_inode (struct inode *inode, int wait)
{
	struct super_block	*sb = inode->i_sb;
	pi_super_info_s	*si = sb->s_fs_info;
	struct page		*page;
	pi_inode_s		*iraw;
	pi_inode_info_s	*tnode = pi_inode(inode);
FN;
PRinode(inode);
	page = pi_get_page(si->si_isuper, inode->i_ino);
	if (IS_ERR(page)) {
		printk(KERN_ERR "pi_write_inode err reading inode=%ld %ld\n",
				inode->i_ino, PTR_ERR(page));
		goto error;
	}
	iraw = (pi_inode_s *)page_address(page);

	iraw->t_hd.h_magic = PI_MAGIC_INODE;
	iraw->t_ino    = inode->i_ino;
	iraw->t_blocks = inode->i_blocks;
	iraw->t_mode   = inode->i_mode;
	iraw->t_uid    = inode->i_uid;
	iraw->t_gid    = inode->i_gid;
	iraw->t_atime  = inode->i_atime;
	iraw->t_mtime  = inode->i_mtime;
	iraw->t_ctime  = inode->i_ctime;
	iraw->t_size   = inode->i_size;
		/*
		 * Handle pi's part of the inode
		 */
	memcpy(iraw->t_extent, tnode->ti_extent, sizeof(iraw->t_extent));
	strcpy(iraw->t_name, tnode->ti_name);

	set_page_dirty(page);
	pi_put_page(page);
	return 0;
error:
	if (!IS_ERR(page)) pi_put_page(page);
	//make_bad_inode(inode);
	return -EIO;
}

static void pi_clear_inode (struct inode *inode)
{
//	pi_inode_info_s	*tnode = pi_inode(inode);
FN;
PRinode(inode);
}

static void pi_put_inode (struct inode *inode)
{
FN;
PRinode(inode);
}

static void free_super_inode (struct inode *isuper)
{
FN;
	truncate_inode_pages( &isuper->i_data, 0);
	iput(isuper);
}

static void pi_put_super (struct super_block *sb)
{
	pi_super_info_s	*si = sb->s_fs_info;
	struct inode		*isuper = si->si_isuper;
FN;
	pi_put_page(si->si_page);
	free_super_inode(isuper);
	sb->s_fs_info = NULL;
	kfree(si);	
}

static int pi_statfs (struct dentry *dentry, struct kstatfs *statfs)
{
	struct super_block *sb = dentry->d_sb;
	pi_super_info_s	*si = sb->s_fs_info;
	pi_super_block_s	*tsb = si->si_sb;
FN;
	statfs->f_type    = sb->s_magic;
	statfs->f_bsize   = sb->s_blocksize;
	statfs->f_blocks  = tsb->sb_numblocks;
	statfs->f_bfree   = tsb->sb_numblocks - tsb->sb_nextblock;
	statfs->f_bavail  = statfs->f_bfree;
	statfs->f_files   = 0;
	statfs->f_ffree   = 0;
	statfs->f_fsid.val[0] = sb->s_dev;	// See comments in "man statfs"
	statfs->f_fsid.val[1] = 0;
	statfs->f_namelen = PI_NAME_LEN;
	statfs->f_frsize  = 0;	// Don't know what this is for.
	return 0;
}

static struct super_operations pi_sops = {
	.alloc_inode	= pi_alloc_inode,
	.destroy_inode	= pi_destroy_inode,
	.read_inode	= pi_read_inode,
	.write_inode	= pi_write_inode,
	.put_inode	= pi_put_inode,
//	.delete_inode	= pi_delete_inode,
	.put_super	= pi_put_super,
//	.write_super	= pi_write_super,
	.statfs		= pi_statfs,
//	.remount_fs	= pi_remount,
	.clear_inode	= pi_clear_inode,
};

int pi_fill_super (
	struct super_block	*sb,
	void			*data,		/* Command line */
	int			isSilent)
{
	struct block_device	*dev    = sb->s_bdev;
	struct inode		*isuper = NULL;
	struct inode		*iroot  = NULL;
	pi_super_info_s	*si     = NULL;
	pi_super_block_s	*tsb;
	struct page		*page;
	unint			sectorSize;
	int			rc = 0;
FN;
	sectorSize = sb_min_blocksize(sb, BLOCK_SIZE);
	if (sectorSize > PI_BLK_SIZE) {
		// Not going to deal with mongo sectors today
		return -EMEDIUMTYPE;
	}
	set_blocksize(dev, PI_BLK_SIZE);

	sb->s_blocksize	     = PI_BLK_SIZE;
	sb->s_blocksize_bits = PI_BLK_SHIFT;
	sb->s_maxbytes	     = PI_MAX_FILE_SIZE;
	sb->s_magic	     = (__u32)PI_MAGIC_SUPER;
	sb->s_op             = &pi_sops;
	sb->s_export_op      = NULL;//&pi_export_ops;// I think this is for NFS

	si = kmalloc(sizeof(pi_super_info_s), GFP_KERNEL);
	if (si == NULL) {
		rc = -ENOMEM;
		goto error;
	}
	sb->s_fs_info = si;
	isuper = iget(sb, PI_SUPER_INO);
	si->si_isuper = isuper;

	page = pi_get_page(isuper, PI_SUPER_BLK);
	if (IS_ERR(page)) {
		rc = PTR_ERR(page);
		printk(KERN_ERR "pi: couldn't read super block %d\n", rc);
		goto error;
	}

	tsb = (pi_super_block_s *)page_address(page);
	if (tsb->sb_hd.h_magic != PI_MAGIC_SUPER) {
		printk(KERN_ERR "pi: bad magic %llu!=%llu\n",
					(unsigned long long)PI_MAGIC_SUPER,
					tsb->sb_hd.h_magic);
		goto error;
	}
	si->si_isuper = isuper;
	si->si_page   = page;
	si->si_sb     = tsb;	// Need to save page - unmap it ?

	if (tsb->sb_blocksize != sb->s_blocksize) {
		printk(KERN_ERR "pi: bad block size %d!=%ld\n",
					tsb->sb_blocksize, sb->s_blocksize);
		goto error;
	}

	iroot = iget(sb, PI_ROOT_INO);
	sb->s_root = d_alloc_root(iroot);
	if (!sb->s_root) {
		printk(KERN_ERR "pi: get root inode failed\n");
		goto error;
	}
	if (!S_ISDIR(iroot->i_mode)) {
		printk(KERN_ERR "pi: corrupt root inode, run pifsck\n");
		goto error;
	}
exit:
	return rc;

error:
	if (rc == 0) rc = -EIO;
	if (sb->s_root) {
		dput(sb->s_root);
		sb->s_root = NULL;
	} else if (iroot) {
		iput(iroot);
	}
	if (isuper) {
		iput(isuper);
	}
	if (si) {
		kfree(si);
		sb->s_fs_info = NULL;
	}
	goto exit;
}

static struct super_block *pi_get_sb (
	struct file_system_type	*fs_type,
	int			flags,
	const char		*dev_name,
	void			*data)
{
FN;
	return get_sb_bdev(fs_type, flags, dev_name, data, pi_fill_super);
}

static void pi_kill_super (struct super_block *sb)
{
FN;
	kill_block_super(sb);
}

static struct file_system_type pi_fs_type = {
	.owner    = THIS_MODULE,
	.name     = "pi",
	.get_sb   = pi_get_sb,
	.kill_sb  = pi_kill_super,
	.fs_flags = FS_REQUIRES_DEV,
};

static void pi_cleanup (void)
{
	if (Perf_inode_cachep) destroy_tnode_cache();
	if (Perf_buf_cachep) destroy_pi_buf_cache();
}

static int pi_init (void)
{
	int	rc;

FN;
	rc = init_pi_buf_cache();
	if (rc != 0) {
		goto error;
	}
	rc = init_pi_inode_cache();
	if (rc != 0) {
		goto error;
	}
	rc = register_filesystem( &pi_fs_type);
	if (rc != 0) {
		goto error;
	}
	return 0;
error:
	printk(KERN_INFO "pi error=%d\n", rc);
	pi_cleanup();
	return rc;	
}

static void pi_exit (void)
{
	unregister_filesystem( &pi_fs_type);
	pi_cleanup();
	printk(KERN_INFO "pi un-loaded\n");
}

MODULE_AUTHOR("Paul Taysom");
MODULE_DESCRIPTION("pi file system");
MODULE_LICENSE("GPL v2");

module_init(pi_init)
module_exit(pi_exit)

