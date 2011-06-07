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
#include <linux/statfs.h>
#include <linux/ctype.h>

#include <zParams.h>

#include <style.h>
#include <tau/msg.h>
#include <tau/switchboard.h>
#include <tau/msg_util.h>
#include <tau/debug.h>

#include <jill.h>

#define PRinode(_x_)	pr_inode(__func__, __LINE__, _x_)

enum {	JACK_NAME          = 1<<6,
	JACK_MAGIC         = 1784767339,
	JACK_BLK_SHIFT     = 12,
	JACK_BLK_SIZE      = 1<<JACK_BLK_SHIFT,
	JACK_BLK_MASK      = JACK_BLK_SIZE - 1,
//	JACK_MAX_FILE_SIZE = 1ull<<63,
	JACK_ROOT_INO      = zROOTDIR_ZID,
	JACK_DOT           = -2,
	JACK_DOTDOT        = -1 };

#define JACK_MAX_FILE_SIZE	(1ull << 63)

struct {
	unint	inode_alloc;
	unint	inode_free;
} Jack_inst;

typedef struct vol_s {
	avatar_s		*v_avatar;
	struct super_block	*v_sb;
	char			v_name[JACK_NAME+1];
} vol_s;

typedef struct jnode_s {
	ki_t		j_key;
	struct inode	j_vfs_inode;
} jnode_s;

static inline jnode_s *get_jnode (struct inode *inode)
{
	return container_of(inode, jnode_s, j_vfs_inode);
}

static int jack_err (int rc);

/*
 * UTF-8 is a encoding of Unicode developed by Bell Labs
 * and adopted as a standard.  It has numerous advantages
 * over regular unicode for serial communication.  The best
 * is that C ASCII strings have the same representation.
 *
 * 0xxxxxxx
 * 110xxxxx 10xxxxxx
 * 1110xxxx 10xxxxxx 10xxxxxx
 *
 * Information taken from "Java in a Nutshell":
 * 1. All ASCII characters are one-byte UTF-8 characters.  A legal
 * ASCII string is a legal UTF-8 string.
 * 2. Any non-ASCII character (i.e., any character with the high-
 * order bit set) is part of a multibyte character.
 * 3. The first byte of any UTF-8 character indicates the number
 * of additional bytes in the character.
 * 4. The first byte of a multibyte character is easily distinguished
 * from the subsequent bytes.  Thus, it is easy to locate the start
 * of a character from an arbitrary position in a data stream.
 * 5. It is easy to convert between UTF-8 and Unicode.
 * 6. The UTF-8 encoding is relatively compact.  For text with a large
 * percentage of ASCII characters, is it more compact than Unicode.  In
 * the worst case, a UTF-8 string is only 50% larger than the corresponding
 * Unicode string.
 */
static int uni2utf (	/* Return -1 = err, else # utf bytes in dest. */
	const unicode_t *unicode,/* Source */
	utf8_t	*utf,	/* Destination */
	int	len)	/* Destination length in bytes */
{
	unicode_t	u;
	utf8_t		*end;

	if ((unicode == NULL) || (utf == NULL) || (len < 1)) {
		return -1;
	}

	end = utf + len - 1;
	for (u = *unicode; u != 0; u = *++unicode) {
		if (u < 0x80) {
			if (utf >= end) {
				*utf = 0;
				return -1;
			}
			*utf++ = u;
		} else if (u < 0x800) {
			if (utf + 1 >= end) {
				*utf = 0;
				return -1;
			}
			*utf++ = (0xc0 | (u >> 6));
			*utf++ = (0x80 | (u & 0x3f));
		} else {
			if (utf + 2 >= end) {
				*utf = 0;
				return -1;
			}
			*utf++ = (0xe0 | (u >> 12));
			*utf++ = (0x80 | ((u >> 6) & 0x3f));
			*utf++ = (0x80 | (u & 0x3f));
		}
	}
	*utf = 0;
	return len - 1 - (end - utf);
}

