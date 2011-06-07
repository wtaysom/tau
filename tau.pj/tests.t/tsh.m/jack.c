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

#define _XOPEN_SOURCE 600
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <zParams.h>
#include <zPublics.h>
#include <zError.h>

#include <style.h>
#include <tau/msg.h>
#include <tau/switchboard.h>
#include <tau/msg_util.h>
#include <mylib.h>
#include <eprintf.h>
#include <utf.h>
#include <debug.h>
#include <cmd.h>

#include <jill.h>

//////////////////////////////////////////////////////////////////////////////

LONG	My_id = 0;

ki_t	Jill_key;
ki_t	Jill_root;
ki_t	Current;

void jack_close (ki_t key)
{
	destroy_key_tau(key);
}

void jack_init (u32 task_id, u32 name_space)
{
	jmsg_s	m;
	int	rc;

	if (init_msg_tau(getprogname())) {
		exit(2);
	}
	sw_register(getprogname());

	rc = sw_any("jill", &Jill_key);
	if (rc) fatal("sw_request %d", rc);

	m.m_method      = JILL_SW_REG;
	m.ji_id         = My_id;
	m.ji_task       = task_id;
	m.ji_name_space = name_space;
	rc = call_tau(Jill_key, &m);
	if (rc) fatal("call_tau %d", rc);

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
		warn("call_tau %d", rc);
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
		warn("putdata_tau \"%s\" = %d", path, rc);
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
		warn("putdata_tau \"%s\" = %d", path, rc);
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
		warn("getdata_tau %d", rc);
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
		warn("putdata_tau %d", rc);
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
		warn("getdata_tau %d", rc);
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
		warn("getdata_tau %d", rc);
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
		warn("putdata_tau \"%s\" = %d", path, rc);
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
		warn("getdata_tau %d", rc);
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
		warn("putdata_tau %d", rc);
		return rc;
	}
	*bytes_written = m.jo_length;
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

enum {	MY_TASK   = 17,
	MAX_DEPTH = 3,
	MAX_KEYS  = 20,
	BLK_SHIFT = 12,
	BLK_SIZE  = 1 << BLK_SHIFT };

static int gregp (int argc, char *argv[])
{
	STATUS	rc;
	ki_t	key;
	char	*file;
	char	buf[10];
	unint	bytesRead;

	if (argc <= 1) return 0;

	file = argv[1];

	rc = jack_open(file, zRR_READ_ACCESS, &key);
	if (rc) {
		warn("jack_open of \"%s\" = %d", file, rc);
		return 0;
	}
	rc = jack_read(key, sizeof(buf), buf, 0, &bytesRead);
	if (rc) {
		warn("jack_read of \"%s\" = %d", file, rc);
		return 0;
	}
	jack_close(key);
	return 0;
}

static int cdp (int argc, char *argv[])
{
	STATUS	rc;
	ki_t	key;
	char	*dir;

	if (argc <= 1) return 0;

	dir = argv[1];

	rc = jack_open(dir, zRR_SCAN_ACCESS, &key);
	if (rc) {
		warn("jack_open \"%s\" = %d", dir, rc);
		return 0;
	}
	if (Current != Jill_root) jack_close(Current);
	Current = key;
	return 0;
}

static QUAD enumerate (Key_t dir, QUAD cookie, zInfo_s *info)
{
	STATUS	rc;
	QUAD	ret_cookie;

	rc = jack_enumerate(dir, cookie, info, &ret_cookie);
	if (rc) return 0;
	return ret_cookie;
}

static int lsp (int argc, char *argv[])
{
	zInfo_s		info;
	unicode_t	*uname;
	utf8_t		name[1024];
	QUAD		cookie = 0;

	for (;;) {
		cookie = enumerate(Current, cookie, &info);
		if (!cookie) break;
		uname = zINFO_NAME( &info);
		uni2utf(uname, name, sizeof(name));
		printf("%s\n", name);
	}
	return 0;
}

static char *pr_guid (GUID_t *guid)
{
	static char	g[40];

	sprintf(g, "%.8x.%.4x.%.4x.%.2x.%.2x.%.2x%.2x%.2x%.2x%.2x%.2x",
		guid->timeLow, guid->timeMid, guid->timeHighAndVersion,
		guid->clockSeqHighAndReserved, guid->clockSeqLow,
		guid->node[0], guid->node[1], guid->node[2],
		guid->node[3], guid->node[4], guid->node[5]);

	return g;
}

