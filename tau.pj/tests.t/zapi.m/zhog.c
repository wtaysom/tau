#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include <zParams.h>
#include <zPublics.h>
#include <zError.h>

#include <style.h>
#include <eprintf.h>
#include <debug.h>

LONG	MyConnectionId = 0;
Key_t	RootKey;
Key_t	VolKey;

enum {	KILO = 1024ULL,
	MEGA = KILO * KILO,
	GIGA = MEGA * KILO,
	TERA = GIGA * KILO,
	PETA = TERA * KILO };

enum {	MY_TASK = 17,
	DEFAULT_SIZE = 17 * TERA };

bool Zero_fill = FALSE;

static void init (char *vol)
{
	STATUS	rc;

	rc = zRootKey(MyConnectionId, &RootKey);
	if (rc != zOK)
	{
		fatal("zRootKey = %d", rc);
		return;
	}
	rc = zOpen(RootKey, MY_TASK, zNSPACE_LONG|zMODE_UTF8, vol,
				zRR_SCAN_ACCESS, &VolKey);
	if (rc != zOK)
	{
		fatal("zOpen of volume \"%s\" = %d", vol, rc);
		return;
	}
}

static Key_t create_file (char *name)
{
	Key_t	key;
	int	rc;

	rc = zCreate(VolKey, MY_TASK, zNILXID, zNSPACE_UNIX|zMODE_UTF8,
			name, zFILE_REGULAR, 0, 0,
			zRR_READ_ACCESS|zRR_WRITE_ACCESS, &key);
	if (rc) {
		fatal("Couldn't create %s error %d\n",
			name, rc);
		return 0;
	}
	return key;
}

static void set_size (Key_t key, u64 size)
{
	int	rc;
	int	flags = zSETSIZE_NON_SPARSE_FILE;

	if (!Zero_fill) {
		flags |= zSETSIZE_NO_ZERO_FILL;
	}

	rc = zSetEOF(key, 0, size, flags);
	if (rc) fatal("zSetEOF %d", rc);
}

static void fibonacci (Key_t key, u64 size)
{
	u64	x = 1;
	u64	y = 1;
	u64	z;

	while (x < size) {
		printf(" %lld\n", x);
		set_size(key, x);
		z = x + y;
		y = x;
		x = z;
	}
}

static u64 size (const char *arg, u64 scale)
{
	u64	x;

	x = strtoull(arg, NULL, 0);
	return x * scale;
}

static void usage (void)
{
	printf("usage: %s [-zf] [-v <volume] [-{kmgtp} <num>]\n"
		"\tz - zero the data\n"
		"\tf - use a fibonacci sequence of expansions\n"
		"\tv - volume\n"
		"\tk - number of kilobytes to consume on disk\n"
		"\tm - number of megabytes to consume on disk\n"
		"\tg - number of gigabytes to consume on disk\n"
		"\tt - number of terabytes to consume on disk\n"
		"\tp - number of petabytes to consume on disk\n"
		"\tuses powers of two\n",
		getprogname());
	exit(2);
}

int main (int argc, char *argv[])
{
	char	filename[50];
	char	*vol = "BIG:/";
	u64	x = 0;
	bool	fib = FALSE;
	uuid_t	guid;
	Key_t	key;
	int	c;

	setprogname(argv[0]);

	while ((c = getopt(argc, argv, "zfv:k:m:g:t:p:?")) != -1) {
		switch (c) {
		case 'z':	Zero_fill = TRUE;	break;
		case 'f':	fib = TRUE;		break;
		case 'v':	vol = optarg;		break;
		case 'k':	x = size(optarg, KILO);	break;
		case 'm':	x = size(optarg, MEGA);	break;
		case 'g':	x = size(optarg, GIGA);	break;
		case 't':	x = size(optarg, TERA);	break;
		case 'p':	x = size(optarg, PETA);	break;
		case '?':
		default:	usage();		break;
		}
	}
	if (!x) x = DEFAULT_SIZE;

	init(vol);

	uuid_generate(guid);
	uuid_unparse(guid, filename);

	key = create_file(filename);

	if (fib) {
		fibonacci(key, x);
	} else {
		set_size(key, x);
	}
	return 0;
}
