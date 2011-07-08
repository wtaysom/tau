/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

#ifndef _FCALLS_H_
#define _FCALLS_H_ 1

#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef _SYTLE_H_
#include <style.h>
#endif

#ifndef _EPRINTF_H_
#include <eprintf.h>
#endif

/*
 * Define macros to make it easier to test file system related system calls.
 * The main goal, is to wrapper each system call so that it when an error
 * occurs, it is easy to track back the case that fails.
 *
 * Yes, this is a shameless abuse of macros but it makes the tests easier
 * to read and write.
 */

#define HERE_PRM	MYFILE, __FUNCTION__, __LINE__
#define HERE_DCL	const char *file, const char *function, int line
#define HERE_ARG	file, function, line

#define chdir(_p)			chdirp(HERE_PRM, 0, _p)
#define close(_fd)			closep(HERE_PRM, 0, _fd)
#define creat(_p, _m)			creatp(HERE_PRM, 0, _p, _m)
#define lseek(_fd, _o, _w)		lseekp(HERE_PRM, 0, _fd, _o, _w, _o)
#define mkdir(_p, _m)			mkdirp(HERE_PRM, 0, _p, _m)
#define open(_p, _f, _m)		openp(HERE_PRM, 0, _p, _f, _m)
#define read(_fd, _b, _n)		readp(HERE_PRM, 0, _fd, _b, _n, _n)
#define write(_fd, _b, _n)		writep(HERE_PRM, 0, _fd, _b, _n, _n)

#define chdirErr(_err, _p)		chdirp(HERE_PRM, _err, _p)
#define closeErr(_err, _fd)		closep(HERE_PRM, _err, _fd)
#define lseekErr(_err, _fd, _o, _w)	lseekp(HERE_PRM, _err, _fd, _o, _w, _o)
#define mkdirErr(_err, _p, _m)		mkdirp(HERE_PRM, _err, _p, _m)

#define lseekOff(_fd, _o, _w, _s)	lseekp(HERE_PRM, 0, _fd, _o, _w, _s)
#define readSz(_fd, _b, _n, _sz)	readp(HERE_PRM, 0, _fd, _b, _n, _sz)
#define writeSz(_fd, _b, _n, _sz)	writep(HERE_PRM, 0, _fd, _b, _n, _sz)

int chdirp(HERE_DCL, int expected, const char *path);
int closep(HERE_DCL, int expected, int fd);
int creatp(HERE_DCL, int expected, const char *path, int mode);
s64 lseekp(HERE_DCL, int expected, int fd, off_t offset, int whence, off_t seek);
int mkdirp(HERE_DCL, int expected, const char *path, mode_t mode);
int openp(HERE_DCL, int expected, const char *path, int flags, int mode);
s64 readp(HERE_DCL, int expected, int fd, void *buf, size_t nbyte, size_t size);
s64 writep(HERE_DCL, int expected, int fd, void *buf, size_t nbyte, size_t size);

extern unint	BlockSize;	/* Block size and buffer sizes */
extern s64	SzBigFile;	/* Size of the "Big File" in bytes */

extern bool Fatal;	/* Exit on unexpected errors */
extern bool StackTrace;	/* Print a stack trace on error */

extern char BigFile[];
extern char NilFile[];
extern char OneFile[];


#endif