typedef struct File_attribute_s {
	LONG	fa_attribute;
	char	*fa_name;
} File_attribute_s;

File_attribute_s	Fa[] = {
	{ zFA_READ_ONLY,		"read only" },
	{ zFA_HIDDEN,			"hidden" },
	{ zFA_SYSTEM,			"system" },
	{ zFA_EXECUTE,			"execute" },
	{ zFA_SUBDIRECTORY,		"subdirectory" },
	{ zFA_ARCHIVE,			"archive" },
	{ zFA_SHAREABLE,		"shareable" },
	{ zFA_SMODE_BITS,		"smode" },
	{ zFA_NO_SUBALLOC,		"no suballoc" },
	{ zFA_TRANSACTION,		"transaction" },
	{ zFA_NOT_VIRTUAL_FILE,		"not virtual" },
	{ zFA_IMMEDIATE_PURGE,		"purge immediate" },
	{ zFA_RENAME_INHIBIT,		"rename inhibit" },
	{ zFA_DELETE_INHIBIT,		"delete inhibit" },
	{ zFA_COPY_INHIBIT,		"copy inhibit" },
	{ zFA_IS_ADMIN_LINK,		"admin link" },
	{ zFA_IS_LINK,			"sym link" },
	{ zFA_REMOTE_DATA_INHIBIT,	"remote inhibit" },
	{ zFA_COMPRESS_FILE_IMMEDIATELY,"compress immediately" },
	{ zFA_DATA_STREAM_IS_COMPRESSED,"compressed" },
	{ zFA_DO_NOT_COMPRESS_FILE,	"don't compress" },
//	{ zFA_HARDLINK,			"hardlink" },
	{ zFA_CANT_COMPRESS_DATA_STREAM,"can't compress" },
	{ zFA_ATTR_ARCHIVE,		"attr archive" },
	{ zFA_VOLATILE,			"volatile" },
	{ 0, NULL }
};

static char *lookup_attrib (LONG attrib)
{
	File_attribute_s	*fa;

	for (fa = Fa; fa->fa_attribute; fa++) {
		if (fa->fa_attribute & attrib) return fa->fa_name;
	}
	return "<unknown>";
}

static void pr_attrib (LONG attrib)
{
	LONG	a;
	int	first = TRUE;

	printf("\tfileAttributes = %x:",  attrib);
	for (a = 0x80000000; a ; a >>= 1) {
		if (a & attrib) {
			if (!first) {
				printf("|");
			} else {
				first = FALSE;
			}
			printf("%s", lookup_attrib(a));
		}
	}
	printf("\n");
}

static void pr_std_info (Standard_s *std)
{
	printf("std info:\n");
	printf("\tzid            = %llx\n", std->zid);
	printf("\tdataStreamZid  = %llx\n", std->dataStreamZid);
	printf("\tparentZid      = %llx\n", std->parentZid);
	printf("\tlogicalEOF     = %llx\n", std->logicalEOF);
	printf("\tvolumeID       = %s\n",   pr_guid( &std->volumeID));
	printf("\tfileType       = %x\n",   std->fileType);
	pr_attrib(std->fileAttributes);
}

static void pr_quota (zInfoC_s *info)
{
	printf("dir quota:\n");
	printf("\tquota      = %llx\n", info->dirQuota.quota);
	printf("\tusedAmount = %llx\n", info->dirQuota.usedAmount);
}

static void pr_info_c (zInfoC_s *info)
{
	if (info->retMask & zGET_STD_INFO) pr_std_info( &info->std);
	if (info->retMask & zGET_DIR_QUOTA) pr_quota(info);
}

static int statp (int argc, char *argv[])
{
	zInfoC_s	info;
	int		rc;

	rc = jack_get_info(Current, zVALID_GET_INFO_MASK, &info);
	if (rc != zOK) {
		warn("jack_get_info %d", rc);
	}
	pr_info_c( &info);
	return 0;
}

static int quotap (int argc, char *argv[])
{
	zInfoC_s	info;
	int		rc;

	if (argc < 2) {
		warn("have to supply a quota");
		return 0;
	}
	info.dirQuota.quota = strtoll(argv[1], NULL, 16);
	rc = jack_modify_info(Current, zMOD_DIR_QUOTA, &info);
	if (rc != zOK) {
		warn("jack_modify_info %d", rc);
	}
	return 0;
}

