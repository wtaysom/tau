/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* fcalls wraps the folling functions with verbose printing, error
 * checking, and location information to make it easier to track problems.
 *
 * chdir, fchdir
 * chmod
 * chown
 * close
 * creat
 * dup
 * fcntl
 * flock
 * fsync, fdatasync
 * ioctl
 * link
 * lseek
 * mkdir
 * mmap, munmap
 * open
 * openat
 * pread, pwrite
 * read, readlink, readlinkat
 * rmdir
 * stat, fstat, lstat
 * statfs, fstatfs
 * statvfs, fstatvfs
 * symlink
 * sync
 * truncate, ftrancate
 * unlink
 * write
 *
 * Directory Library Calls
 * closedir
 * opendir
 * readdir
 * rewinddir
 * scandir?
 * seekdir
 * telldir
 *
 * Asynchronous I/O
 * aio_cancel
 * aio_error
 * aio_fsync
 * aio_return
 * aio_suspend
 * aio_write
 *
 * Extended Attributes
 * fgetxattr
 * flistxattr
 * fremovexattr
 * fsetxattr
 * getxattr
 * listxattr
 * removexattr
 * setxattr
 */

/* TODO(taysom):
 * 1. May want to track or print working directory in error
 * 2. Do I want to add timers around system calls?
 */

#define _GNU_SOURCE  /* for dup3 */

#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <debug.h>
#include <eprintf.h>
#include <style.h>
#include <where.h>

#include "wrapper.h"

extern bool Verbose;     /* Print each called file system operations */

bool Fatal = TRUE;       /* Exit on unexpected errors */
bool StackTrace = TRUE;  /* Print a stack trace on failures */

/* PrVerbose used to optionally print arguments being used */
static void PrVerbose (Where_s w, const char *fmt, va_list ap) {
  if (!Verbose) return;
  if (getprogname() != NULL) {
    printf("%s ", getprogname());
  }
  printf("%s:%s<%d> ", w.file, w.fn, w.line);
  if (fmt) {
    vprintf(fmt, ap);
  }
  printf("\n");
}

/* vPrErrork prints the errors dectected in these routines.
 * It takes two formats so it can take args from the xxxxk
 * routine and from the routines that check for errors.
 */
static void vPrErrork (Where_s w, const char *fmt1, va_list ap,
                       const char *fmt2, ...) {
  int lasterr = errno;
  va_list args;

  fflush(stdout);
  fprintf(stderr, "ERROR ");
  if (getprogname() != NULL) {
    fprintf(stderr, "%s ", getprogname());
  }
  fprintf(stderr, "%s:%s<%d> ", w.file, w.fn, w.line);
  if (fmt2) {
    va_start(args, fmt2);
    vfprintf(stderr, fmt2, args);
    va_end(args);
  }
  if (fmt1) {
    vfprintf(stderr, fmt1, ap);
  }
  fprintf(stderr, ": %s<%d>\n", strerror(lasterr), lasterr);
  if (StackTrace) stacktrace_err();
  if (Fatal) exit(2); /* conventional value for failed execution */
}

/* Check checks the return code from system calls.
 * If expected_err is not zero, verifies that error was set.
 */
static s64 Check (Where_s w, s64 rc, int expected_err,
                  s64 rtn, const char *fmt, ...) {
  va_list args;

  if (Verbose && fmt) {
    va_start(args, fmt);
    PrVerbose(w, fmt, args);
    va_end(args);
  }
  if (rc == -1) {
    if (expected_err && (errno == expected_err)) return rc;
  } else {
    if (!expected_err && (rc == rtn)) return rc;
  }
  va_start(args, fmt);
  if (rc == -1) {
    if (expected_err) {
      vPrErrork(w, fmt, args, "expected: %s<%d>\n\t",
        strerror(expected_err), expected_err);
    } else {
      vPrErrork(w, fmt, args, NULL);
    }
  } else {
    vPrErrork(w, fmt, args,
        "expected %lld but returned %lld\n\t",
        rtn, rc);
  }
  va_end(args);
  return rc;
}

