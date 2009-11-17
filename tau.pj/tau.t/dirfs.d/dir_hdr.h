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

#ifndef _DIR_HDR_H_
#define _DIR_HDR_H_ 1

#ifndef _DIRFS_H_
#include <dirfs.h>
#endif

enum { SHARE_PEER, SHARE_ADD, SHARE_DEL, SHARE_OPS };
enum { PEER_LOOKUP, PEER_ADDDIR, PEER_NEWDIR, PEER_PRINT, PEER_READDIR,
	BACKUP_ADDDIR, BACKUP_NEWDIR, BACKUP_PRINT, PEER_OPS };

int failure (char *err, int rc);
void prompt (char *s);

int init_cmd   (void);
int init_hello (void);
int init_fs    (void);
int init_sage  (void);

u64  dir_lookup   (u64 parent, char *name);
void dir_add      (u64 parent, u64 child, char *name);
u64  dir_new      (void);
void dir_print    (u64 id, unint indent);
void backup_add   (u64 parent, u64 child, char *name);
void backup_init  (u64 id);
void backup_print (u64 id, unint indent);

void cr_dir (const char *path);
void pr_tree   (void);
void pr_backup (void);
void mkfs_dir (u64 total_shares, u64 my_shares, u64 my_backup);
void upshares (u64 mask);
struct dir_s *read_dir (u64 id);

void peer_broadcast (void *m);

void crdir_cmd (void *m);
void readdir_cmd (void *msg);

unsigned remote   (u64 id);
unsigned backup   (u64 id);
int      is_local (u64 id);

#endif
