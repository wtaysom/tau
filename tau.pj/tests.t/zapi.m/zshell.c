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
#include <mylib.h>
#include <eprintf.h>
#include <utf.h>
#include <debug.h>
#include <cmd.h>

enum {	MY_TASK   = 17,
	MAX_DEPTH = 3,
	MAX_KEYS  = 20,
	BLK_SHIFT = 12,
	BLK_SIZE  = 1 << BLK_SHIFT };

LONG	My_id = 0;
Key_t	Root_key;
Key_t	Current;

static int gregp (int argc, char *argv[])
{
	STATUS	rc;
	Key_t	key;
	char	*file;
	char	buf[10];
	NINT	bytesRead;

	if (argc <= 1) return 0;

	file = argv[1];

	rc = zOpen(Current, MY_TASK, zNSPACE_LONG|zMODE_UTF8|zMODE_LINK,
			file, zRR_READ_ACCESS, &key);
	if (rc) {
		weprintf("ERROR: zOpen of \"%s\" = %d", file, rc);
		return 0;
	}
	rc = zRead(key, 0, 0, sizeof(buf), buf, &bytesRead);
	if (rc) {
		weprintf("ERROR: zRead of \"%s\" = %d", file, rc);
		return 0;
	}
	zClose(key);
	return 0;
}

static int cdp (int argc, char *argv[])
{
	STATUS	rc;
	Key_t	key;
	char	*dir;

	if (argc <= 1) return 0;

	dir = argv[1];

	rc = zOpen(Current, MY_TASK, zNSPACE_LONG|zMODE_UTF8|zMODE_LINK,
			dir, zRR_SCAN_ACCESS, &key);
	if (rc) {
		weprintf("ERROR: zOpen of \"%s\" = %d", dir, rc);
		return 0;
	}
	if (Current != Root_key) zClose(Current);
	Current = key;
	return 0;
}

static QUAD enumerate (Key_t dir, QUAD cookie, zInfo_s *info)
{
	STATUS	rc;
	QUAD	ret_cookie;

	rc = zEnumerate(dir, cookie, zNTYPE_FILE, 0/*match*/,
			zGET_STD_INFO | zGET_NAME, sizeof(zInfo_s),
			zINFO_VERSION_A, info, &ret_cookie);
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

enum { zVALID_GET_INFO_MASK = (zGET_STD_INFO | zGET_NAME | zGET_ALL_NAMES |
			zGET_PRIMARY_NAMESPACE | zGET_TIMES_IN_SECS |
			zGET_TIMES_IN_MICROS | zGET_IDS | zGET_STORAGE_USED |
			zGET_BLOCK_SIZE | zGET_COUNTS | zGET_DATA_STREAM_INFO |
			zGET_DELETED_INFO | zGET_MAC_METADATA | zGET_UNIX_METADATA |
			zGET_VOLUME_INFO | zGET_VOL_SALVAGE_INFO | zGET_POOL_INFO | 
			zGET_DIR_QUOTA | zGET_INH_RIGHTS_MASK | zGET_ALL_TRUSTEES )};

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

	rc = zGetInfo(Current, zVALID_GET_INFO_MASK,
				sizeof(info), zINFO_VERSION_C, &info);
	if (rc != zOK) {
		weprintf("ERROR: zGetInfo = %d", rc);
	}
	pr_info_c( &info);
	return 0;
}

static int quotap (int argc, char *argv[])
{
	zInfoC_s	info;
	int		rc;

	if (argc < 2) {
		weprintf("have to supply a quota");
		return 0;
	}
	info.dirQuota.quota = strtoll(argv[1], NULL, 16);
	rc = zModifyInfo(Current, zNILXID, zMOD_DIR_QUOTA,
				sizeof(info), zINFO_VERSION_C, &info);
	if (rc != zOK) {
		weprintf("ERROR: zModifyInfo = %d", rc);
	}
	return 0;
}

static int undop (int argc, char *argv[])
{
	zInfoC_s	info;
	int		rc;

	info.std.fileAttributes = 0;
	info.std.fileAttributesModMask = zFA_IS_LINK;
	rc = zModifyInfo(Current, zNILXID, zMOD_FILE_ATTRIBUTES,
				sizeof(info), zINFO_VERSION_C, &info);
	if (rc != zOK) {
		weprintf("ERROR: zModifyInfo = %d", rc);
	}
	return 0;
}