void pr_inode (const char *fn, unsigned line, struct inode *inode)
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

//////////////////////////////////////////////////////////////////////////////

static char	Module[] = "jack";
static avatar_s	*Jack;

static LONG	My_id = 0;

static ki_t	Jill_key;
static ki_t	Jill_root;
static ki_t	Current;

static void jack_receive (int rc, void *msg)
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
		iprintk("bad method %u >= %u",
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

void jack_close (ki_t key)
{
	destroy_key_tau(key);
}

void jack_init (u32 task_id, u32 name_space)
{
	jmsg_s	m;
	int	rc;

	Jack = init_msg_tau(Module, NULL /* receive */);
	if (!Jack) {
		eprintk("init_msg_tau");
		return;
	}
	sw_register(Module);

	rc = sw_any("jill", &Jill_key);
	if (rc) {
		eprintk("sw_request %d", rc);
		return;
	}
	m.m_method      = JILL_SW_REG;
	m.ji_id         = My_id;
	m.ji_task       = task_id;
	m.ji_name_space = name_space;
	rc = call_tau(Jill_key, &m);
	if (rc) eprintk("call_tau %d", rc);

	Jill_root = m.q.q_passed_key;
	Current  = Jill_root;
}

int jack_root_key (u32 id, u32 task_id, u32 name_space, ki_t *key)
{
	jmsg_s	m;
	int	rc;

	m.m_method      = JILL_SW_REG;
	m.ji_id         = 0;
	m.ji_task       = task_id;
	m.ji_name_space = name_space;
	rc = call_tau(Jill_key, &m);
	if (rc) {
		eprintk("call_tau %d", rc);
		return rc;
	}
	*key = m.q.q_passed_key;
	return 0;
}

int jack_create (
	char	*path,
	u64	file_type,
	u64	file_attributes,
	u64	create_flags,
	u64	rights,
	ki_t	*key)
{
	jmsg_s	m;
	unint	len;
	int	rc;

	len = strlen(path) + 1;

	m.m_method            = JILL_CREATE;
	m.ji_file_type        = file_type;
	m.ji_file_attributes  = file_attributes;
	m.ji_create_flags     = create_flags;
	m.ji_requested_rights = rights;

	rc = putdata_tau(Current, &m, len, path);
	if (rc) {
		eprintk("putdata_tau \"%s\" = %d", path, rc);
		return rc;
	}
	*key = m.q.q_passed_key;
	return 0;
}

int jack_delete (char *path)
{
	jmsg_s	m;
	unint	len;
	int	rc;

	len = strlen(path) + 1;

	m.m_method = JILL_DELETE;

	rc = putdata_tau(Current, &m, len, path);
	if (rc) {
		eprintk("putdata_tau \"%s\" = %d", path, rc);
		return rc;
	}
	return 0;
}

int unilen (unicode_t *u)
{
	int	i;

	for (i = 0; *u++; i++)
		;
	return i;
}

int jack_new_connection (ki_t key, unicode_t *fdn)
{
	jmsg_s	m;
	int	len;
	int	rc;

	m.m_method = JILL_NEW_CONNECTION;
	len = unilen(fdn) + 1;

	rc = getdata_tau(key, &m, len, fdn);
	if (rc) {
		eprintk("getdata_tau %d", rc);
		return rc;
	}
	return 0;
}

int jack_enumerate (ki_t dir, u64 cookie, zInfo_s *info, u64 *ret_cookie)
{
	jmsg_s	m;
	int	rc;

	m.m_method = JILL_ENUMERATE;
	m.jo_cookie = cookie;

	rc = getdata_tau(dir, &m, sizeof(zInfo_s), info);
	if (rc) {
		eprintk("putdata_tau %d", rc);
		return rc;
	}
	*ret_cookie = m.jo_cookie;
	return 0;
}

int jack_get_info (ki_t key, u64 mask, zInfoC_s *info)
{
	jmsg_s	m;
	int	rc;

	m.m_method = JILL_GET_INFO;
	m.jo_info_mask = mask;

	rc = getdata_tau(key, &m, sizeof(*info), info);
	if (rc) {
		eprintk("getdata_tau %d", rc);
		return rc;
	}
	return 0;
}

int jack_modify_info (ki_t key, u64 mask, zInfoC_s *info)
{
	jmsg_s	m;
	int	rc;

	m.m_method = JILL_GET_INFO;
	m.jo_info_mask = mask;

	rc = putdata_tau(key, &m, sizeof(*info), info);
	if (rc) {
		eprintk("getdata_tau %d", rc);
		return rc;
	}
	return 0;
}

int jack_open (char *path, u64 rights, ki_t *key)
{
	jmsg_s	m;
	unint	len;
	int	rc;

	len = strlen(path) + 1;

	m.m_method = JILL_OPEN;
	m.ji_requested_rights = rights;

	rc = putdata_tau(Current, &m, len, path);
	if (rc) {
		eprintk("putdata_tau \"%s\" = %d", path, rc);
		return rc;
	}
	*key = m.q.q_passed_key;
	return 0;
}

int jack_read (ki_t key, unint len, void *buf, u64 offset, unint *bytes_read)
{
	jmsg_s	m;
	int	rc;

	m.m_method = JILL_READ;
	m.jo_offset = offset;

	rc = getdata_tau(key, &m, len, buf);
	if (rc) {
		eprintk("getdata_tau %d", rc);
		return rc;
	}
	*bytes_read = m.jo_length;
	return 0;
}

int jack_write (ki_t key, unint len, void *buf, u64 offset, unint *bytes_written)
{
	jmsg_s	m;
	int	rc;

	m.m_method = JILL_WRITE;
	m.jo_offset = offset;

	rc = putdata_tau(key, &m, len, buf);
	if (rc) {
		eprintk("putdata_tau %d", rc);
		return rc;
	}
	*bytes_written = m.jo_length;
	return 0;
}

int jack_zid_open (u64 zid, u64 rights, ki_t *key)
{
	jmsg_s	m;
	int	rc;

	m.m_method            = JILL_ZID_OPEN;
	m.ji_zid              = zid;
	m.ji_requested_rights = rights;

	rc = call_tau(Current, &m);
	if (rc) {
		eprintk("call_tau zid=%llx %d", zid, rc);
		return rc;
	}
	*key = m.q.q_passed_key;
	return 0;
}
//////////////////////////////////////////////////////////////////////////////

static int jack_readdir (struct file *fp, void *dirent, filldir_t filldir)
{
	struct dentry 	*dentry = fp->f_dentry;
	struct inode	*inode  = dentry->d_inode;
	jnode_s		*jnode  = get_jnode(inode);
	u64		cookie  = fp->f_pos;
	zInfo_s		zinfo;
	unicode_t	*uname;
	utf8_t		name[JACK_NAME];
	int		done;
	int		rc;
FN;
	cookie += JACK_DOT;
	if (cookie == JACK_DOT) {
		done = filldir(dirent, ".", sizeof(".")-1,
				0,
				fp->f_dentry->d_inode->i_ino,
				DT_DIR);
		if (done) goto exit;
		++cookie;
	}
	if (cookie == JACK_DOTDOT) {
		done = filldir(dirent, "..", sizeof("..")-1,
				1,
				fp->f_dentry->d_parent->d_inode->i_ino,
				DT_DIR);
		if (done) goto exit;
		++cookie;
	}
	for (;;) {
		rc = jack_enumerate(jnode->j_key, cookie, &zinfo, &cookie);
		if (rc) {
			break;
		}
		uname = zINFO_NAME( &zinfo);
		uni2utf(uname, name, sizeof(name));

		done = filldir(dirent, name,
			strlen(name), zinfo.std.zid, cookie, 0/*type?*/);
	}
exit:
	fp->f_pos = cookie - JACK_DOT;
	return 0;
}

static int jack_release_file (struct inode *inode, struct file *filp)
{
FN;
PRinode(inode);
	return 0;
}

static int jack_sync_file (
	struct file     *file,
	struct dentry   *dentry,
	int             datasync)
{
FN;
	return 0;
}

struct file_operations jack_file_operations = {
	.llseek		= generic_file_llseek,
	.read		= generic_file_read,
	.write		= generic_file_write,
	.aio_read	= generic_file_aio_read,
	.aio_write	= generic_file_aio_write,
	.readdir	= jack_readdir,
//	.ioctl		= jack_ioctl,
	.mmap		= generic_file_mmap,
	.open		= generic_file_open,
	.release	= jack_release_file,
	.fsync		= jack_sync_file,
	.readv		= generic_file_readv,
	.writev		= generic_file_writev,
	.sendfile	= generic_file_sendfile,
};
//////////////////////////////////////////////////////////////////////////////


static struct dentry *jack_lookup (
	struct inode		*parent,
	struct dentry		*dentry,
	struct nameidata	*nd)
{
	struct inode		*child = NULL;
	struct super_block	*sb = parent->i_sb;
	u64	ino;
FN;
PRs(dentry->d_name.name);
	ino = lookup_dir_entry(get_jnode(parent), dentry);
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


static struct inode_operations jack_dir_inode_operations = {
	.create = jack_create,
	.lookup = jack_lookup,
	.mkdir  = jack_mkdir,
	.mknod  = jack_mknod,
	.unlink = jack_unlink,
};

static struct inode_operations jack_file_inode_operations = {
	.lookup = jack_lookup,
};
//////////////////////////////////////////////////////////////////////////////

static inline int init_jnode (
	jnode_s		*jnode,
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
	struct inode		*inode = &jnode->j_vfs_inode;
	struct super_block	*sb    = inode->i_sb;
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
	inode->i_rdev    = sb->s_dev;
	inode->i_blkbits = JACK_BLK_SHIFT;
	inode->i_blksize = JACK_BLK_SIZE;

	if (S_ISREG(mode)) {
		inode->i_op  = &jack_file_inode_operations;
		inode->i_fop = &jack_file_operations;
	} else if (S_ISDIR(mode)) {
		inode->i_op  = &jack_dir_inode_operations;
		inode->i_fop = &jack_file_operations;
	} else {
		eprintk("Not ready for this mode %o", inode->i_mode);
	}
	assert(inode->i_mapping);
	inode->i_mapping->a_ops = &jack_aops;

	jnode->j_key = key;
	return rc;
}

static kmem_cache_t *Jnode_cachep;

static struct inode *jack_alloc_inode (struct super_block *sb)
{
	jnode_s	*jnode;
FN;
	jnode = kmem_cache_alloc(Jnode_cachep, SLAB_KERNEL);
	if (!jnode) return NULL;

	++Jack_inst.inode_alloc;

	return &jnode->j_vfs_inode;
}

static void jack_destroy_inode (struct inode *inode)
{
	jnode_s	*jnode = get_jnode(inode);
FN;
	assert(inode);

	enter_tau(Jack);

	kmem_cache_free(Jnode_cachep, jnode);
	++Jack_inst.inode_free;

	exit_tau();
}

static void jnode_init_once (
	void		*t,
	kmem_cache_t	*cachep,
	unsigned long	flags)
{
	jnode_s	*jnode = t;
FN;
	if ((flags & (SLAB_CTOR_VERIFY|SLAB_CTOR_CONSTRUCTOR))
			== SLAB_CTOR_CONSTRUCTOR) {
		inode_init_once( &jnode->j_vfs_inode);
	}
}

static int init_jnode_cache (void)
{
FN;
	Jnode_cachep = kmem_cache_create("jnode_cache",
				sizeof(jnode_s),
				0, SLAB_HWCACHE_ALIGN|SLAB_RECLAIM_ACCOUNT,
				jnode_init_once, NULL);
	if (Jnode_cachep == NULL) {
		return -ENOMEM;
	}
	return 0;
}

static void destroy_jnode_cache (void)
{
FN;
	if (!Jnode_cachep) return;
	if (Jack_inst.inode_alloc != Jack_inst.inode_free) {
		eprintk("alloc=%ld free=%ld",
			Jack_inst.inode_alloc, Jack_inst.inode_free);
	}
	if (kmem_cache_destroy(Jnode_cachep)) {
		eprintk("not all structures were freed");
	}
}

//////////////////////////////////////////////////////////////////////////////


//////	jack_init(zNO_TASK, zNSPACE_LONG|zMODE_UTF8|zMODE_LINK);

//////////////////////////////////////////////////////////////////////////

u32	Jack_posix_permission_mask = 0755;

static inline struct timespec itime (u64 secs)
{
	return ((struct timespec) { secs, 0 });
}

static u32 gen_mode (u32 attr)
{
	u32	mode = 0666;

	if (attr & zFA_SUBDIRECTORY) mode |= S_IFDIR;
	else if (attr & zFA_IS_LINK) mode |= S_IFLNK;
	else mode |= S_IFREG;

	if (attr & zFA_READ_ONLY) mode &= ~0222;// Turn off write permission
	if (attr & zFA_HIDDEN)    mode &= ~0444;// Turn off read permission
	if ((attr & zFA_SUBDIRECTORY) || (attr & zFA_EXECUTE)) {
		mode |= 0111;   // Turn on execute/search permission
	}
	mode = mode & (Jack_posix_permission_mask | ~0777);

	return mode;
}

static void jack_read_inode (struct inode *inode)
{
	jnode_s		*jnode = get_jnode(inode);
	ki_t		key = 0;
	zInfoC_s	zinfo;
	int		rc;
FN;
	enter_tau(Jack);
	rc = jack_zid_open(inode->i_ino, 0, &key);
	if (rc) {
		eprintk("jack_zid_open %d", rc);
		goto error;
	}
	rc = jack_get_info(key,
		zGET_STD_INFO | zGET_STORAGE_USED | zGET_TIMES_IN_SECS,
		&zinfo);
	rc = init_jnode(jnode,
			0,
			inode->i_ino,
			gen_mode(zinfo.std.fileAttributes),
			0, //uid
			0, //gid
			zinfo.std.logicalEOF,
			(zinfo.storageUsed.dataBytes + JACK_BLK_MASK)
				>> JACK_BLK_SHIFT,
			itime(zinfo.time.accessed),
			itime(zinfo.time.modified),
			itime(zinfo.time.metaDataModified));
	if (rc) goto error;
	destroy_key_tau(key);
	exit_tau();
	return;

error:
	destroy_key_tau(key);
	make_bad_inode(inode);
	exit_tau();
}

static int jack_statfs (struct dentry *dentry, struct kstatfs *statfs)
{
	struct super_block	*sb = dentry->d_sb;
	zInfoC_s		zinfo;
	u64			total_blocks;
	u64			in_use_blocks;
	int			rc;
FN;
	enter_tau(Jack);
	rc = jack_get_info(0,
		zGET_VOLUME_INFO | zGET_POOL_INFO | zGET_VOL_SALVAGE_INFO,
		&zinfo);
	if (rc) {
		eprintk("jack_get_info %d", rc);
		goto error;
	}
	total_blocks = zinfo.pool.totalSpace >> PAGE_SHIFT;
	if (zinfo.pool.totalSpace != zinfo.pool.numUsedBytes) {
		in_use_blocks = (zinfo.pool.numUsedBytes
				- zinfo.pool.purgeableBytes) >> PAGE_SHIFT;
	} else {
		in_use_blocks = zinfo.pool.numUsedBytes >> PAGE_SHIFT;
	}
	statfs->f_type    = sb->s_magic;
	statfs->f_bsize   = sb->s_blocksize;
	statfs->f_blocks  = total_blocks;
	statfs->f_bfree   = total_blocks - in_use_blocks;
	statfs->f_bavail  = statfs->f_bfree;
	statfs->f_files   = zinfo.vol.numObjects;
	statfs->f_ffree   = 0;
	statfs->f_fsid.val[0] = sb->s_dev;	// See comments in "man statfs"
	statfs->f_fsid.val[1] = 0;
	statfs->f_namelen     = JILL_NAME_LEN;
	statfs->f_frsize      = 0;	// Don't know what this is for.
	exit_tau();
	return 0;
error:
	exit_tau();
	return jack_err(rc);

}

static struct super_operations jack_sops = {
	.alloc_inode	= jack_alloc_inode,
	.destroy_inode	= jack_destroy_inode,
	.read_inode	= jack_read_inode,
	.write_inode	= jack_write_inode,
	.put_inode	= jack_put_inode,
//	.delete_inode	= jack_delete_inode,
	.put_super	= jack_put_super,
//	.write_super	= jack_write_super,
	.statfs		= jack_statfs,
//	.remount_fs	= jack_remount,
//	.clear_inode	= jack_clear_inode,
};


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

static void jack_get_options (char *data, vol_s *v)
{
	option_s	options[] = {
		{ "vol", string_opt, v->v_name, FALSE },
		{ 0, 0, 0, 0 }};

	parse_opt(options, data);
}

static int jack_fill_super (
	struct super_block	*sb,
	void			*data,		/* Command line */
	int			isSilent)
{
	struct inode	*iroot  = NULL;
	vol_s		*v = NULL;
	int		rc = 0;
FN;
	sb->s_blocksize	     = JACK_BLK_SIZE;
	sb->s_blocksize_bits = JACK_BLK_SHIFT;
	sb->s_maxbytes	     = JACK_MAX_FILE_SIZE;
	sb->s_magic	     = JACK_MAGIC;
	sb->s_op             = &jack_sops;
	sb->s_export_op      = NULL;//&jack_export_ops;// I think this is for NFS

	v = kmalloc(sizeof(*v), GFP_KERNEL);
	if (!v) {
		rc = -ENOMEM;
		goto error;
	}
	zero(*v);
	sb->s_fs_info = v;
	v->v_sb = sb;

	jack_get_options(data, v);

	v->v_avatar = init_msg_tau(v->v_name, jack_receive);
	if (!v->v_avatar) goto error;

	rc = sw_register(v->v_name);
	if (rc) goto error;

	iroot = iget(sb, JACK_ROOT_INO);
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
		if (v->v_avatar) die_tau(v->v_avatar);
		kfree(v);
		sb->s_fs_info = NULL;
	}
	exit_tau();
	return jack_err(rc);
}

static struct super_block *jack_get_sb (
	struct file_system_type	*fs_type,
	int			flags,
	const char		*dev_name,
	void			*data)
{
FN;
	return get_sb_nodev(fs_type, flags, data, jack_fill_super);
}

static void jack_kill_super (struct super_block *sb)
{
FN;
HERE;
	kill_anon_super(sb);
}

static struct file_system_type jack_fs_type = {
	.owner    = THIS_MODULE,
	.name     = "jack",
	.get_sb   = jack_get_sb,
	.kill_sb  = jack_kill_super,
	.fs_flags = 0,
};

static int jack_err (int rc)
{
	if (!rc) return rc;

	if (IS_ERR_VALUE(rc)) return rc;

	eprintk("error is %d returning -1", rc);

	return -1;
}

static void jack_exit (void)
{
	unregister_filesystem( &jack_fs_type);
	destroy_jnode_cache();
}

static int jack_init (void)
{
	extern int	tau_init (void);
	int	rc;

FN;
	rc = init_jnode_cache();
	if (rc) goto error;

	rc = register_filesystem( &jack_fs_type);
	if (rc) goto error;

	return 0;
error:
	printk(KERN_INFO "jack error=%d\n", rc);
	jack_exit();
	return jack_err(rc);
}

MODULE_AUTHOR("Paul Taysom");
MODULE_DESCRIPTION("jack file system");
MODULE_LICENSE("GPL v2");

module_init(jack_init)
module_exit(jack_exit)

