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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tau/msg.h>
#include <tau/msg_util.h>
#include <tau/switchboard.h>

#include <eprintf.h>
#include <debug.h>
#include <cmd.h>

#include <dirfs.h>

#include <cmd_hdr.h>

static unsigned	Dir_key;

static int crdir (char *path, void *ignore)
{
	fs_msg_s	msg;
	int		rc;

	msg.m_method = DIR_CREATE;
	strncpy(msg.cr_path, path, sizeof(msg.cr_path));
	rc = call_tau(Dir_key, &msg);
	if (rc) return failure("crdir:call_tau", rc);
	return rc;
}

static int crp (int argc, char *argv[])
{
	if (!Did_mkfs) {
		printf("Need to do mkfs first\n");
		return 0;
	}
	return map_argv(argc, argv, crdir, NULL);
}

static int prtreep (int argc, char *argv[])
{
	msg_s	msg;
	int	rc;

	msg.m_method = DIR_PRTREE;
	rc = call_tau(Dir_key, &msg);
	if (rc) return failure("prtree:call", rc);
	return rc;
}

static int prbackupp (int argc, char *argv[])
{
	msg_s	msg;
	int	rc;

	msg.m_method = DIR_PRBACKUP;
	rc = call_tau(Dir_key, &msg);
	if (rc) return failure("prbackup:call_tau", rc);
	return rc;
}

static void pr_dir (dir_s *dir)
{
	unint	i;
	dname_s	*dn;

	printf("%8lld\n", dir->dr_id);
	for (i = 0; i < DIR_SZ; i++) {
		dn = &dir->dr_n[i];
		if (dn->dn_num) {
			printf("  %8lld %.*s\n", dn->dn_num, DIR_SZ, dn->dn_name);
		}
	}
}

static int read_dir (u64 ino, dir_s *dir)
{
	fs_msg_s	msg;
	int		rc;
	unsigned	data_key;

	msg.m_method   = DIR_READ;
	msg.rd_dir_num = ino;
	rc = call_tau(Dir_key, &msg);
	if (rc) return failure("read_dir:call_tau", rc);

	data_key = msg.q.q_passed_key;

	rc = read_data_tau(data_key, sizeof(*dir), dir, 0);
	if (rc) return failure("read_dir:read_data", rc);

	destroy_key_tau(data_key);

	return rc;
}

static int rdp (int argc, char *argv[])
{
	static dir_s		dir;
	unsigned	i;
	int		rc;
	u64		ino;

	for (i = 1; i < argc; i++) {
		ino = atoll(argv[i]);
		if (ino) {
			rc = read_dir(ino, &dir);
			if (rc) return rc;
			pr_dir( &dir);
		}
	}
	return 0;
}

static void pr_indent (unint indent)
{
	while (indent--) {
		printf("\t");
	}
}

static void pr_node (u64 id, unint indent)
{
	unint	i;
	dir_s	dir;
	dname_s	*dn;
	int	rc;

	rc = read_dir(id, &dir);
	if (rc) {
		printf("%8lld not found because %d\n", id, rc);
		return;
	}
prmem("dir", &dir, sizeof(dir));
	for (i = 0; i < DIR_SZ; i++) {
		dn = &dir.dr_n[i];
		if (dn->dn_num) {
			pr_indent(indent);
			printf("  %8lld %.*s\n", dn->dn_num,
					DIR_SZ, dn->dn_name);
			pr_node(dn->dn_num, indent + 1);
		}
	}
prmem("dir", &dir, sizeof(dir));
}

static int lsp (int argc, char *argv[])
{
	printf("<root>\n");
	pr_node(ROOT, 0);
	return 0;
}

void init_dir (void)
{
	int		rc;

	rc = sw_local("dir_cmd", &Dir_key);
	if (rc) failure("sw_local", rc);

	CMD(cr,		"<path> # create a dir entry");
	CMD(prtree,	"# have dirfs print full directory tree");
	CMD(prbackup,	"# have dirfs print backup tree");
	CMD(rd,		"[inode num]+ # get and print a dir");
	CMD(ls,		"# list dir tree");
}