static int undop (int argc, char *argv[])
{
	zInfoC_s	info;
	int		rc;

	info.std.fileAttributes = 0;
	info.std.fileAttributesModMask = zFA_IS_LINK;
	rc = jack_modify_info(Current, zMOD_FILE_ATTRIBUTES, &info);
	if (rc != zOK) {
		warn("jack_modify_info %d", rc);
	}
	return 0;
}

static int mklinkp (int argc, char *argv[])
{
	zInfoC_s	info;
	int		rc;

	info.std.fileAttributes = zFA_IS_LINK;
	info.std.fileAttributesModMask = zFA_IS_LINK;
	rc = jack_modify_info(Current, zMOD_FILE_ATTRIBUTES, &info);
	if (rc != zOK) {
		warn("jack_modify_info %d", rc);
	}
	return 0;
}

static unicode_t *fdn (const char *name)
{
	static unicode_t	Fdn[1000];
	const char	*c = name;
	unicode_t	*f = Fdn;

	while ((*f++ = *c++))
		;
	return Fdn;
}

void pu (const unicode_t *f)
{
	while (*f) {
		printf("%c", *f);
		++f;
	}
	printf("\n");
}

static int loginp (int argc, char *argv[])
{
	int	rc;

	jack_close(Jill_root);
	rc = jack_root_key(0, 13, zNSPACE_UNIX|zMODE_UTF8|zMODE_LINK, &Jill_root);
	if (rc) {
		warn("jack_root_key %d", rc);
	}
	rc = jack_new_connection(Jill_root, fdn(argv[1]));
	if (rc) {
		warn("jack_new_connection \"%s\" %d", argv[1], rc);
	}
	Current = Jill_root;
	return 0;
}

static int resetp (int argc, char *argv[])
{
	resetTimer();
	return 0;
}

static int zOpentstp (int argc, char *argv[])
{
	char	*file;
	ki_t	key;
	int	i;
	int	rc;

	if (argc < 2) {
		printf("Not enough args: zOpentst <path>\n");
		return 0;
	}
	file = argv[1];
	startTimer();
	for (i = 0; i < 1000000; i++) {
		rc = jack_open(file, zRR_READ_ACCESS, &key);
		if (rc) {
			weprintf("jack_open = %d", rc);
			return rc;
		}
		jack_close(key);
	}
	stopTimer();
	prTimer();
	printf("\n");
	return 0;
}

static int opentstp (int argc, char *argv[])
{
	char	*file;
	int	fd;
	int	i;

	if (argc < 2) {
		printf("Not enough args: opentst <path>\n");
		return 0;
	}
	file = argv[1];
	startTimer();
	for (i = 0; i < 1000000; i++) {
		fd = open(file, O_RDONLY);
		if (fd == -1) {
			weprintf("open:");
			return fd;
		}
		close(fd);
	}
	stopTimer();
	prTimer();
	printf("\n");
	return 0;
}

enum { BSZ = 1024 };

static int zReadtstp (int argc, char *argv[])
{
	char	buf[BSZ];
	char	*file;
	ki_t	key;
	NINT	bytes_read;
	int	i;
	int	rc;

	if (argc < 2) {
		printf("Not enough args: zOpentst <path>\n");
		return 0;
	}
	file = argv[1];
	rc = jack_open(file, zRR_READ_ACCESS, &key);
	if (rc) {
		warn("jack_open %d", rc);
		return rc;
	}
	startTimer();
	for (i = 0; i < 100000; i++) {
		jack_read(key, 1, buf, 0, &bytes_read);
	}
	stopTimer();
	prTimer();
	printf("\n");
	jack_close(key);
	return 0;
}

static int readtstp (int argc, char *argv[])
{
	char	buf[BSZ];
	char	*file;
	int	fd;
	int	i;

	if (argc < 2) {
		printf("Not enough args: opentst <path>\n");
		return 0;
	}
	file = argv[1];
	fd = open(file, O_RDONLY);
	if (fd == -1) {
		weprintf("open:");
		return fd;
	}
	startTimer();
	for (i = 0; i < 100000; i++) {
		pread(fd, buf, 1, 0);
	}
	stopTimer();
	prTimer();
	printf("\n");
	close(fd);
	return 0;
}

static void randomize (char *buf, unsigned size, u64 offset)
{
	unsigned	i;

	srandom(offset);
	for (i = 0; i < size; i++) {
		*buf++ = random();
	}
}

