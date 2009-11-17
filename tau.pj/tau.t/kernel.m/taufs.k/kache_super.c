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
#include <linux/ctype.h>

#include <style.h>
#include <tau/msg.h>
#include <tau/switchboard.h>
#include <tau/tag.h>
#include <tau/fsmsg.h>

#define tMASK	tKACHE
#include <tau/debug.h>

#include <tau/disk.h>
#include <util.h>
#include <tau_fs.h>
#include <kache.h>
#include <tau_err.h>

struct {	// These shouldn't be atomic types because it is not worth it
	unint	buf_alloc;
	unint	buf_free;
	unint	inode_alloc;
	unint	inode_free;
} Kache_inst;

static void close_knode (void *msg);

static struct {
	type_s		kt_tag;
	method_f	kt_ops[0];
} Knode_type = { { 0, close_knode } };

void kache_pr_inode (const char *fn, unsigned line, struct inode *inode)
{
	struct list_head	*head, *next;
	struct dentry		*alias;

	if (!inode) {
		tau_pr(fn, line, "<null inode>\n");
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
		tau_pr(fn, line, "****"
			"inode=%p ino=%lx count=%x state=%lx <no dentries>\n",
			inode, inode->i_ino, inode->i_count.counter,
			inode->i_state);
	} else {
		//extern struct dentry	*Taysom;
		//Taysom = alias;
		tau_pr(fn, line, "****"
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

static struct inode_operations kache_file_inode_operations;
static struct inode_operations kache_dir_inode_operations;

/**************************************************************************/

//XXX: I think I used Mypages just for debugging
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

//XXX:static void add_page (struct page *page)
//XXX:{
//XXX:	if (Next_mypage == &Mypages[NUM_PAGES]) return;
//XXX:
//XXX:	*Next_mypage++ = page;
//XXX:}
//XXX:
//XXX:static void del_page (struct page *page)
//XXX:{
//XXX:	struct page	**pp;
//XXX:
//XXX:	for (pp = Mypages; pp < Next_mypage; pp++) {
//XXX:		if (*pp == page) {
//XXX:			*pp = *--Next_mypage;
//XXX:			return;
//XXX:		}
//XXX:	}
//XXX:}

#if 0
//XXX:buf_s *kache_assign_buf (struct page *page)
//XXX:{
//XXX:	buf_s	*buf;
//XXX:FN;
//XXX:	buf = kache_alloc_buf();
//XXX:	if (!buf) {
//XXX:		return NUL;
//XXX:	}
//XXX:	add_page(page);
//XXX:	SetPagePrivate(page);
//XXX:	page->private = (addr)buf;
//XXX:	buf->b_page = page;
//XXX:	return buf;
//XXX:}
//XXX:
//XXX:void kache_remove_buf (struct page *page)
//XXX:{
//XXX:	buf_s   *buf = (buf_s *)page->private;
//XXX:FN;
//XXX:	del_page(page);
//XXX:	ClearPagePrivate(page);
//XXX:	page->private = 0;
//XXX:	assert(buf);
//XXX:	kache_destroy_buf(buf);
//XXX:}
#endif

#if 0
//XXX:static void kache_buf_init_once (
//XXX:	void		*b,
//XXX:	kmem_cache_t	*cachep,
//XXX:	unsigned long	flags)
//XXX:{
//XXX:	buf_s	*buf = b;
//XXX://ENTER;
//XXX:	buf->b_blknum = 0;
//XXX:	buf->b_magic = BUF_MAGIC;
//XXX:// Fill in rest of fields including agent
//XXX:}
//XXX:
//XXX:static int init_kache_buf_cache (void)
//XXX:{
//XXX:ENTER;
//XXX:	Kache_buf_cachep = kmem_cache_create("kache_buf_cache",
//XXX:				sizeof(buf_s),
//XXX:				0, SLAB_HWCACHE_ALIGN|SLAB_RECLAIM_ACCOUNT,
//XXX:				kache_buf_init_once, NULL);
//XXX:	if (Kache_buf_cachep == NULL) {
//XXX:		return -ENOMEM;
//XXX:	}
//XXX:	return 0;
//XXX:}
//XXX:
//XXX:static void destroy_kache_buf_cache (void)
//XXX:{
//XXX:ENTER;
//XXX:	if (Kache_inst.buf_alloc != Kache_inst.buf_free) {
//XXX:		printk(KERN_INFO "destroy_kache_buf_cache: alloc=%ld free=%ld\n",
//XXX:			Kache_inst.buf_alloc, Kache_inst.buf_free);
//XXX:	}
//XXX:	if (kmem_cache_destroy(Kache_buf_cachep)) {
//XXX:		printk(KERN_INFO "destroy_kache_buf_cache: not all structures were freed\n");
//XXX:	}
//XXX:}
#endif

/**************************************************************************/
static kmem_cache_t *Knode_cachep;

static struct inode *kache_alloc_inode (struct super_block *sb)
{
	knode_s	*knode;
FN;
	knode = kmem_cache_alloc(Knode_cachep, SLAB_KERNEL);
	if (!knode) return NULL;

	++Kache_inst.inode_alloc;
	knode->ki_state = KN_NORMAL;
	return &knode->ki_vfs_inode;
}

static void close_knode (void *msg)
{
	msg_s	*m = msg;
	knode_s	*knode = m->q.q_tag;
FN;
	switch (knode->ki_state) {
	case KN_NORMAL:
		destroy_key_tau(knode->ki_key);
		knode->ki_state = KN_CLOSED;
		break;
	case KN_DIEING:
		kmem_cache_free(Knode_cachep, knode);
		++Kache_inst.inode_free;
		break;
	case KN_CLOSED:
	default:
		eprintk("knode bad state = %lld", knode->ki_state);
		break;
	}
	//XXX: need to tell VFS to invalidate
}

static void kache_destroy_inode (struct inode *inode)
{
	knode_s	*knode = get_knode(inode);
FN;
PRx(inode->i_ino);
	assert(inode);

	enter_tau(knode->ki_replica->rp_avatar);
	switch (knode->ki_state) {
	case KN_NORMAL:
		destroy_gate_tau(knode->ki_keyid);
		destroy_key_tau(knode->ki_key);
		knode->ki_state = KN_DIEING;
		break;
	case KN_CLOSED:
		kmem_cache_free(Knode_cachep, knode);
LABEL(Inode_freed);
		++Kache_inst.inode_free;
		break;
	case KN_DIEING:
	default:
		eprintk("knode bad state = %lld", knode->ki_state);
		break;
	}
HERE;
	exit_tau();
}

static void knode_init_once (
	void		*t,
	kmem_cache_t	*cachep,
	unsigned long	flags)
{
	knode_s	*knode = t;
FN;
	if ((flags & (SLAB_CTOR_VERIFY|SLAB_CTOR_CONSTRUCTOR))
			== SLAB_CTOR_CONSTRUCTOR) {
		inode_init_once( &knode->ki_vfs_inode);
	}
	knode->ki_type = &Knode_type.kt_tag;
}

static int init_knode_cache (void)
{
FN;
	Knode_cachep = kmem_cache_create("knode_cache",
				sizeof(knode_s),
				0, SLAB_HWCACHE_ALIGN|SLAB_RECLAIM_ACCOUNT,
				knode_init_once, NULL);
	if (Knode_cachep == NULL) {
		return -ENOMEM;
	}
	return 0;
}

static void destroy_knode_cache (void)
{
FN;
	if (Kache_inst.inode_alloc != Kache_inst.inode_free) {
		printk(KERN_INFO "destroy_tnode_cache: alloc=%ld free=%ld\n",
			Kache_inst.inode_alloc, Kache_inst.inode_free);
	}
	if (kmem_cache_destroy(Knode_cachep)) {
		printk(KERN_INFO "destroy_tnode_cache: not all structures were freed\n");
	}
}
/**************************************************************************/

void kache_put_page (struct page *page)
{
FN;
	if (!page) return;

	kunmap(page);
	page_cache_release(page);
}

int kache_zero_new_page (struct inode *inode, struct page *page)
{
FN;
	kmap(page);	// need a matching unmap
	zero_page(page);
	SetPageUptodate(page);
	ClearPageLocked(page);
	return 0;
}

struct page *kache_new_page (struct inode *dir, u64 blkno)
{
	struct address_space	*mapping = dir->i_mapping;
//XXX:	knode_s	*knode = get_knode(dir);
	struct page		*page;
//XXX:	buf_s			*buf;
//XXX:	u64			pblk;
	u64			extent;
FN;
	page = read_cache_page(mapping, blkno,
			(filler_t*)kache_zero_new_page, NULL);
	if (IS_ERR(page)) return page;

	assert(!page->private);

//XXX:	pblk = kache_alloc_block(dir->i_sb, TAU_ALU);
//XXX:	if (!pblk) {
//XXX:		kache_put_page(page);
//XXX:		return ERR_PTR(-ENOSPC);
//XXX:	}
	extent = page->index / TAU_ALU;
//XXX:	knode->ti_extent[extent].e_start  = pblk;
//XXX:	knode->ti_extent[extent].e_length = TAU_ALU;
	mark_inode_dirty(dir);

//XXX:	buf = kache_assign_buf(page);
//XXX:	if (!buf) {
//XXX:		kache_put_page(page);
//XXX:		return ERR_PTR(-ENOMEM);
//XXX:	}
//XXX:	buf->b_blknum = pblk;

	wait_on_page_locked(page); //TODO:why this instead of page_lock?
					// wait_on_page_locked does NOT
					// get a lock, it just waits for
					// the unlock.  lock_page, waits
					// for and locks the page.
	// Page mapped in kache_zero_new_page
	if (!PageUptodate(page)) goto fail;
	if (!PageChecked(page)) {
		SetPageChecked(page);
	}
	if (PageError(page)) goto fail;
	return page;
fail:
	kache_put_page(page);
	return ERR_PTR(-EIO);
}

struct page *kache_get_page (struct inode *inode, u64 blkno)
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
	kache_put_page(page);
	return ERR_PTR(-EIO);
}

static int kache_writepage (struct page *page, struct writeback_control *wbc)
{
	struct inode	*inode = page->mapping->host;
	knode_s		*knode = get_knode(inode);
	fsmsg_s		m;
	int		rc;
FN;
	enter_tau(knode->ki_replica->rp_avatar);
	kmap(page);
	m.m_method = FLUSH_PAGE;
	m.io_blkno = page->index;
	rc = putdata_tau(knode->ki_key, &m, PAGE_CACHE_SIZE,
			page_address(page));
	if (rc) {
		eprintk("putdata_tau %d", rc);
		kunmap(page);
		exit_tau();
		return -ENOMEM;
	}
	set_page_writeback(page);
	unlock_page(page);
	kunmap(page);
	exit_tau();
	return 0;	
}

static int kache_readpage (struct file *file, struct page *page)
{
//XXX:	struct inode		*inode = page->mapping->host;
//XXX:	knode_s	*knode = get_knode(inode);
	u64			pblk = 0;
	u64			extent;
	u64			offset;
FN;
	extent = page->index / TAU_ALU;
	offset = page->index % TAU_ALU;
	if (extent >= TAU_NUM_EXTENTS) {
		return -EFBIG;
	}
//XXX:	pblk = knode->ti_extent[extent].e_start;
	if (pblk) {
//XXX:		kache_fill_page(page, pblk + offset);
		// return unlocked
	} else {
		zero_page(page);
	}
	return 0;	
}

#if 0
static int kache_readpages (
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

//XXX: in version 2.6.25, prepare_write and commit_write are replaced
//XXX: by write_begin and write_end.  See //lwn.net/Articles/254856	
static int kache_prepare_write (
	struct file	*file,
	struct page	*page,
	unsigned	from,	// XXX: from/to tells me which bytes
				// are going to be written. I can then
				// determine if we need to read the block
				// in and which bytes to zero.
	unsigned	to)
{
	struct inode	*inode = page->mapping->host;
	knode_s		*knode = get_knode(inode);
	fsmsg_s		m;
	int		rc;
FN;
	if (PageMappedToDisk(page)) return 0;

	enter_tau(knode->ki_replica->rp_avatar);
	kmap(page);
	m.m_method = PREPARE_WRITE;
	m.io_blkno = page->index;
	rc = getdata_tau(knode->ki_key, &m, PAGE_CACHE_SIZE,
			page_address(page));
	if (rc) {
		eprintk("getdata_tau %d", rc);
		kunmap(page);
		exit_tau();
		return -ENOMEM;
	}
	if (m.m_response == ZERO_FILL) {
		zero_page(page);
		mark_inode_dirty(inode);
		SetPageUptodate(page);
		set_page_dirty(page);
	}
	kunmap(page);
	exit_tau();
	return 0;	
}

static int kache_writepages (
	struct address_space		*mapping,
	struct writeback_control	*wbc)
{
FN;
	return mpage_writepages(mapping, wbc, NULL);
}

static int kache_invalidatepage (struct page *page, unsigned long offset)
{
FN;
//XXX:	kache_remove_buf(page);
	return 1;// return 0 if we don't release it
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,6))
	static int kache_releasepage (struct page *page, int gfp_mask)
#else
	static int kache_releasepage (struct page *page, gfp_t gfp_mask)
#endif
{
FN;
//XXX:	kache_remove_buf(page);

	return 1;// return 0 if we don't release it
		// NSS uses a pointer in Buffer_s to
		// determine if it can release it.
}

struct address_space_operations kache_aops = {
	.readpage	= kache_readpage,
//	.readpages	= kache_readpages,
	.writepage	= kache_writepage,
	.sync_page	= block_sync_page,
	.prepare_write	= kache_prepare_write,
	.commit_write	= nobh_commit_write,
	.invalidatepage = kache_invalidatepage,
	.releasepage    = kache_releasepage,
	.writepages	= kache_writepages,
	.set_page_dirty = __set_page_dirty_nobuffers,
};

static int kache_readdir (struct file *fp, void *dirent, filldir_t filldir)
{
	struct dentry 	*dentry = fp->f_dentry;
	struct inode	*inode  = dentry->d_inode;
	knode_s	*knode  = get_knode(inode);
	loff_t		pos     = fp->f_pos;
	int		done;
FN;
	pos += TAU_DOT;
	if (pos == TAU_DOT) {
		done = filldir(dirent, ".", sizeof(".")-1,
				0,
				fp->f_dentry->d_inode->i_ino,
				DT_DIR);
		if (done) goto exit;
		++pos;
	}
	if (pos == TAU_DOTDOT) {
		done = filldir(dirent, "..", sizeof("..")-1,
				1,
				fp->f_dentry->d_parent->d_inode->i_ino,
				DT_DIR);
		if (done) goto exit;
		++pos;
	}
	pos = do_readdir(knode->ki_key, dirent, filldir, pos);
exit:
	fp->f_pos = pos - TAU_DOT;
	return 0;
}

static int kache_release_file (struct inode *inode, struct file *filp)
{
FN;
PRinode(inode);
	return 0;
}

static int kache_sync_file (
	struct file     *file,
	struct dentry   *dentry,
	int             datasync)
{
FN;
	return 0;
}

struct file_operations kache_file_operations = {
	.llseek		= generic_file_llseek,
	.read		= generic_file_read,
	.write		= generic_file_write,
	.aio_read	= generic_file_aio_read,
	.aio_write	= generic_file_aio_write,
	.readdir	= kache_readdir,
//	.ioctl		= kache_ioctl,
	.mmap		= generic_file_mmap,
	.open		= generic_file_open,
	.release	= kache_release_file,
	.fsync		= kache_sync_file,
	.readv		= generic_file_readv,
	.writev		= generic_file_writev,
	.sendfile	= generic_file_sendfile,
};

static inline struct timespec itime (u64 secs)
{
	return ((struct timespec) { secs, 0 });
}

static int register_knode (knode_s *knode)
{
	msg_s	m;
	int	rc;
FN;
	m.q.q_tag  = knode;
	m.q.q_type = RESOURCE;
	rc = create_gate_tau( &m);
	if (rc) {
		eprintk("create_gate_tau %d", rc);
		return rc;
	}
	knode->ki_keyid = m.cr_id;
	m.m_method = REGISTER_OPEN;
	rc = send_key_tau(knode->ki_key, &m);
	if (rc) {
		eprintk("send_key_tau %d", rc);
		destroy_key_tau(m.q.q_passed_key);
	}
	return rc;
}

static inline int init_knode (
	knode_s		*knode,
	replica_s	*replica,
	ki_t		key,
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
	struct inode		*inode = &knode->ki_vfs_inode;
	volume_s		*v     = replica->rp_volume;
	struct super_block	*sb    = v->vol_sb;
	int			rc;
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
	inode->i_blkbits = TAU_BLK_SHIFT;
	inode->i_blksize = TAU_BLK_SIZE;

	if (S_ISREG(mode)) {
		inode->i_op  = &kache_file_inode_operations;
		inode->i_fop = &kache_file_operations;
	} else if (S_ISDIR(mode)) {
		inode->i_op  = &kache_dir_inode_operations;
		inode->i_fop = &kache_file_operations;
	} else {
		eprintk("Not ready for this mode %o", inode->i_mode);
	}
	assert(inode->i_mapping);
	inode->i_mapping->a_ops = &kache_aops;

	knode->ki_key     = key;
	knode->ki_replica = replica;
	init_tree( &knode->ki_tree, &Kache_dir_species, inode, 0);

	rc = register_knode(knode);
	if (rc) {
		destroy_key_tau(key);
	}
	return rc;
}

static int create_spoke (
	replica_s	*replica,
	u64		parent_ino,
	u32		mode,
	struct qstr	*qname,
	u64		*child_ino,
	ki_t		*child_key)
{
	fsmsg_s		m;
	int		rc;
FN;
	m.crt_parent = parent_ino;
	m.crt_mode   = mode;
	m.crt_uid    = current->fsuid;
	m.crt_gid    = current->fsgid;
	m.m_method   = CREATE_SPOKE;
	rc = putdata_tau(replica->rp_key, &m, qname->len, qname->name);
	if (rc) {
		eprintk("putdata_tau %d", rc);
		return rc;
	}
	*child_ino = m.crr_ino;
	*child_key = m.q.q_passed_key;
	return 0;
}

static int kache_mknod (
	struct inode	*dir,
	struct dentry	*dentry,
	int		mode,
	dev_t		dev)
{
	struct super_block	*sb = dir->i_sb;
	struct inode		*inode = NULL;
	knode_s		*knode;
	knode_s		*parent = get_knode(dir);
	replica_s	*replica;
	ki_t		knode_key = 0;
	u64		knode_ino;
	struct timespec	time;
	int		rc;
FN;
	replica = pick_replica(parent->ki_replica->rp_volume);
	enter_tau(replica->rp_avatar);

	rc = create_spoke(replica, dir->i_ino, mode,
				&dentry->d_name,
				&knode_ino, &knode_key);
	if (rc) {
		eprintk("create_inode %d", rc);
		goto error;
	}
	rc = add_dir_entry(parent, dentry, knode_ino, mode);
	if (rc) {
		eprintk("add_dir_entry %d", rc);
		destroy_key_tau(knode_key);
		goto error;
	}

	inode = new_inode(sb);
	if (!inode) {
		eprintk("new_inode");
		rc = ENOSPC;
		goto error;
	}
	knode = get_knode(inode);
	time = CURRENT_TIME;
	rc = init_knode(knode, replica, knode_key, knode_ino, mode,
			current->fsuid, current->fsgid,
			0, 0,
			time, time, time);
	if (rc) goto error;

	insert_inode_hash(inode);
	mark_inode_dirty(inode);

	d_instantiate(dentry, inode);//XXX: not in the old code but may have been
					// higher up.
PRinode(inode);
	exit_tau();
	return 0;

error:
	if (inode) {
		make_bad_inode(inode);
		iput(inode);
	} else {
		return -ENOMEM;
	}
	exit_tau();
	return rc;
}

static int kache_mkdir (
	struct inode	*dir,
	struct dentry	*dentry,
	int		mode)
{
FN;
	return kache_mknod(dir, dentry, mode | S_IFDIR, 0);
}

static int kache_create (
	struct inode		*dir,
	struct dentry		*dentry,
	int			mode,
	struct nameidata	*nd)
{
FN;
	return kache_mknod(dir, dentry, mode | S_IFREG, 0);
}

static struct dentry *kache_lookup (
	struct inode		*parent,
	struct dentry		*dentry,
	struct nameidata	*nd)
{
	struct inode		*child = NULL;
	struct super_block	*sb = parent->i_sb;
	u64	ino;
FN;
PRs(dentry->d_name.name);
	ino = lookup_dir_entry(get_knode(parent), dentry);
PRd(ino);
	if (!ino) {
		goto not_found;
	}

	child = iget(sb, ino);
	if (!child) return NULL;	//We don't want to make a negative
					//entry for this case.

	return d_splice_alias(child, dentry);

not_found:
	d_add(dentry, child);
	return NULL;	
}

static int kache_unlink (
	struct inode		*dir,
	struct dentry		*dentry)
{
	struct inode		*inode = dentry->d_inode;
	int	rc = 0;
FN;
//XXX:	rc = kache_delete_dir(dir, dentry->d_name.name);
//XXX:	if (!rc) goto exit;

	dir->i_mtime = CURRENT_TIME;
	if (inode) --inode->i_nlink;
//XXX:exit:
	return rc;
}

static struct inode_operations kache_dir_inode_operations = {
	.create = kache_create,
	.lookup = kache_lookup,
	.mkdir  = kache_mkdir,
	.mknod  = kache_mknod,
	.unlink = kache_unlink,
};

static struct inode_operations kache_file_inode_operations = {
	.lookup = kache_lookup,
};

static void kache_read_inode (struct inode *inode)
{
	struct super_block	*sb = inode->i_sb;
	volume_s	*v = sb->s_fs_info;
	knode_s		*knode = get_knode(inode);
	replica_s	*replica;
	fsmsg_s		m;
	int		rc;
FN;
PRinode(inode);
	replica = vol_replica(v, inode->i_ino);
	if (!replica) goto error;

	enter_tau(replica->rp_avatar);
	m.m_method = INODE_OPEN;
	m.fs_inode.fs_ino = inode->i_ino;
	rc = call_tau(replica->rp_key, &m);
	if (rc) {
		eprintk("call_key_tau %d", rc);
		goto error;
	}
	rc = init_knode(knode, replica,
			m.q.q_passed_key,
			inode->i_ino,
			m.fs_inode.fs_mode,
			m.fs_inode.fs_uid,
			m.fs_inode.fs_gid,
			m.fs_inode.fs_size,
			m.fs_inode.fs_blocks,
			itime(m.fs_inode.fs_atime),
			itime(m.fs_inode.fs_mtime),
			itime(m.fs_inode.fs_ctime));
	if (rc) goto error;
	exit_tau();
	return;

error:
PRinode(inode);
	make_bad_inode(inode);
	exit_tau();
}

static int kache_write_inode (struct inode *inode, int wait)
{
	knode_s	*knode = get_knode(inode);
	fsmsg_s	m;
	int	rc;
FN;
PRinode(inode);
	enter_tau(knode->ki_replica->rp_avatar);
	m.fs_inode.fs_ino    = inode->i_ino;
	m.fs_inode.fs_size   = inode->i_size;
	m.fs_inode.fs_blocks = inode->i_blocks;
	m.fs_inode.fs_mode   = inode->i_mode;
	m.fs_inode.fs_uid    = inode->i_uid;
	m.fs_inode.fs_gid    = inode->i_gid;
	m.fs_inode.fs_atime  = inode->i_atime.tv_sec;
	m.fs_inode.fs_mtime  = inode->i_mtime.tv_sec;
	m.fs_inode.fs_ctime  = inode->i_ctime.tv_sec;
	m.m_method = INO_WRITE;
	rc = send_tau(knode->ki_key, &m);
	if (rc) {
		eprintk("ino %lx error=%d", inode->i_ino, rc);
		make_bad_inode(inode);
	}
	exit_tau();
	return rc;
}

#if 0
static void kache_clear_inode (struct inode *inode)
{
//	knode_s	*knode = get_knode(inode);
FN;
}
#endif

static void kache_put_super (struct super_block *sb)
{
	volume_s	*v = sb->s_fs_info;
FN;
HERE;
	sb->s_fs_info = NULL;
	cleanup_replica_matrix(v);
	die_tau(v->vol_avatar);
	kfree(v);
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,16))
static int kache_statfs (struct dentry *dentry, struct kstatfs *statfs)
{
	struct super_block *sb = dentry->d_sb;
#elif ((LINUX_VERSION_CODE == KERNEL_VERSION(2,6,16)) && (EXTRAVERSION > 56))
static int kache_statfs (struct dentry *dentry, struct kstatfs *statfs)
{
	struct super_block *sb = dentry->d_sb;
#else
static int kache_statfs (struct super_block *sb, struct kstatfs *statfs)
{
#endif
FN;
	statfs->f_type    = sb->s_magic;
	statfs->f_bsize   = sb->s_blocksize;
	statfs->f_blocks  = 17; //XXX:tsb->tb_numblocks;
	statfs->f_bfree   = 13; //XXX:tsb->tb_numblocks - tsb->tb_nextblock;
	statfs->f_bavail  = statfs->f_bfree;
	statfs->f_files   = 0;
	statfs->f_ffree   = 0;
	statfs->f_fsid.val[0] = sb->s_dev;	// See comments in "man statfs"
	statfs->f_fsid.val[1] = 0;
	statfs->f_namelen = TAU_FILE_NAME_LEN;
	statfs->f_frsize  = 0;	// Don't know what this is for.
	return 0;
}

static void kache_put_inode (struct inode *inode)
{
FN;
PRinode(inode);
}

static struct super_operations kache_sops = {
	.alloc_inode	= kache_alloc_inode,
	.destroy_inode	= kache_destroy_inode,
	.read_inode	= kache_read_inode,
	.write_inode	= kache_write_inode,
	.put_inode	= kache_put_inode,
//	.delete_inode	= kache_delete_inode,
	.put_super	= kache_put_super,
//	.write_super	= kache_write_super,
	.statfs		= kache_statfs,
//	.remount_fs	= kache_remount,
//	.clear_inode	= kache_clear_inode,
};

typedef struct ksw_type_s {
	type_s		tksw_tag;
	method_f	tksw_key_supplied;
} ksw_type_s;

typedef struct ksw_s {
	type_s			*ksw_type;
	struct semaphore	ksw_mutex;//XXX: would prefer to use synchronous
						// message operations.
	unint			ksw_key;
} ksw_s;	

static void ksw_key_supplied (void *msg)
{
	msg_s	*m = msg;
	ksw_s	*ksw = m->q.q_tag;
FN;
	ksw->ksw_key = m->q.q_passed_key;
	up( &ksw->ksw_mutex);
}

ksw_type_s	Ksw_type = { { SW_REPLY_MAX, 0 },
				.tksw_key_supplied = ksw_key_supplied };

/*
 * Function to parse volume arguments from -o mount option.
 * <option> ::= <arg>[,<arg>]
 * <arg>    ::= <name>[=<value>]
 * <value>  ::= <number>|<string>|<uuid>
 *
 * Have to pass in a list of functions for processing specific
 * name.
 */

enum { MAX_STRING = 64 };

typedef int	(*opt_f)(char *value, void *arg);

typedef struct option_s {
	char	*opt_name;
	opt_f	opt_func;
	void	*opt_arg;
	bool	opt_set;
} option_s;

int string_opt (char *value, void *arg)
{
	int	n;

	n = strlen(value);
	if (n >= MAX_STRING) return -1;

	strncpy(arg, value, n);
	return 0;
}

int number_opt (char *value, void *arg)
{
	u64	n;

	n = simple_strtoul(value, NULL, 0);
	*(u64 *)arg = n;
	return 0;
}

int guid_opt (char *value, void *arg)
{
	return uuid_parse(value, arg);
}

static char *get_token (char **cmdline, char *separator)
{
	char	*p = *cmdline;
	char	*token;
	int	c;

	while (isspace(*p)) {
		++p;
	}
	for (token = p;; p++) {
		c = *p;
		if (isspace(c)) {
			*p = '\0';
			do {
				c = *++p;
			} while (isspace(c));
			break;
		}
		if (c == '\0') break;
		if (c == '=') break;
		if (c == ',') break;
	}
	*p = '\0';
	*cmdline = ++p;
	*separator = c;
	return token;
}

/*static*/ bool isset_opt (option_s *options, char *name)
{
	option_s	*opt;

	for (opt = options; opt->opt_name; opt++) {
		if (strcmp(name, opt->opt_name) == 0) {
			return opt->opt_set;
		}
	}
	return -FALSE;
}

static int set_opt (option_s *options, char *name, char *value)
{
	option_s	*opt;
	int		rc = 0;

	for (opt = options; opt->opt_name; opt++) {
		if (strcmp(name, opt->opt_name) == 0) {
			if (opt->opt_func) {
				rc = opt->opt_func(value, opt->opt_arg);
				return rc;
			}
			opt->opt_set = TRUE;
			return rc;
		}
	}
	return -1;
}

static void parse_opt (option_s *options, char *cmdline)
{
	char	separator;
	char	*name;
	char	*value;
	int	rc;
 
	for (;;) {
		name = get_token( &cmdline, &separator);
		if (separator == '=') {
			value = get_token( &cmdline, &separator);
		} else {
			value = NULL;
		}
		rc = set_opt(options, name, value);
		if (rc) {
			eprintk("name=%s value=%s", name, value);
		}
		if (separator == '\0') break;
	}
}

static void kache_get_options (char *data, volume_s *v)
{
	option_s	options[] = {
		{ "vol", string_opt, v->vol_name, FALSE },
		{ "guid", guid_opt, v->vol_guid, FALSE },
		{ 0, 0, 0, 0 }};

	parse_opt(options, data);
}

static int find_root (volume_s *volume)
{
#if 0
	ksw_s	*ksw;
	ki_t	key;
	int	rc;
FN;
	ksw = zalloc_tau(sizeof(ksw_s));
	assert(ksw);
	ksw->ksw_type = &Ksw_type.tksw_tag;
	init_MUTEX_LOCKED( &ksw->ksw_mutex);

	key = make_gate(ksw, REQUEST | PASS_OTHER);
	if (!key) {
		return -1;
	}
	rc = sw_request(ksi->ksi_vol_name, key);
	if (rc) {
		destroy_key_tau(key);
		return rc;
	}
	down( &ksw->ksw_mutex);
	ksi->ksi_key = ksw->ksw_key;
	//XXX: ksw has to be left around because we might receive more messages
	// on it. This needs to be rethought.
	
	return 0;
#endif
	return 0;
}

typedef struct bag_stat_s {
	guid_t	bs_vol_id;
	guid_t	bs_bag_id;
	u32	bs_nshards;
	u32	bs_nreplicas;
} bag_stat_s;

// Should probably poll all bags for information
static int stat_vol (ki_t key, volume_s *volume)
{
	fsmsg_s	m;
	int	rc;

	if (!key) {
		return qERR_NOT_FOUND;
	}
	m.m_method = STAT_BAG;
	rc = call_tau(key, &m);
	if (rc) {
		eprintk("call %d", rc);
		destroy_key_tau(key);
		return rc;
	}
	if (uuid_compare(volume->vol_guid, m.bag_guid_vol) != 0) {
		destroy_key_tau(key);
		return qERR_NOT_FOUND;
	}
	if (m.bag_state != TAU_BAG_VOLUME) {
		eprintk("bag in wrong state %lld", m.bag_state);
		return qERR_NOT_FOUND;
	}
	
	uuid_copy(volume->vol_guid, m.bag_guid_vol);
	volume->vol_num_shards   = m.bag_num_shards;
	volume->vol_num_replicas = m.bag_num_replicas;

	destroy_key_tau(key);
	return 0;
}

static int find_bag (volume_s *volume)
{
	u64	sequence = 0;
	ki_t	key;
	int	rc;
	int	i;

	for (i = 0; i < 100; i++) {
		rc = sw_next("bag", &key, sequence, &sequence);
		if (rc == DESTROYED) break;
		if (rc) {
			eprintk("find_bag failed %d", rc);
			return rc;
		}
		rc = stat_vol(key, volume);
		if (rc) {
			eprintk("find_bag failed %d", rc);
		} else {
			return rc;
		}
	}
	return qERR_NOT_FOUND;
}

static void vol_receive (int rc, void *msg)
{
	struct object_s {
		type_s	*obj_type;
	} *obj;
	msg_s	*m = msg;
	type_s	*type;
FN;
	obj  = m->q.q_tag;
	type = obj->obj_type;
	if (!rc) {
		if (m->m_method < type->ty_num_methods) {
			type->ty_method[m->m_method](m);
			return;
		}
		printk(KERN_INFO "bad method %u >= %u\n",
			m->m_method, type->ty_num_methods);
		if (m->q.q_passed_key) {
			destroy_key_tau(m->q.q_passed_key);
		}
		return;	
	}
	if (rc == DESTROYED) {
		type->ty_destroy(m);
		return;
	}
	eprintk("err = %d", rc);
}

static int kache_fill_super (
	struct super_block	*sb,
	void			*data,		/* Command line */
	int			isSilent)
{
	struct inode	*iroot  = NULL;
	volume_s	*v = NULL;
	int		rc = 0;
FN;
	sb->s_blocksize	     = TAU_BLK_SIZE;
	sb->s_blocksize_bits = TAU_BLK_SHIFT;
	sb->s_maxbytes	     = TAU_MAX_FILE_SIZE;
	sb->s_magic	     = (__u32)KACHE_MAGIC_SUPER;
	sb->s_op             = &kache_sops;
	sb->s_export_op      = NULL;//&kache_export_ops;// I think this is for NFS

	v = kmalloc(sizeof(*v), GFP_KERNEL);
	if (!v) {
		rc = -ENOMEM;
		goto error;
	}
	zero(*v);
	sb->s_fs_info = v;
	v->vol_sb = sb;

	kache_get_options(data, v);

	v->vol_avatar = init_msg_tau(v->vol_name, vol_receive);
	if (!v->vol_avatar) goto error;
	enter_tau(v->vol_avatar);

	rc = sw_register(v->vol_name);
	if (rc) goto error;

	rc = find_bag(v);
	if (rc) goto error;

	rc = init_replica_matrix(v);
	if (rc) {
		eprintk("init_replica_matrix = %d", rc);
		goto error;
	}

	rc = find_root(v);
	if (rc) goto error;

	iroot = iget(sb, TAU_ROOT_INO);
	sb->s_root = d_alloc_root(iroot);
	if (!sb->s_root) {
		eprintk("get root inode");
		goto error;
	}
	if (!S_ISDIR(iroot->i_mode)) {	// Check should have been done by taufs
		eprintk("corrupt root inode, run taufsck");
		goto error;
	}
	exit_tau();
	return 0;

error:
HERE;
	if (rc == 0) rc = -EIO;
	if (sb->s_root) {
		dput(sb->s_root);
		sb->s_root = NULL;
	} else if (iroot) {
		iput(iroot);
	}
	if (v) {
		cleanup_replica_matrix(v);
		if (v->vol_avatar) die_tau(v->vol_avatar);
		kfree(v);
		sb->s_fs_info = NULL;
	}
	exit_tau();
	return kache_err(rc);
}

static struct super_block *kache_get_sb (
	struct file_system_type	*fs_type,
	int			flags,
	const char		*dev_name,
	void			*data)
{
FN;
	return get_sb_nodev(fs_type, flags, data, kache_fill_super);
}

static void kache_kill_super (struct super_block *sb)
{
FN;
HERE;
	kill_anon_super(sb);
}

static struct file_system_type kache_fs_type = {
	.owner    = THIS_MODULE,
	.name     = "kache",
	.get_sb   = kache_get_sb,
	.kill_sb  = kache_kill_super,
	.fs_flags = 0,
};

int kache_err (int rc)
{
//	extern int	tau_err (int rc);

	if (!rc) return rc;

	if (IS_ERR_VALUE(rc)) return rc;

	eprintk("error is %d returning -1\n", rc);

	return -1;
}

static void kache_exit (void)
{
	extern void	tau_exit (void);

	tau_exit();
	unregister_filesystem( &kache_fs_type);
	if (Knode_cachep) destroy_knode_cache();
}

static int kache_init (void)
{
	extern int	tau_init (void);
	int	rc;

FN;
//XXX:	rc = init_kache_buf_cache();
//XXX:	if (rc) goto error;

	rc = init_knode_cache();
	if (rc) goto error;

	rc = register_filesystem( &kache_fs_type);
	if (rc) goto error;

	rc = tau_init();
	if (rc) goto error;

	return 0;
error:
	printk(KERN_INFO "tau error=%d\n", rc);
	kache_exit();
	return kache_err(rc);	
}

MODULE_AUTHOR("Paul Taysom");
MODULE_DESCRIPTION("tau file system");
MODULE_LICENSE("GPL v2");

module_init(kache_init)
module_exit(kache_exit)

