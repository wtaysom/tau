/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* fcalls wraps the folling functions with verbose printing, error
 * checking, and location information make it easier to track problems.
 *
 * chdir, fchdir
 * chmod
 * chown
 * close
 * creat
 * dup
 * fcntl
 * flock
 * fstat
 * fstatfs
 * fstatvfs
 * fsync
 * ftruncate
 * ioctl
 * link
 * lseek
 * lstat
 * mkdir
 * mmap
 * open
 * openat
 * pread
 * read
 * readlink
 * readlinkat
 * rmdir
 * stat
 * statfs
 * statvfs
 * symlink
 * sync
 * truncate
 * unlink
 * write
 *
 * Directory Library Calls
 * closedir
 * opendir
 * readdir
 * rewinddir
 * scandir
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

/* TODO(taysom): May want to track or print working directory in error */

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
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
#include <where.h>

extern bool Verbose; /* Print each called file system operations */

bool Fatal = TRUE; /* Exit on unexpected errors */
bool StackTrace = TRUE;

static void PrVerbose (Where_s w, const char *fmt, ...) {
  va_list args;

  if (!Verbose) return;
  if (getprogname() != NULL) {
    printf("%s ", getprogname());
  }
  printf("%s:%s<%d> ", w.file, w.fn, w.line);
  if (fmt) {
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
  }
  printf("\n");
}

static void vPrErrork (
  Where_s w,
  const char *fmt1,
  va_list ap,
  const char *fmt2,
  ...) {
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

    if (fmt1[0] != '\0' && fmt1[strlen(fmt1)-1] == ':') {
      fprintf(stderr, " %s<%d>", strerror(lasterr), lasterr);
    }
  }
  fprintf(stderr, "\n");
  if (StackTrace) stacktrace_err();
  if (Fatal) exit(2); /* conventional value for failed execution */
}

static s64 Check (Where_s w, s64 rc, int expected_err,
                  s64 rtn, const char *fmt, ...) {
  va_list args;

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

static void *CheckPtr (Where_s w, void *p, bool is_null, const char *fmt, ...) {
  va_list args;

  if (is_null && !p) return p;
  if (!is_null && p) return p;
  va_start(args, fmt);
  vPrErrork(w, fmt, args, " %p ", p);
  va_end(args);
  return p;
}

int chdirk (Where_s w, int err, const char *path) {
  int rc = chdir(path);
  PrVerbose(w, "chdir(%s)", path);
  return Check(w, rc, err, 0, "chdir %s:", path);
}

int closek (Where_s w, int err, int fd) {
  int rc = close(fd);
  PrVerbose(w, "close(%d)", fd);
  return Check(w, rc, err, 0, "close %d:", fd);
}

int closedirk (Where_s w, int err, DIR *dir) {
  int rc = closedir(dir);
  PrVerbose(w, "closedir(%p)", dir);
  return Check(w, rc, err, 0, "closedir(%p):", dir);
}

int creatk (Where_s w, int err, const char *path, mode_t mode) {
  int fd = creat(path, mode);
  PrVerbose(w, "creat(%s, 0%o)", path, mode);
  return Check(w, fd, err, fd, "creat(%s, 0%o):",
    path, mode);
}

s64 lseekk (Where_s w, int err, int fd, s64 offset, int whence, s64 seek) {
  s64 rc = lseek(fd, offset, whence);
  PrVerbose(w, "lseek(%d, %zu, %d)", fd, offset, whence);
  return Check(w, rc, err, seek, "lseek(%d, %zu, %d):",
    fd, offset, whence);
}

int mkdirk (Where_s w, int err, const char *path, mode_t mode) {
  int rc = mkdir(path, mode);
  PrVerbose(w, "mkdir(%s, 0%o)", path, mode);
  return Check(w, rc, err, 0, "mkdir(%s, 0%o):", path, mode);
}

int openk (Where_s w, int err, const char *path, int flags, mode_t mode) {
  int fd = open(path, flags, mode);
  PrVerbose(w, "open(%s, 0x%x, 0%o)", path, flags, mode);
  return Check(w, fd, err, fd, "open(%s, 0x%x, 0%o):",
    path, flags, mode);
}

struct dirent *opendirk (Where_s w, bool is_null, const char *path) {
  DIR *dir = opendir(path);
  PrVerbose(w, "opendir(%p)", path);
  return CheckPtr(w, dir, is_null, "opendir(%p):", path);
}

s64 readk (Where_s w, int err, int fd, void *buf, size_t nbyte, size_t size) {
  s64 rc = read(fd, buf, nbyte);
  PrVerbose(w, "read(%d, %p, %zu)", fd, buf, nbyte);
  return Check(w, rc, err, size, "read(%d, %p, %zu):",
    fd, buf, nbyte);
}

struct dirent *readdirk (Where_s w, DIR *dir) {
  struct dirent *ent = readdir(dir);
  PrVerbose(w, "readdir(%p) = %p", dir, ent);
  /* No Check for errors because NULL indicates EOF */
  return ent;
}

int rmdirk (Where_s w, int err, const char *path) {
  int rc = rmdir(path);
  PrVerbose(w, "rmdir(%s)", path);
  return Check(w, rc, err, 0, "rmdir(%s):", path);
}

s64 writek (Where_s w, int err, int fd, void *buf, size_t nbyte, size_t size) {
  s64 rc = write(fd, buf, nbyte);
  PrVerbose(w, "write(%d, %p, %zu)", fd, buf, nbyte);
  return Check(w, rc, err, nbyte, "write(%d, %p, %zu):",
    fd, buf, nbyte);
}
