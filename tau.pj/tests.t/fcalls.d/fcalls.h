/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _FCALLS_H_
#define _FCALLS_H_ 1

#include <eprintf.h>
#include <style.h>
#include <where.h>
#include <wrapper.h>

/* Define macros to make it easier to test file system related system calls.
 * The goal is to wrap each system call so that when an error occurs, it is
 * easy to find the case that fails.
 *
 * Yes, this is a shameless abuse of macros but it makes the tests easier
 * to read and write.
 *
 * Conventions:
 *   1. A macro is defined for each system call with the same name
 *   2. A wrapper function is defined that is the name of system
 *      call followed by a 'k' which stands for check.
 */

#define chdir(p)           chdirk     (WHERE, 0, p)
#define close(fd)          closek     (WHERE, 0, fd)
#define closedir(dir)      closedirk  (WHERE, 0, dir)
#define creat(p, m)        creatk     (WHERE, 0, p, m)
#define lseek(fd, o, w)    lseekk     (WHERE, 0, fd, o, w, o)
#define mkdir(p, m)        mkdirk     (WHERE, 0, p, m)
#define open(p, f, m)      openk      (WHERE, 0, p, f, m)
#define opendir(p)         opendirk   (WHERE, FALSE, p)
#define read(fd, b, n)     readk      (WHERE, 0, fd, b, n, n)
#define readdir(d)         readdirk   (WHERE, d)
#define readdir_r(d, e, r) readdir_rk (WHERE, 0, d, e, r)
#define rmdir(p)           rmdirk     (WHERE, 0, p)
#define seekdir(d, o)      seekdirk   (WHERE, d, o)
#define telldir(d)         telldirk   (WHERE, 0, d)
#define write(fd, b, n)    writek     (WHERE, 0, fd, b, n, n)

#define chdirErr(err, p)        chdirk(WHERE, err, p)
#define closeErr(err, fd)       closek(WHERE, err, fd)
#define lseekErr(err, fd, o, w) lseekk(WHERE, err, fd, o, w, o)
#define mkdirErr(err, p, m)     mkdirk(WHERE, err, p, m)

#define lseekCheckOffset(fd, o, w, s) lseekk(WHERE, 0, fd, o, w, s)
#define readCheckSize(fd, b, n, sz)   readk (WHERE, 0, fd, b, n, sz)
#define writeCheckSize(fd, b, n, sz)  writek(WHERE, 0, fd, b, n, sz)

extern snint BlockSize;  /* Block size and buffer size */
extern s64 SzBigFile;    /* Size of the "Big File" in bytes */

extern bool Fatal;       /* Exit on unexpected errors */
extern bool StackTrace;  /* Print a stack trace on error */

extern char BigFile[];
extern char EmptyFile[];
extern char OneFile[];


#endif  /* _FCALLS_H_ */