static void fill (Key_t key, u64 size)
{
	char	buf[BLK_SIZE];
	u64	offset;
	NINT	written;
	u64	r;
	u64	x;
	int	rc;

	x = sizeof(buf);
	offset = 0;
	for (r = size; r; r -= x, offset += x) {
		if (r < x) x = r;

		randomize(buf, x, offset);
		rc = jack_write(key, x, buf, offset, &written);
		if (rc) {
			warn("jack_write %d", rc);
			return;
		}
	}
}

static int bigp (int argc, char *argv[])
{
	char	*name;
	u64	size;
	ki_t	key;
	int	rc;

	if (argc < 3) {
		printf("big <path> <size>\n");
		return 0;
	}
	name = argv[1];
	size = strtoll(argv[2], NULL, 0);
	rc = jack_create(name, zFILE_REGULAR, 0, 0,
			zRR_READ_ACCESS|zRR_WRITE_ACCESS, &key);
	if (rc) return rc;

	fill(key, size);

	jack_close(key);
	return 0;
}

static ki_t	Key[MAX_KEYS];

static Key_t lookup_key (int id)
{
	if (id >= MAX_KEYS) return 0;

	return Key[id];
}

static int save_key (ki_t key)
{
	int	i;

	for (i = 1; i < MAX_KEYS; i++) {
		if (Key[i] == 0) {
			Key[i] = key;
			return i;
		}
	}
	return 0;
}

static void free_key (int id)
{
	if (id >= MAX_KEYS) return;

	Key[id] = 0;
}

static int openp (int argc, char *argv[])
{
	char	*name;
	ki_t	key;
	int	id;
	int	rc;

	if (argc < 2) {
		printf("open <path>\n");
		return 0;
	}
	name = argv[1];
	rc = jack_open(name, zRR_READ_ACCESS|zRR_WRITE_ACCESS, &key);
	if (rc) return rc;

	id = save_key(key);
	if (!id) {
		printf("too many open files %d\n", MAX_KEYS - 1);
		return 0;
	}
	printf("open id for %s is %d\n", name, id);

	return 0;
}

static int closep (int argc, char *argv[])
{
	ki_t	key;
	int	id;

	if (argc < 2) {
		printf("close <id>\n");
		return 0;
	}
	id = atoi(argv[1]);
	key = lookup_key(id);
	if (!key) {
		printf("Key not found for %d\n", id);
		return 0;
	}
	free_key(id);
	jack_close(key);
	return 0;
}

static int write_block (ki_t key, int block)
{
	char	buf[BLK_SIZE];
	u64	offset;
	NINT	written;
	int	rc;

	randomize(buf, sizeof(buf), block);
	offset = block * sizeof(buf);
	rc = jack_write(key, sizeof(buf), buf, offset, &written);
	if (rc) {
		printf("zWrite = %d\n", rc);
		return 0;
	}
	return 0;
}

static int fiddlep (int argc, char *argv[])
{
	Key_t	key;
	int	id;
	int	block;
	int	nblks = 1;
	int	i;

	if (argc < 3) {
		printf("fiddle <open id> <block num> [<num blocks>]\n");
		return 0;
	}
	if (argc > 1) {
		id = atoi(argv[1]);
	}
	if (argc > 2) {
		block = atoi(argv[2]);
	}
	if (argc > 3) {
		nblks = atoi(argv[3]);
	}
	key = lookup_key(id);
	if (!key) {
		printf("Invalid open id\n");
		return 0;
	}
	for (i = 0; i < nblks; i++, block++) {
		write_block(key, block);
	}
	return 0;
}

//======================================================================

enum { MAX_NAME = 16 };

typedef struct file_s {
	ki_t	f_key;
} file_s;

unint	Num_files;
unint	Num_blocks;
file_s	*Files;

static char *make_name (char *name, int i)
{
	snprintf(name, MAX_NAME-1, "%u", i);
	return name;
}

static int create_file (char *name, u64 size)
{
	ki_t	key;
	int	rc;

	rc = jack_create(name, zFILE_REGULAR, 0, 0,
			zRR_READ_ACCESS|zRR_WRITE_ACCESS, &key);
	if (rc) {
		printf("Couldn't create %s error %d\n",
			name, rc);
		return rc;
	}
	fill(key, size);
	jack_close(key);

	return 0;
}

