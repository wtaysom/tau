#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#include <zParams.h>
#include <zPublics.h>
#include <zError.h>

#include <style.h>
#include <mystdlib.h>
#include <eprintf.h>
#include <utf.h>
#include <debug.h>


enum { MY_TASK = 17, MAX_NAME = 8 };

LONG	MyConnectionId = 0;
Key_t	RootKey;
Key_t	VolKey;

struct {
	u64	num_opens;
	u64	num_dirs;
	u64	num_files;
	u64	num_deleted;
	u64	num_read;
	u64	num_written;
} Inst;

typedef struct arg_s {
	char		from[256];
	char		to[256];
	unsigned	width;
	unsigned	depth;
	unsigned	seed;
} arg_s;

int urand_r (unsigned max, arg_s *arg)
{
	return max ? (rand_r( &arg->seed) % max) : 0;
}

void pr_inst (void)
{
	printf("opens   = %10llu\n", Inst.num_opens);
	printf("created = %10llu\n", Inst.num_dirs+Inst.num_files);
	printf("dirs    = %10llu\n", Inst.num_dirs);
	printf("files   = %10llu\n", Inst.num_files);
	printf("deleted = %10llu\n", Inst.num_deleted);
	printf("read    = %10llu\n", Inst.num_read);
	printf("written = %10llu\n", Inst.num_written);
}

void clear_inst (void)
{
	zero(Inst);
}

void prompt (char *p)
{
	printf("%s$ ", p);
	getchar();
}

void gen_name (char *c, arg_s *arg)
{
	unsigned	i;
	static char file_name_char[] =  "abcdefghijklmnopqrstuvwxyz"
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					"_0123456789";

	for (i = 0; i < MAX_NAME - 1; i++) {
		*c++ = file_name_char[urand_r(sizeof(file_name_char)-1, arg)];
	}
	*c = '\0';
}

void init (char *vol)
{
	STATUS	rc;

	rc = zRootKey(MyConnectionId, &RootKey);
	if (rc != zOK)
	{
		eprintf("ERROR: zRootKey = %d\n", rc);
		return;
	}
	rc = zOpen(RootKey, MY_TASK, zNSPACE_LONG|zMODE_UTF8, vol,
				zRR_SCAN_ACCESS, &VolKey);
	if (rc != zOK)
	{
		eprintf("ERROR: zOpen of volume \"%s\" = %d\n", vol, rc);
		return;
	}
}

Key_t open_file (Key_t parent, char *file)
{
	STATUS	rc;
	Key_t	key;

	rc = zOpen(parent, MY_TASK, zNSPACE_UNIX|zMODE_UTF8, file,
				zRR_READ_ACCESS|zRR_WRITE_ACCESS, &key);
	if (rc != zOK)
	{
		eprintf("ERROR: zOpen of file \"%s\" = %d\n", file, rc);
		return 0;
	}
	++Inst.num_opens;
	return key;
}

Key_t open_dir (Key_t parent, char *dir)
{
	STATUS	rc;
	Key_t	key;

	rc = zOpen(parent, MY_TASK, zNSPACE_UNIX|zMODE_UTF8, dir,
				zRR_SCAN_ACCESS, &key);
	if (rc != zOK)
	{
		eprintf("ERROR: zOpen of directory \"%s\" = %d\n", dir, rc);
		return 0;
	}
	++Inst.num_opens;
	return key;
}

void fill (Key_t key, arg_s *arg)
{
	static char	rnd_char[] = "abcdefghijklmnopqrstuvwxyz\n";
	char		buf[4096];
	unsigned long	i, n;
	u64		offset;
	NINT		written;
	STATUS		rc;

	n = (urand_r(10, arg)+1) *(urand_r(10, arg)+1) * (urand_r(10, arg)+1);

	for (i = 0; i <  sizeof(buf); i++) {
		buf[i] = rnd_char[urand_r(sizeof(rnd_char)-1, arg)];
	}
	for (i = 0, offset = 0; i < n; i++, offset += sizeof(buf)) {
		rc = zWrite(key, 0, offset, sizeof(buf), buf, &written);
		if (rc) {
			eprintf("zWrite = %d\n", rc);
			return;
		}
		if (written != sizeof(buf)) {
			eprintf("Not all bytes written = %ld\n", written);
			return;
		}
	}
	Inst.num_written += offset;
}

