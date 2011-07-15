/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

#ifndef _FCALLS_H_
#define _FCALLS_H_ 1

#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef _DIRENT_H
#include <dirent.h>
#endif

#ifndef _SYTLE_H_
#include <style.h>
#endif

#ifndef _WHERE_H_
#include <where.h>
#endif

#ifndef _EPRINTF_H_
#include <eprintf.h>
#endif

/* Define macros to make it easier to test file system related system calls.
 * The main goal, is to wrapper each system call so that it when an error
 * occurs, it is easy to track back the case that fails.
 *
 * Yes, this is a shameless abuse of macros but it makes the tests easier
 * to read and write.
 *
 * Conventions:
 *   1. A macro is defined for each system call with the same name
 *   2. A wrapper function is defined that is the name of system
 *      call followed by a 'k' which stands for check.
 */

#define chdir(p)        chdirk   (WHERE, 0, p)
#define close(fd)       closek   (WHERE, 0, fd)
#define closedir(dir)   closedirk(WHERE, 0, dir)
#define creat(p, m)     creatk   (WHERE, 0, p, m)
#define lseek(fd, o, w) lseekk   (WHERE, 0, fd, o, w, o)
#define mkdir(p, m)     mkdirk   (WHERE, 0, p, m)
#define open(p, f, m)   openk    (WHERE, 0, p, f, m)
#define opendir(p)      opendirk (WHERE, FALSE, p)
#define read(fd, b, n)  readk    (WHERE, 0, fd, b, n, n)
#define readdir(d)      readdirk (WHERE, d)
#define rmdir(p)        rmdirk   (WHERE, 0, p)
#define write(fd, b, n) writek   (WHERE, 0, fd, b, n, n)

#define chdirErr(err, p)        chdirk(WHERE, err, p)
#define closeErr(err, fd)       closek(WHERE, err, fd)
#define lseekErr(err, fd, o, w) lseekk(WHERE, err, fd, o, w, o)
#define mkdirErr(err, p, m)     mkdirk(WHERE, err, p, m)

#define lseekOff(fd, o, w, s) lseekk(WHERE, 0, fd, o, w, s)
#define readSz(fd, b, n, sz)  readk (WHERE, 0, fd, b, n, sz)
#define writeSz(fd, b, n, sz) writek(WHERE, 0, fd, b, n, sz)

int closek(Where_s w, int expected, int fd);
int closedirk(Where_s w, int expected, DIR *dir);
int creatk(Where_s w, int expected, const char *path, int mode);
s64 lseekk(Where_s w, int expected, int fd, s64 offset, int whence, s64 seek);
int openk (Where_s w, int expected, const char *path, int flags, int mode);
s64 readk (Where_s w, int expected, int fd, void *buf,
           size_t nbyte, size_t size);
s64 writek(Where_s w, int expected, int fd, void *buf,
           size_t nbyte, size_t size);
int  chdirk(Where_s where, int expected, const char *path);
int  mkdirk(Where_s w, int expected, const char *path, mode_t mode);
DIR *opendirk(Where_s w, bool is_null, const char *path);
int  rmdirk(Where_s w, int expected, const char *path);

struct dirent *readdirk(Where_s w, DIR *dir);

extern snint BlockSize;  /* Block size and buffer size */
extern s64 SzBigFile;    /* Size of the "Big File" in bytes */

extern bool Fatal;       /* Exit on unexpected errors */
extern bool StackTrace;  /* Print a stack trace on error */

extern char BigFile[];
extern char EmptyFile[];
extern char OneFile[];


#endif