/* CheckPtr checks error returns for functions returning a pointer.
 */
static void *CheckPtr (Where_s w, void *p, bool is_null, const char *fmt, ...) {
  va_list args;

  if (Verbose && fmt) {
    va_start(args, fmt);
    PrVerbose(w, fmt, args);
    va_end(args);
  }
  if (is_null && !p) return p;
  if (!is_null && p) return p;
  va_start(args, fmt);
  vPrErrork(w, fmt, args, " %p ", p);
  va_end(args);
  return p;
}

int chdirk (Where_s w, int expected_err, const char *path) {
  int rc = chdir(path);
  return Check(w, rc, expected_err, 0, "chdir(%s)", path);
}

int fchdirk (Where_s w, int expected_err, int fd) {
  int rc = fchdir(fd);
  return Check(w, rc, expected_err, 0, "fchdir(%d)", fd);
}

int chmodk (Where_s w, int expected_err, const char *path, mode_t mode) {
  int rc = chmod(path, mode);
  return Check(w, rc, expected_err, 0, "chmod(%s, 0%o)", path, mode);
}

int fchmodk (Where_s w, int expected_err, int fd, mode_t mode) {
  int rc = fchmod(fd, mode);
  return Check(w, rc, expected_err, 0, "fchmod(%d, 0%o)", fd, mode);
}

int chownk (Where_s w, int expected_err, const char *path,
            uid_t owner, gid_t group) {
  int rc = chown(path, owner, group);
  return Check(w, rc, expected_err, 0, "chown(%s, %d, %d)",
               path, owner, group);
}

int fchownk (Where_s w, int expected_err, int fd,
            uid_t owner, gid_t group) {
  int rc = fchown(fd, owner, group);
  return Check(w, rc, expected_err, 0, "fchown(%d, %d, %d)",
               fd, owner, group);
}

int lchownk (Where_s w, int expected_err, const char *path,
            uid_t owner, gid_t group) {
  int rc = lchown(path, owner, group);
  return Check(w, rc, expected_err, 0, "lchown(%s, %d, %d)",
               path, owner, group);
}

int closek (Where_s w, int expected_err, int fd) {
  int rc = close(fd);
  return Check(w, rc, expected_err, 0, "close(%d)", fd);
}

int creatk (Where_s w, int expected_err, const char *path, mode_t mode) {
  int fd = creat(path, mode);
  return Check(w, fd, expected_err, fd, "creat(%s, 0%o)",
               path, mode);
}

int dupk (Where_s w, int expected_err, int oldfd) {
  int newfd = dup(oldfd);
  return Check(w, newfd, expected_err, newfd, "dup(%d)", oldfd);
}

int dup2k (Where_s w, int expected_err, int oldfd, int newfd) {
  int rc = dup2(oldfd, newfd);
  return Check(w, rc, expected_err, 0, "dup2(%d, %d)", oldfd, newfd);
}

int dup3k (Where_s w, int expected_err, int oldfd, int newfd, int flags) {
  int rc = dup3(oldfd, newfd, flags);
  return Check(w, rc, expected_err, 0, "dup3(%d, %d, %d)",
               oldfd, newfd, flags);
}

int fcntlk (Where_s w, int expected_err, int fd, int cmd, void *arg) {
  int rc = fcntl(fd, cmd, arg);
  return Check(w, rc, expected_err, 0, "fcntl(%d, %d, %p)", fd, cmd, arg);
}

int flockk (Where_s w, int expected_err, int fd, int operation) {
  int rc = flock(fd, operation);
  return Check(w, rc, expected_err, 0, "flock(%d, %d)", fd, operation);
}

int fsynck (Where_s w, int expected_err, int fd) {
  int rc = fsync(fd);
  return Check(w, rc, expected_err, 0, "fsync(%d)", fd);
}

int fdatasynck (Where_s w, int expected_err, int fd) {
  int rc = fdatasync(fd);
  return Check(w, rc, expected_err, 0, "fdatasync(%d)", fd);
}