static int mklinkp (int argc, char *argv[])
{
	zInfoC_s	info;
	int		rc;

	info.std.fileAttributes = zFA_IS_LINK;
	info.std.fileAttributesModMask = zFA_IS_LINK;
	rc = zModifyInfo(Current, zNILXID, zMOD_FILE_ATTRIBUTES,
				sizeof(info), zINFO_VERSION_C, &info);
	if (rc != zOK) {
		weprintf("ERROR: zModifyInfo = %d", rc);
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

static void pu (const unicode_t *f)
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

	zClose(Root_key);
	rc = zRootKey(0, &Root_key);
	if (rc) {
		eprintf("ERROR: zRootKey = %d", rc);
	}
	rc = zNewConnection(Root_key, fdn(argv[1]));
	if (rc) {
pu(fdn(argv[1]));
		weprintf("ERROR: zNewConnection = %d", rc);
	}
	Current = Root_key;
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
	Key_t	key;
	int	i;
	int	rc;

	if (argc < 2) {
		printf("Not enough args: zOpentst <path>\n");
		return 0;
	}
	file = argv[1];
	startTimer();
	for (i = 0; i < 1000000; i++) {
		rc = zOpen(Current, MY_TASK, zNSPACE_LONG|zMODE_UTF8|zMODE_LINK,
			file, zRR_READ_ACCESS, &key);
		if (rc) {
			weprintf("zOpen = %d", rc);
			return rc;
		}
		zClose(key);
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
	Key_t	key;
	NINT	bytes_read;
	int	i;
	int	rc;

	if (argc < 2) {
		printf("Not enough args: zOpentst <path>\n");
		return 0;
	}
	file = argv[1];
	rc = zOpen(Current, MY_TASK, zNSPACE_LONG|zMODE_UTF8|zMODE_LINK,
			file, zRR_READ_ACCESS, &key);
	if (rc) {
		weprintf("zOpen = %d", rc);
		return rc;
	}
	startTimer();
	for (i = 0; i < 100000; i++) {
		zRead(key, 0, 0, 1, buf, &bytes_read);
	}
	stopTimer();
	prTimer();
	printf("\n");
	zClose(key);
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
		rc = zWrite(key, 0, offset, x, buf, &written);
		if (rc) {
			printf("zWrite = %d", rc);
			return;
		}
	}
}

static int bigp (int argc, char *argv[])
{
	char	*name;
	u64	size;
	Key_t	key;
	int	rc;

	if (argc < 3) {
		printf("big <path> <size>\n");
		return 0;
	}
	name = argv[1];
	size = strtoll(argv[2], NULL, 0);
	rc = zCreate(Current, MY_TASK, zNILXID, zNSPACE_UNIX|zMODE_UTF8,
                        name, zFILE_REGULAR, 0, 0,
			zRR_READ_ACCESS|zRR_WRITE_ACCESS, &key);
	if (rc) return rc;

	fill(key, size);

	zClose(key);
	return 0;
}

static Key_t	Key[MAX_KEYS];

static Key_t lookup_key (int id)
{
	if (id >= MAX_KEYS) return 0;

	return Key[id];
}

static int save_key (Key_t key)
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
	Key_t	key;
	int	id;
	int	rc;

	if (argc < 2) {
		printf("open <path>\n");
		return 0;
	}
	name = argv[1];
	rc = zOpen(Current, MY_TASK, zNSPACE_UNIX|zMODE_UTF8,
                        name, zRR_READ_ACCESS|zRR_WRITE_ACCESS, &key);
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
	Key_t	key;
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
	zClose(key);
	return 0;
}

static int write_block (Key_t key, int block)
{
	char	buf[BLK_SIZE];
	u64	offset;
	NINT	written;
	int	rc;

	randomize(buf, sizeof(buf), block);
	offset = block * sizeof(buf);
	rc = zWrite(key, 0, offset, sizeof(buf), buf, &written);
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
	Key_t	f_key;
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
	Key_t	key;
	int	rc;

	rc = zCreate(Current, MY_TASK, zNILXID, zNSPACE_UNIX|zMODE_UTF8,
			name, zFILE_REGULAR, 0, 0,
			zRR_READ_ACCESS|zRR_WRITE_ACCESS, &key);
	if (rc) {
		printf("Couldn't create %s error %d\n",
			name, rc);
		return rc;
	}
	fill(key, size);
	zClose(key);

	return 0;
}
	
static int open_file (char *name, Key_t *key)
{
	int	rc;

	rc = zOpen(Current, MY_TASK, zNSPACE_UNIX|zMODE_UTF8,
			name, zRR_READ_ACCESS|zRR_WRITE_ACCESS, key);
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

	rc = zDelete(Current, zNILXID, zNSPACE_UNIX|zMODE_UTF8, name, 0 , 0);
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
		zClose(Files[i].f_key);
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

static void init (void)
{
//	extern void smszapi_init(void);
	STATUS	rc;

//	smszapi_init();

	rc = zRootKey(My_id, &Root_key);
	if (rc) {
		eprintf("ERROR: zRootKey = %d", rc);
	}
	Current = Root_key;
}

int main (int argc, char *argv[])
{
	setprogname(argv[0]);

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

	init();

	return shell();
}
