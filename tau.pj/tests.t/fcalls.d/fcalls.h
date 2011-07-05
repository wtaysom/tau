/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

#ifndef _FCALLS_H_
#define _FCALLS_H_ 1

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

#define HERE		MYFILE, __FUNCTION__, __LINE__
#define HERE_PRM	const char *file, const char *function, int line

#define chdir(_p)			chdirp(HERE, 0, _p)
#define chdirErr(_err, _p)		chdirp(HERE, _err, _p)
#define close(_fd)			closep(HERE, 0, _fd)
#define lseek(_fd, _o, _w)		lseekp(HERE, 0, _fd, _o, _w, _o)
#define lseekOff(_fd, _o, _w, _s)	lseekp(HERE, 0, _fd, _o, _w, _s)
#define open(_p, _f, _m)		openp(HERE, 0, _p, _f, _m)
#define read(_fd, _b, _n)		readp(HERE, 0, _fd, _b, _n, _n)
#define readSz(_fd, _b, _n, _sz)	readp(HERE, 0, _fd, _b, _n, _sz)
#define write(_fd, _b, _n)		writep(HERE, 0, _fd, _b, _n, _n)
#define writeSz(_fd, _b, _n, _sz)	writep(HERE, 0, _fd, _b, _n, _sz)

int chdirp(HERE_PRM, int expected, const char *path);
int closep(HERE_PRM, int expected, int fd);
s64 lseekp(HERE_PRM, int expected, int fd, off_t offset, int whence, off_t seek);
int openp(HERE_PRM, int expected, const char *path, int flags, int mode);
s64 readp(HERE_PRM, int expected, int fd, void *buf, size_t nbyte, size_t size);
s64 writep(HERE_PRM, int expected, int fd, void *buf, size_t nbyte, size_t size);


#endif
