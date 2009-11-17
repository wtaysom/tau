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

#ifndef _DIRFS_H_
#define _DIRFS_H_

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

/*
 * Inodes are divide between shares by inode % MAX_SHARES
 * - or -
 * Do I divide up by parent directory?
 */

enum { DIR_CREATE, DIR_PRTREE, DIR_PRBACKUP, DIR_READ, DIR_OPS };
enum { FS_MAKE, FS_OPS };

enum {	SHARE_SHIFT	= 2,
	MAX_SHARES	= (1<<SHARE_SHIFT),
	SHARE_MASK	= MAX_SHARES - 1,
	SUPER		= 1,
	ROOT		= 2,
	NAME_SZ		= 32 - sizeof(u64),
	PATH_SZ		= 64 - sizeof(u64),
	DIR_SZ		= 16,
	EOS		= '\0',
	SLASH		= '/'};

typedef struct fs_msg_s {
	sys_s	q;
	union {
		METHOD_S;
		body_u  b;

		struct {
			METHOD_S;
			u64	mk_total_shares;
			u64	mk_my_shares;	/* A mask of shares */
			u64	mk_my_backup;	/* Mask of shares I backup */
		};

		struct {
			METHOD_S;
			char	cr_path[PATH_SZ];
		};

		struct {
			METHOD_S;
			u64	rd_dir_num;
		};

		struct {
			METHOD_S;
			u64	dir_parent_id;
			u64	dir_id;
			char	dir_indent;
			char	dir_name[NAME_SZ];
		};
	};
} fs_msg_s;


typedef struct dname_s {
	u64	dn_num;
	char	dn_name[NAME_SZ];
} dname_s;

typedef struct dir_s {
	u64		dr_id;	// Should blow this one away
	dname_s		dr_n[DIR_SZ];
} dir_s;

typedef struct super_s {
	u64		sp_id;
	u32		sp_num_nodes;
	u32		sp_my_num;

	u32		sp_backup_num;
	u32		sp_total_shares;

	u64		sp_my_shares;
	u64		sp_my_backup;
	void		*sp_dev;
} super_s;

#endif
