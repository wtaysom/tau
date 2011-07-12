/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

/*
chdir, fchdir
chmod
chown
close
creat
dup
fcntl
flock
fstat
fstatfs
fstatvfs
fsync
ftruncate
ioctl
link
lseek
lstat
mkdir
mmap
open
openat
pread
read
readlink
readlinkat
rmdir
stat
statfs
statvfs
symlink
sync
truncate
unlink
write

Directory Library Calls
closedir
opendir
readdir
rewinddir
scandir
seekdir
telldir

Asynchronous I/O
aio_cancel
aio_error
aio_fsync
aio_return
aio_suspend
aio_write

Extended Attributes
fgetxattr
flistxattr
fremovexattr
fsetxattr
getxattr
listxattr
removexattr
setxattr
*/

// XXX: May want to track or print working directory in error

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <style.h>
#include <debug.h>
#include <eprintf.h>

/* Can't include fcalls.h so have to define these again */
#define HERE_DCL	const char *file, const char *function, int line
#define HERE_ARG	file, function, line

extern bool Verbose;	/* Print each called file system operations */

bool Fatal = TRUE;	/* Exit on unexpected errors */
bool StackTrace = TRUE;

static void verbose (HERE_DCL, const char *fmt, ...)
{
	va_list args;

	if (!Verbose) return;
	if (getprogname() != NULL) {
		printf("%s ", getprogname());
	}
	printf("%s:%s<%d> ", HERE_ARG);
	if (fmt) {
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
	}
	printf("\n");
}

static s64 check (HERE_DCL, s64 rc, int err, s64 rtn, const char *fmt, ...)
{
	int	lasterr = errno;
	va_list args;

	if (rc == -1) {
		if (err && (lasterr == err)) return rc;
	} else {
		if (!err && (rc == rtn)) return rc;
	}
	fflush(stdout);
	fprintf(stderr, "ERROR ");
	if (getprogname() != NULL) {
		fprintf(stderr, "%s ", getprogname());
	}
	fprintf(stderr, "%s:%s<%d> ", HERE_ARG);
	va_start(args, fmt);
	if (rc == -1) {
		if (err) {
			fprintf(stderr, "expected: %s<%d>\n\t",
				strerror(err), err);
		}
		if (fmt) {
			vfprintf(stderr, fmt, args);

			if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':') {
				fprintf(stderr, " %s<%d>",
					strerror(lasterr), lasterr);
			}
		}
	} else {
		if (fmt) {
			vfprintf(stderr, fmt, args);

			fprintf(stderr, "expected %lld but returned %lld",
				rtn, rc);
		}
	}
	va_end(args);
	fprintf(stderr, "\n");
	if (StackTrace) stacktrace_err();
	if (Fatal) exit(2); /* conventional value for failed execution */
	return rc;
}

#if 0
static void vpr_error (HERE_DCL, int expected, const char *fmt, va_list ap)
{
	int	lasterr = errno;

	fflush(stdout);
	fprintf(stderr, "Fatal ");
	if (getprogname() != NULL) {
		fprintf(stderr, "%s ", getprogname());
	}
	fprintf(stderr, "%s:%s<%d> ", HERE_ARG);
	if (expected) fprintf(stderr, "expected=%d ", expected);
	if (fmt) {
		vfprintf(stderr, fmt, ap);

		if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':') {
			fprintf(stderr, " %s<%d>", strerror(lasterr), lasterr);
		}
	}
	fprintf(stderr, "\n");
	if (StackTrace) stacktrace_err();
	if (Fatal) exit(2); /* conventional value for failed execution */
}

int is_expected (HERE_DCL, int rc, int expected, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	if (expected == 0) {
		if (rc == -1) vpr_error(HERE_ARG, expected, fmt, args);
	} else {
		if (rc == 0) {
			vpr_error(HERE_ARG, expected, fmt, args);
		} else if (rc == -1) {
			if (errno != expected) {
				vpr_error(HERE_ARG, expected, fmt, args);
			}
		}
	}
	va_end(args);
	return rc;
}
#endif

int chdirp (HERE_DCL, int err, const char *path)
{
	int rc = chdir(path);
	verbose(HERE_ARG, "chdir(%s)", path);
	return check(HERE_ARG, rc, err, 0, "chdir %s:", path);
}

int closedirp (HERE_DCL, int err, const char *path)
{
	int rc = chdir(path);
	verbose(HERE_ARG, "chdir(%s)", path);
	return check(HERE_ARG, rc, err, 0, "chdir %s:", path);
}

int creatp (HERE_DCL, int err, const char *path, mode_t mode)
{
	int fd = creat(path, mode);
	verbose(HERE_ARG, "creat(%s, 0%o)", path, mode);
	return check(HERE_ARG, fd, err, fd, "creat(%s, 0%o):",
		path, mode);
}

s64 lseekp (HERE_DCL, int err, int fd, off_t offset, int whence, size_t seek)
{
	s64 rc = lseek(fd, offset, whence);
	verbose(HERE_ARG, "lseek(%d, %zu, %d)", fd, offset, whence);
	return check(HERE_ARG, rc, err, seek, "lseek(%d, %zu, %d):",
		fd, offset, whence);
}

int mkdirp (HERE_DCL, int err, const char *path, mode_t mode)
{
	int rc = mkdir(path, mode);
	verbose(HERE_ARG, "mkdir(%s, 0%o)", path, mode);
	return check(HERE_ARG, rc, err, 0, "mkdir(%s, 0%o):", path, mode);
}

int openp (HERE_DCL, int err, const char *path, int flags, mode_t mode)
{
	int fd = open(path, flags, mode);
	verbose(HERE_ARG, "open(%s, 0x%x, 0%o)", path, flags, mode);
	return check(HERE_ARG, fd, err, fd, "open(%s, 0x%x, 0%o):",
		path, flags, mode);
}

int closep (HERE_DCL, int err, int fd)
{
	int rc = close(fd);
	verbose(HERE_ARG, "close(%d)", fd);
	return check(HERE_ARG, rc, err, 0, "close %d:", fd);
}

s64 readp (HERE_DCL, int err, int fd, void *buf, size_t nbyte, size_t size)
{
	s64 rc = read(fd, buf, nbyte);
	verbose(HERE_ARG, "read(%d, %p, %zu)", fd, buf, nbyte);
	return check(HERE_ARG, rc, err, size, "read(%d, %p, %zu):",
		fd, buf, nbyte);
}

int rmdirp (HERE_DCL, int err, const char *path)
{
	int rc = rmdir(path);
	verbose(HERE_ARG, "rmdir(%s)", path);
	return check(HERE_ARG, rc, err, 0, "rmdir(%s):", path);
}

s64 writep (HERE_DCL, int err, int fd, void *buf, size_t nbyte, size_t size)
{
	s64 rc = write(fd, buf, nbyte);
	verbose(HERE_ARG, "write(%d, %p, %zu)", fd, buf, nbyte);
	return check(HERE_ARG, rc, err, nbyte, "write(%d, %p, %zu):",
		fd, buf, nbyte);
}
