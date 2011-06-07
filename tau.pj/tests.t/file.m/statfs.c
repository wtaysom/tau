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
/*
 * NAME
 *        statfs, fstatfs - get file system statistics
 *
 * SYNOPSIS
 *        #include <sys/vfs.h>
 *
 *        int statfs(const char *path, struct statfs *buf);
 *        int fstatfs(int fd, struct statfs *buf);
 *
 * DESCRIPTION
 *        statfs  returns  information  about  a mounted file system.  path is
 *        the path name of any file within the mounted filesystem.  buf is  a
 *        pointer to a statfs structure defined as follows:
 *
 *               struct statfs {
 *                  long    f_type;     // type of filesystem (see below)
 *                  long    f_bsize;    // optimal transfer block size
 *                  long    f_blocks;   // total data blocks in file system
 *                  long    f_bfree;    // free blocks in fs
 *                  long    f_bavail;   // free blocks avail to non-superuser
 *                  long    f_files;    // total file nodes in file system
 *                  long    f_ffree;    // free file nodes in fs
 *                  fsid_t  f_fsid;     // file system id
 *                  long    f_namelen;  // maximum length of filenames
 *                  long    f_spare[6]; // spare for later
 *               };
 *        Fields  that  are  undefined  for a particular file system are set
 *        to 0.  fstatfs returns the same information about an open  file
 *        referenced  by descriptor fd.
 *
 * RETURN VALUE
 *        On  success,  zero  is returned.  On error, -1 is returned, and
 *        errno is set appropriately.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/vfs.h>

#include <style.h>
#include <puny.h>
#include <eprintf.h>


typedef struct FsType_s {
	long	fs_magic;
	char	*fs_name;
} FsType_s;

FsType_s	FsType[] = {
	{ 0xADFF,	"Amiga Fast FS" },
	{ 0x00414A53,	"SGI's Extent FS" },
	{ 0x137D,	"EXT (not 2 or 3) FS" },
	{ 0xEF51,	"Old EXT-2 FS" },
	{ 0xEF53,	"EXT-2 FS" },
	{ 0xF995E849,	"OS/2 FS" },
	{ 0x9660,	"ISO-CD FS" },
	{ 0x137F,	"Orig. Minix FS" },
	{ 0x138F,	"30 char Minix FS" },
	{ 0x2468,	"Minix V2" },
	{ 0x2478,	"Minix V2, 30 char" },
	{ 0x4d44,	"MSDOS" },
	{ 0x564c,	"NCP" },
	{ 0x6969,	"NFS" },
	{ 0x9fa0,	"Proc" },
	{ 0x517B,	"SMB" },
	{ 0x012FF7B4,	"Xenix SysV" },
	{ 0x012FF7B5,	"SysV.4" },
	{ 0x012FF7B6,	"SysV.2" },
	{ 0x012FF7B7,	"Coherent CD-ROM" },
	{ 0x00011954,	"UFS Solaris" },
	{ 0x58465342,	"XFS (SGI or Berkeley?)" },
	{ 0x012FD16D,	"Xia FS" },
	{ 0,		NULL }
};

char *findFsName (long magic)
{
	FsType_s	*fs;

	for (fs = FsType; fs->fs_name; ++fs) {
		if (fs->fs_magic == magic) return fs->fs_name;
	}
	return "Unknown";
}

void prStatfs (char *name, struct statfs *sfs)
{
	printf("%s: type=%s: file system id=%d.%d\n",
		name, findFsName(sfs->f_type),
		sfs->f_fsid.__val[0], sfs->f_fsid.__val[1]);
	printf("\t%10lld: optimal transfer block size\n", (u64)sfs->f_bsize);
	printf("\t%10lld: total data blocks in file system\n", (u64)sfs->f_blocks);
	printf("\t%10lld: free blocks in fs\n", (u64)sfs->f_bfree);
	printf("\t%10lld: free blocks avail to non-superuser\n", (u64)sfs->f_bavail);
	printf("\t%10lld: total file nodes in file system\n", (u64)sfs->f_files);
	printf("\t%10lld: free file nodes in fs\n", (u64)sfs->f_ffree);
	printf("\t%10lld: maximum length of filenames\n", (u64)sfs->f_namelen);
}

int doStatfs (char *name)
{
	struct statfs	sfs;
	int		rc;

	rc = statfs(name, &sfs);
	if (rc == -1) {
		rc = errno;
		fprintf(stderr, "statfs:%s %s\n", name, strerror(errno));
		return rc;
	}
	prStatfs(name, &sfs);
	return 0;
}

int main (int argc, char *argv[])
{
	int	i;
	int	rc = 0;

	punyopt(argc, argv, NULL, NULL);
	if (argc == optind) {
		rc = doStatfs(Option.dir);
	} else for (i = optind; i < argc && !rc; ++i) {
		rc = doStatfs(argv[i]);
	}
	if (rc) fatal("doStatfs:");
	return 0;
}