Key_t cr_file (Key_t parent, char *name)
{
	Key_t	key;
	int	rc;

	rc = zCreate(parent, MY_TASK, zNILXID, zNSPACE_UNIX|zMODE_UTF8,
                        name, zFILE_REGULAR, 0, 0,
			zRR_READ_ACCESS|zRR_WRITE_ACCESS, &key);
	if (rc) {
		eprintf("cr_file zCreate file \"%s\" = %d\n", name, rc);
		return 0;
	}
	++Inst.num_files;
	return key;
}

int copy_file (Key_t from, Key_t to)
{
	char	buf[4096];
	u64	offset;
	NINT	n;
	int	rc;

	for (offset = 0;;) {
		rc = zRead(from, 0, offset, sizeof(buf), buf, &n);
		if (rc) {
			eprintf("copy_file zRead = %d", rc);
			return -1;
		}
		if (n == 0) break;

		if (n != sizeof(buf)) {
			eprintf("copy_file only read %d bytes\n", n);
			return -1;
		}

		Inst.num_read += n;
		rc = zWrite(to, 0, offset, sizeof(buf), buf, &n);
		if (rc) {
			eprintf("copy_file zWrite = %d", rc);
			return -1;
		}
		if (n != sizeof(buf)) {
			eprintf("copy_file only wrote %d bytes", n);
			return -1;
		}
		offset += n;
		Inst.num_written += n;
	}
	return 0;
}

Key_t cr_dir (Key_t parent, char *name)
{
	Key_t	key;
	int	rc;

	rc = zCreate(parent, MY_TASK, zNILXID, zNSPACE_UNIX|zMODE_UTF8,
                        name, zFILE_REGULAR, zFA_SUBDIRECTORY, 0,
			zRR_SCAN_ACCESS, &key);
	if (rc) {
		eprintf("cr_dir zCreate file \"%s\" = %d\n", name, rc);
		return 0;
	}
	++Inst.num_dirs;
	return key;
}

void delete (Key_t parent, char *name)
{
	int	rc;

	rc = zDelete(parent, zNILXID, zNSPACE_UNIX|zMODE_UTF8, name, 0 , 0);
	if (rc) {
		eprintf("delete \"%s\" = %d\n", name, rc);
		return;
	}
	++Inst.num_deleted;
}

Key_t rand_file (Key_t parent, arg_s *arg)
{
	char	name[MAX_NAME];
	Key_t	key;

	gen_name(name, arg);

	key = cr_file(parent, name);
	fill(key, arg);
	zClose(key);

	return 0;
}

Key_t rand_dir (Key_t parent, arg_s *arg)
{
	char	name[MAX_NAME];
	Key_t	key;

	gen_name(name, arg);

	key = cr_dir(parent, name);

	return key;
}

void mk_dir (Key_t parent, unsigned width, unsigned depth, arg_s *arg)
{
	Key_t		child;
	unsigned	i;

	for (i = 0; i < width; i++) {
		if (depth) {
			child = rand_dir(parent, arg);
			mk_dir(child, width, depth-1, arg);
			zClose(child);
		} else {
			rand_file(parent, arg);
		}
	}
}

void mk_tree (char *dir, int width, int depth, arg_s *arg)
{
	Key_t	parent;

	parent = cr_dir(VolKey, dir);
	mk_dir(parent, width, depth, arg);
	zClose(parent);
}

QUAD enumerate (Key_t dir, QUAD cookie, zInfo_s *info)
{
	STATUS	rc;
	QUAD	ret_cookie;

	rc = zEnumerate(dir, cookie, zNTYPE_FILE, 0/*match*/,
			zGET_STD_INFO | zGET_NAME, sizeof(zInfo_s),
			zINFO_VERSION_A, info, &ret_cookie);
	if (rc) return 0;
	return ret_cookie;
}

static void pr_indent (unsigned level)
{
	while (level--) {
		printf("  ");
	}
}

void walk_dir (Key_t parent, unsigned level)
{
	zInfo_s		info;
	Key_t		child;
	unicode_t	*uname;
	utf8_t		name[1024];
	QUAD		cookie = 0;

	for (;;) {
		cookie = enumerate(parent, cookie, &info);
		if (!cookie) break;
		uname = zINFO_NAME( &info);
		uni2utf(uname, name, sizeof(name));
		pr_indent(level); printf("%s\n", name);
		if (info.std.fileAttributes & zFA_SUBDIRECTORY) {
			child = open_dir(parent, name);
			walk_dir(child, level+1);
			zClose(child);
		}
	}
}

void walk_tree (char *dir)
{
	Key_t	parent;

	parent = open_dir(VolKey, dir);
	walk_dir(parent, 0);
	zClose(parent);
}