int ioctlk (Where_s w, int expected_err, int fd, int request, void *arg) {
  int rc = ioctl(fd, request, arg);
  return Check(w, rc, expected_err, 0, "ioctl(%d, %d, %p)", fd, request, arg);
}

int linkk (Where_s w, int expected_err,
           const char *oldpath, const char *newpath) {
  int rc = link(oldpath, newpath);
  return Check(w, rc, expected_err, 0, "link(%s, %s)", oldpath, newpath);
}

s64 lseekk (Where_s w, int expected_err, int fd,
            s64 offset, int whence, s64 seek) {
  s64 rc = lseek(fd, offset, whence);
  return Check(w, rc, expected_err, seek, "lseek(%d, %lld, %d)",
               fd, offset, whence);
}

int mkdirk (Where_s w, int expected_err, const char *path, mode_t mode) {
  int rc = mkdir(path, mode);
  return Check(w, rc, expected_err, 0, "mkdir(%s, 0%o)", path, mode);
}

void *mmapk (Where_s w, bool is_null, void *addr, size_t length,
             int prot, int flags, int fd, off_t offset) {
  void *map = mmap(addr, length, prot, flags, fd, offset);
  return CheckPtr(w, map, is_null, "mmap(%p, %zd, 0%o, 0%o, %d, %zd)",
                  addr, length, prot, flags, fd, offset);
}

int munmapk (Where_s w, int expected_err, void *addr, size_t length) {
  int rc = munmap(addr, length);
  return Check(w, rc, expected_err, 0, "munmap(%p, %zd)", addr, length);
}

int openk (Where_s w, int expected_err,
           const char *path, int flags, mode_t mode) {
  int fd = open(path, flags, mode);
  return Check(w, fd, expected_err, fd, "open(%s, 0x%x, 0%o)",
               path, flags, mode);
}

int openatk (Where_s w, int expected_err, int dirfd,
           const char *path, int flags, mode_t mode) {
  int fd = openat(dirfd, path, flags, mode);
  return Check(w, fd, expected_err, fd, "openat(%d, %s, 0x%x, 0%o)",
               dirfd, path, flags, mode);
}

s64 preadk (Where_s w, int expected_err, int fd,
            void *buf, size_t nbyte, size_t size, s64 offset) {
  s64 rc = pread(fd, buf, nbyte, offset);
  return Check(w, rc, expected_err, size, "pread(%d, %p, %zu, %lls)",
               fd, buf, nbyte, offset);
}

s64 pwritek (Where_s w, int expected_err, int fd,
             void *buf, size_t nbyte, size_t size, s64 offset) {
  s64 rc = pwrite(fd, buf, nbyte, offset);
  return Check(w, rc, expected_err, size, "pwrite(%d, %p, %zu, %lls)",
               fd, buf, nbyte, offset);
}

s64 readk (Where_s w, int expected_err, int fd,
           void *buf, size_t nbyte, size_t size) {
  s64 rc = read(fd, buf, nbyte);
  return Check(w, rc, expected_err, size, "read(%d, %p, %zu)",
               fd, buf, nbyte);
}

s64 readlinkk (Where_s w, int expected_err, const char *path,
               void *buf, size_t nbyte, size_t size) {
  s64 rc = readlink(path, buf, nbyte);
  return Check(w, rc, expected_err, size, "readlink(%s, %p, %zu)",
               path, buf, nbyte);
}

s64 readlinkatk (Where_s w, int expected_err, int fd, const char *path,
               void *buf, size_t nbyte, size_t size) {
  s64 rc = readlinkat(fd, path, buf, nbyte);
  return Check(w, rc, expected_err, size, "readlinkat(%d, %s, %p, %zu)",
               fd, path, buf, nbyte);
}

int rmdirk (Where_s w, int expected_err, const char *path) {
  int rc = rmdir(path);
  return Check(w, rc, expected_err, 0, "rmdir(%s)", path);
}

int statk (Where_s w, int expected_err, const char *path, struct stat *buf) {
  int rc = stat(path, buf);
  return Check(w, rc, expected_err, 0, "stat(%s, %p)", path, buf);
}