static int open_file (char *name, ki_t *key)
{
	int	rc;

	rc = jack_open(name, zRR_READ_ACCESS|zRR_WRITE_ACCESS, key);
	if (rc) {
		printf("Couldn't open %s error %d\n",
			name, rc);
		return rc;
	}
	return 0;
}

static int delete_file (char *name)
{
	int	rc;

	rc = jack_delete(name);
	if (rc) {
		printf("Couldn't delete %s error %d\n",
			name, rc);
		return rc;
	}
	return 0;
}

static int crfilesp (int argc, char *argv[])
{
	char	name[MAX_NAME];
	int	i;
	int	rc;

	if (Num_files) {
		printf("Already of %ld files\n", Num_files);
		return 0;
	}
	Num_files = 1;
	Num_blocks = 10;
	if (argc > 1) {
		Num_files = atoi(argv[1]);
	}
	if (argc > 2) {
		Num_blocks = atoi(argv[2]);
	}

	for (i = 0; i < Num_files; i++) {
		make_name(name, i);
		rc = create_file(name, Num_blocks << BLK_SHIFT);
		if (rc) {
			printf("Could only create %d files"
				" before getting error %d\n",
				i, rc);
			Num_files = i;
		}
	}
	return 0;
}

static int openfilesp (int argc, char *argv[])
{
	char	name[MAX_NAME];
	int	i;
	int	rc;

	Files = ezalloc(Num_files * sizeof(file_s));
	for (i = 0; i < Num_files; i++) {
		make_name(name, i);
		rc = open_file(name, &Files[i].f_key);
		if (rc) {
			printf("Couldn't open file %s error %d\n",
				name, rc);
		}
	}
	return 0;
}

static int playp (int argc, char *argv[])
{
	int	block = 7;
	int	nblks = 1;
	int	i;
	int	j;

	if (argc > 1) {
		block = atoi(argv[1]);
	}
	if (argc > 2) {
		nblks = atoi(argv[2]);
	}
	for (j = 0; j < nblks; j++) {
		for (i = 0; i < Num_files; i++) {
			write_block(Files[i].f_key, block+j);
		}
	}
	return 0;
}

static int closefilesp (int argc, char *argv[])
{
	int	i;

	for (i = 0; i < Num_files; i++) {
		jack_close(Files[i].f_key);
	}
	free(Files);
	return 0;
}

static int deletefilesp (int argc, char *argv[])
{
	char	name[MAX_NAME];
	int	i;
	int	rc;

	for (i = 0; i < Num_files; i++) {
		make_name(name, i);
		rc = delete_file(name);
		if (rc) {
			printf("Stopped deleting at %d because %d\n", i, rc);
			return rc;
		}
	}
	Num_files = 0;
	return 0;
}
//======================================================================


#define PR_SZ(_x_)	printf("sizeof(" # _x_ ")=%ld\n", (snint)sizeof(_x_))
static int sizesp (int argc, char *argv[])
{
	PR_SZ(zInfo_s);
	PR_SZ(zInfoB_s);
	PR_SZ(zInfoC_s);
	return 0;
}

int main (int argc, char *argv[])
{
	setprogname(argv[0]);

	jack_init(13, zNSPACE_LONG|zMODE_UTF8|zMODE_LINK);

	init_shell(NULL);

	CMD(cd,		"# change directory");
	CMD(ls,		"# list directory");
	CMD(stat,	"# statistics for file or directory");
	CMD(quota,	"# set quota on current directory");
	CMD(sizes,	"# print size of various structures");
	CMD(undo,	"# undo symbolic link");
	CMD(mklink,	"# turn on symbolic link (only for testing)");
	CMD(greg,	"# Greg's open/read test");
	CMD(login,	"# login <fdn>");
	CMD(reset,	"# reset timer sums");
	CMD(zOpentst,	"# zOpentst <path>");
	CMD(opentst,	"# opentst <path>");
	CMD(zReadtst,	"# zReadtst <path>");
	CMD(readtst,	"# readtst <path>");
	CMD(big,	"# big <size>");
	CMD(open,	"# open <path>");
	CMD(close,	"# close <open id>");
	CMD(fiddle,	"# fiddle <open id> <block num>");

	CMD(crfiles,	"# crfiles [<num> [<size>]]");
	CMD(openfiles,	"# openfiles");
	CMD(play,	"# play [<block num>]");
	CMD(closefiles,	"# closefiles");
	CMD(deletefiles,"# deletefiles");

	return shell();
}