void copy_dir (Key_t p_from, Key_t p_to)
{
	zInfo_s		info;
	Key_t		c_from;
	Key_t		c_to;
	unicode_t	*uname;
	utf8_t		name[1024];
	QUAD		cookie = 0;

	for (;;) {
		cookie = enumerate(p_from, cookie, &info);
		if (!cookie) break;
		uname = zINFO_NAME( &info);
		uni2utf(uname, name, sizeof(name));
		if (info.std.fileAttributes & zFA_SUBDIRECTORY) {
			c_from = open_dir(p_from, name);
			c_to   = cr_dir(p_to, name);
			copy_dir(c_from, c_to);
		} else {
			c_from = open_file(p_from, name);
			c_to   = cr_file(p_to, name);
			copy_file(c_from, c_to);
		}
		zClose(c_from);
		zClose(c_to);
	}
}

void copy_tree (char *from, char *to)
{
	Key_t	p_from;
	Key_t	p_to;

	p_from = open_dir(VolKey, from);
	p_to   = cr_dir(VolKey, to);

	copy_dir(p_from, p_to);

	zClose(p_from);
	zClose(p_to);
}

void delete_dir (Key_t parent)
{
	zInfo_s		info;
	Key_t		child;
	unicode_t	*uname;
	utf8_t		name[1024];
	QUAD		cookie = 0;

	for (;;) {
		cookie = enumerate(parent, cookie, &info);
		if (!cookie) break;
		uname = zINFO_NAME( &info);
		uni2utf(uname, name, sizeof(name));
		if (info.std.fileAttributes & zFA_SUBDIRECTORY) {
			child = open_dir(parent, name);
			delete_dir(child);
			zClose(child);
		}
		delete(parent, name);
	}
}

void delete_tree (char *dir)
{
	Key_t	parent;

	parent = open_dir(VolKey, dir);
	delete_dir(parent);
	zClose(parent);
	delete(VolKey, dir);
}

void *ztree (void *arg)
{
	arg_s	*a = arg;

//printf("Begin mk_tree %s\n", a->from);
	mk_tree(a->from, a->width, a->depth, arg);
//	walk_tree(a->from);

//printf("Begin copy_tree %s\n", a->to);
	copy_tree(a->from, a->to);
//	walk_tree(a->to);
#if 1
//printf("Begin delete_tree %s\n", a->from);
	delete_tree(a->from);
//printf("Begin delete_tree %s\n", a->to);
	delete_tree(a->to);
#endif
	return NULL;
}

void start_threads (
	unsigned threads,
	unsigned width,
	unsigned depth,
	char	*from,
	char	*to)
{
	pthread_t	*thread;
	unsigned	i;
	int		rc;
	arg_s		*arg;
	arg_s		*a;

	thread = ezalloc(threads * sizeof(pthread_t));
	arg    = ezalloc(threads * sizeof(arg_s));
	for (i = 0, a = arg; i < threads; i++, a++) {
		sprintf(a->from, "%s_%d", from, i);
		sprintf(a->to,   "%s_%d", to, i);
		a->width = width;
		a->depth = depth;
		a->seed  = random();
		rc = pthread_create( &thread[i], NULL, ztree, a);
		if (rc) {
			eprintf("pthread_create %d\n", rc);
			break;
		}
	}
	while (i--) {
		pthread_join(thread[i], NULL);
	}
}

void usage (void)
{
	printf("Usage: %s [vol [iterations [threads [width [depth [from [to]]]]]]]\n",
			getprogname());
	exit(2);
}

int main (int argc, char *argv[])
{
	char		*vol = "V0:/";
	char		*from = "ztree";
	char		*to = "copy";
	unsigned	iterations = 1;
	unsigned	threads = 11;
	unsigned	width = 2;
	unsigned	depth = 3;
	unsigned	i;

	setprogname(argv[0]);
	if (argc > 1) {
		vol = argv[1];
	}
	if (argc > 2) {
		iterations = atoi(argv[2]);
	}
	if (argc > 3) {
		threads = atoi(argv[3]);
	}
	if (argc > 4) {
		width = atoi(argv[4]);
	}
	if (argc > 5) {
		depth = atoi(argv[5]);
	}
	if (argc > 6) {
		from = argv[6];
	}
	if (argc > 7) {
		to = argv[7];
	}
	if (!threads || !width || !depth || !from || !to) {
		usage();
	}

	init(vol);
	for (i = 0; i < iterations; i++) {
		srandom(137);

		startTimer();
		start_threads(threads, width, depth, from, to);
		stopTimer();

		pr_inst();

		prTimer();

		printf("\n");

		clear_inst();
	}
	zClose(VolKey);
	zClose(RootKey);

	return 0;
}
