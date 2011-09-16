/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

#include <linux/magic.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <stdio.h>

#include <style.h>
#include <eprintf.h>

#define num_elements(_a)	(sizeof(_a) / sizeof(_a[0]))

typedef struct Fstype_s {
	long	type;
	char	*name;
} Fstype_s;

#define FSTYPE(x)	{ x, # x }

Fstype_s Fstype[] = {
	FSTYPE(ADFS_SUPER_MAGIC),
	FSTYPE(AFFS_SUPER_MAGIC),
	FSTYPE(AFS_SUPER_MAGIC),
	FSTYPE(AUTOFS_SUPER_MAGIC),
	FSTYPE(CODA_SUPER_MAGIC),
	FSTYPE(CRAMFS_MAGIC),
	FSTYPE(CRAMFS_MAGIC_WEND),
	FSTYPE(DEBUGFS_MAGIC),
	FSTYPE(SYSFS_MAGIC),
	FSTYPE(SECURITYFS_MAGIC),
	FSTYPE(SELINUX_MAGIC),
	FSTYPE(RAMFS_MAGIC),
	FSTYPE(TMPFS_MAGIC),
	FSTYPE(HUGETLBFS_MAGIC),
	FSTYPE(SQUASHFS_MAGIC),
	FSTYPE(EFS_SUPER_MAGIC),
	FSTYPE(EXT2_SUPER_MAGIC),
	FSTYPE(EXT3_SUPER_MAGIC),
	FSTYPE(XENFS_SUPER_MAGIC),
	FSTYPE(EXT4_SUPER_MAGIC),
	FSTYPE(BTRFS_SUPER_MAGIC),
	FSTYPE(HPFS_SUPER_MAGIC),
	FSTYPE(ISOFS_SUPER_MAGIC),
	FSTYPE(JFFS2_SUPER_MAGIC),
	FSTYPE(ANON_INODE_FS_MAGIC),
	FSTYPE(MINIX_SUPER_MAGIC),
	FSTYPE(MINIX_SUPER_MAGIC2),
	FSTYPE(MINIX2_SUPER_MAGIC),
	FSTYPE(MINIX2_SUPER_MAGIC2),
	FSTYPE(MINIX3_SUPER_MAGIC),
	FSTYPE(MSDOS_SUPER_MAGIC),
	FSTYPE(NCP_SUPER_MAGIC),
	FSTYPE(NFS_SUPER_MAGIC),
	FSTYPE(OPENPROM_SUPER_MAGIC),
	FSTYPE(PROC_SUPER_MAGIC),
	FSTYPE(QNX4_SUPER_MAGIC),
	FSTYPE(REISERFS_SUPER_MAGIC),
	FSTYPE(SMB_SUPER_MAGIC),
	FSTYPE(USBDEVICE_SUPER_MAGIC),
	FSTYPE(CGROUP_SUPER_MAGIC),
	FSTYPE(FUTEXFS_SUPER_MAGIC),
	/*FSTYPE(INOTIFYFS_SUPER_MAGIC), not defined in earlier headers */
	FSTYPE(STACK_END_MAGIC),
	FSTYPE(DEVPTS_SUPER_MAGIC),
	FSTYPE(SOCKFS_MAGIC)};

static char *lookup_fstype(long fstype)
{
	int	i;

	for (i = 0; i < num_elements(Fstype); i++) {
		if (fstype == Fstype[i].type) return Fstype[i].name;
	}
	return "<fs type unknown>";
}

int dostatfs (const char *path)
{
	struct statfs	fs;
	int	rc;

	rc = statfs(path, &fs);
	if (rc) {
		warn("statfs failed %s:", path);
		return rc;
	}
 	printf("statfs for %s:\n"
		"\ttype of file system: %14llx %s\n"
		"\tblock size         : %'14lld\n"
		"\ttotal data blocks  : %'14lld\n"
		"\tfree blocks        : %'14lld\n"
		"\tinuse (total-free) : %'14lld\n"
		"\tavailable blocks   : %'14lld\n"
		"\ttotal inodes       : %'14lld\n"
		"\tfree inodes        : %'14lld\n"
		"\tinuse (total-free) : %'14lld\n"
		"\tfile system id     :     0x%.8x 0x%.8x\n"
		"\tmax file name len  : %'14lld\n",
		path,
		(s64)fs.f_type, lookup_fstype(fs.f_type),
		(s64)fs.f_bsize,
		(s64)fs.f_blocks,
		(s64)fs.f_bfree,
		(s64)fs.f_blocks - (s64)fs.f_bfree,
		(s64)fs.f_bavail,
		(s64)fs.f_files,
		(s64)fs.f_ffree,
		(s64)fs.f_files - (s64)fs.f_ffree,
		fs.f_fsid.__val[0], fs.f_fsid.__val[1],
		(s64)fs.f_namelen);
	return 0;
}
