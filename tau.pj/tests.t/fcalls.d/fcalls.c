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

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <style.h>
#include <debug.h>
#include <eprintf.h>

#define HERE_PRM	const char *file, const char *function, int line
#define HERE_ARG	file, function, line

static bool Fatal = TRUE;	/* Exit on unexpected errors */
static bool Stacktrace = TRUE;

/* pr_error: print error message and exit if Fatal flag is set */
void pr_error (HERE_PRM, const char *fmt, ...)
{
	va_list args;

	fflush(stdout);
	fprintf(stderr, "Fatal ");
	if (getprogname() != NULL) {
		fprintf(stderr, "%s ", getprogname());
	}
	fprintf(stderr, "%s:%s<%d> ", HERE_ARG);
	if (fmt) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);

		if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':') {
			fprintf(stderr, " %s<%d>", strerror(errno), errno);
		}
	}
	fprintf(stderr, "\n");
	if (Stacktrace) stacktrace_err();
	if (Fatal) exit(2); /* conventional value for failed execution */
}

int chdirq (HERE_PRM, const char *path)
{
	int rc = chdir(path);
	if (rc == -1) pr_error(HERE_ARG, "chdir %s:", path);
	return rc;
}

int chdirE (HERE_PRM, int err, const char *path)
{
	int rc = chdir(path);
	if (rc == 0) pr_error(HERE_ARG, "chdir %s:", path);
	if (rc == -1) {
		if (errno != err) {
			pr_error(HERE_ARG, "expected %d chdir %s:", err, path);
		}
	}
	return rc;
}