int fstatk (Where_s w, int expected_err, int fd, struct stat *buf) {
  int rc = fstat(fd, buf);
  return Check(w, rc, expected_err, 0, "fstat(%d, %p)", fd, buf);
}

int lstatk (Where_s w, int expected_err, const char *path, struct stat *buf) {
  int rc = lstat(path, buf);
  return Check(w, rc, expected_err, 0, "lstat(%s, %p)", path, buf);
}

int statfsk (Where_s w, int expected_err, const char *path,
             struct statfs *buf) {
  int rc = statfs(path, buf);
  return Check(w, rc, expected_err, 0, "statfs(%s, %p)", path, buf);
}

int fstatfsk (Where_s w, int expected_err, int fd, struct statfs *buf) {
  int rc = fstatfs(fd, buf);
  return Check(w, rc, expected_err, 0, "fstatfs(%d, %p)", fd, buf);
}

int statvfsk (Where_s w, int expected_err, const char *path,
              struct statvfs *buf) {
  int rc = statvfs(path, buf);
  return Check(w, rc, expected_err, 0, "statvfs(%s, %p)", path, buf);
}

int fstatvfsk (Where_s w, int expected_err, int fd, struct statvfs *buf) {
  int rc = fstatvfs(fd, buf);
  return Check(w, rc, expected_err, 0, "fstatvfs(%d, %p)", fd, buf);
}

int symlinkk (Where_s w, int expected_err,
           const char *oldpath, const char *newpath) {
  int rc = symlink(oldpath, newpath);
  return Check(w, rc, expected_err, 0, "symlink(%s, %s)", oldpath, newpath);
}

void synck (Where_s w) {
  sync();
  Check(w, 0, 0, 0, "sync()");
}

int truncatek (Where_s w, int expected_err, const char *path, s64 length) {
  int rc = truncate(path, length);
  return Check(w, rc, expected_err, 0, "truncate(%s, %lld)", path, length);
}

int ftruncatek (Where_s w, int expected_err, int fd, s64 length) {
  int rc = ftruncate(fd, length);
  return Check(w, rc, expected_err, 0, "ftruncate(%d, %lld)", fd, length);
}

int unlinkk (Where_s w, int expected_err, const char *path) {
  int rc = unlink(path);
  return Check(w, rc, expected_err, 0, "unlink(%s)", path);
}

s64 writek (Where_s w, int expected_err, int fd,
            void *buf, size_t nbyte, size_t size) {
  s64 rc = write(fd, buf, nbyte);
  return Check(w, rc, expected_err, nbyte, "write(%d, %p, %zu)",
               fd, buf, nbyte);
}

int closedirk (Where_s w, int expected_err, DIR *dir) {
  int rc = closedir(dir);
  return Check(w, rc, expected_err, 0, "closedir(%p)", dir);
}

DIR *opendirk (Where_s w, bool is_null, const char *path) {
  DIR *dir = opendir(path);
  return CheckPtr(w, dir, is_null, "opendir(%s)", path);
}

struct dirent *readdirk (Where_s w, DIR *dir) {
  struct dirent *ent = readdir(dir);
  /* Check only for verbose because NULL indicates EOF */
  Check(w, 0, 0, 0, "readdir(%p)", dir);
  return ent;
}

int readdir_rk (Where_s w, int expected_err, DIR *dir, struct dirent *entry,
                struct dirent **result) {
  s64 rc = readdir_r(dir, entry, result);
  return Check(w, rc, expected_err, 0, "readdir_r(%p, %p, %p)",
               dir, entry, result);
}

void rewinddirk (Where_s w, DIR *dir) {
  rewinddir(dir);
  Check(w, 0, 0, 0, "rewinddir(%p)", dir);
}

void seekdirk (Where_s w, DIR *dir, long offset) {
  seekdir(dir, offset);
  Check(w, 0, 0, 0, "seekdir(%p, %ld)", dir, offset);
}

long telldirk (Where_s w, int expected_err, DIR *dir) {
  long offset = telldir(dir);
  return Check(w, offset, expected_err, offset, "teldir(%p)", dir);
}
