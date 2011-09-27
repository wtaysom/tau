/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* fcalls wraps the folling functions with verbose printing,
 * error checking, and location information to make it easier
 * to track problems.
 *
 * +chdir -fchdir
 * chmod
 * +chown +fchown
 * +close
 * +creat
 * +dup +dup2 -dup3
 * fcntl
 * flock
 * fsync fdatasync
 * ioctl
 * +link
 * +lseek
 * +mkdir
 * mmap munmap
 * +open
 * +openat
 * +pread pwrite
 * +read -readlink -readlinkat
 * +rmdir
 * +stat +fstat lstat
 * +statfs fstatfs
 * +statvfs fstatvfs
 * symlink
 * sync
 * +truncate ftrancate
 * +unlink
 * +write
 *
 * Directory Library Calls
 * +closedir
 * +opendir
 * +readdir
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

#ifdef __linux__
#include <sys/statvfs.h>
#include <sys/vfs.h>
#else
#include <sys/param.h>
#include <sys/mount.h>
#endif
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
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
#include <timer.h>
#include <where.h>

#include "main.h"
#include "wrapper.h"

enum { NUM_BUCKETS = 107,
       HASH_MULT   = 37 };

#define START_TIMER  u64 start = nsecs()
#define STOP_TIMER u64 finish = nsecs(); Record(w, finish - start, __FUNCTION__)

typedef struct Time_s Time_s;
struct Time_s {
  Time_s *next;
  unint count;
  u64 sum;
  const char *id;
};

static Time_s *Bucket[NUM_BUCKETS];
static Time_s Total;

static Time_s **hash (const char *id) {
  unint h = 0;
  while (*id) {
    h = h * HASH_MULT + *id++;
  }
  return &Bucket[h % NUM_BUCKETS];
}

static Time_s *find (const char *id) {
  Time_s **bucket = hash(id);
  Time_s *t = *bucket;
  while (t) {
    if (t->id == id) return t;
    t = t->next;
  }
  t = ezalloc(sizeof(*t));
  t->id = id;
  t->next = *bucket;
  *bucket = t;
  return t;
}

/* Record a time interval */
static void Record (Where_s w, u64 time, const char *id) {
//  printf("%s:%d %lld %s\n", w.file, w.line, time, id);
  Time_s *t = find(id);
  t->sum += time;
  ++t->count;
  Total.sum += time;
  ++Total.count;
}

void DumpRecords (void) {
  int i;
  printf("syscall     count  average (nsecs)\n");
  for (i = 0; i < NUM_BUCKETS; i++) {
    Time_s *t = Bucket[i];
    while (t) {
      printf("%-10s %6ld    %g\n", t->id, t->count,
             (double)t->sum / (double)t->count);
      t = t->next;
    }
  }
  printf("Total      %6ld    %g\n", Total.count,
         (double)Total.sum / (double)Total.count);
}

/* PrVerbose used to optionally print arguments being used */
static void PrVerbose (Where_s w, const char *fmt, va_list ap) {
  if (!Local_option.verbose) return;
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
  if (Local_option.stack_trace) stacktrace_err();
  if (Local_option.exit_on_error) exit(2);
}

/* Check checks the return code from system calls.
 * If expected_err is not zero, verifies that error was set.
 */
static s64 Check (Where_s w, s64 rc, int expected_err,
                  s64 rtn, const char *fmt, ...) {
  va_list args;

//printf("rc=%lld expected_err=%d rtn=%lld\n", rc, expected_err, rtn);
  if (Local_option.verbose && fmt) {
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

  if (Local_option.verbose && fmt) {
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
  START_TIMER;
  int rc = chdir(path);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "chdir(%s)", path);
}

int fchdirk (Where_s w, int expected_err, int fd) {
  START_TIMER;
  int rc = fchdir(fd);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "fchdir(%d)", fd);
}

int chmodk (Where_s w, int expected_err, const char *path, mode_t mode) {
  START_TIMER;
  int rc = chmod(path, mode);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "chmod(%s, 0%o)", path, mode);
}

int fchmodk (Where_s w, int expected_err, int fd, mode_t mode) {
  START_TIMER;
  int rc = fchmod(fd, mode);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "fchmod(%d, 0%o)", fd, mode);
}

int chownk (Where_s w, int expected_err, const char *path,
            uid_t owner, gid_t group) {
  START_TIMER;
  int rc = chown(path, owner, group);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "chown(%s, %d, %d)",
               path, owner, group);
}

int fchownk (Where_s w, int expected_err, int fd,
            uid_t owner, gid_t group) {
  START_TIMER;
  int rc = fchown(fd, owner, group);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "fchown(%d, %d, %d)",
               fd, owner, group);
}

int lchownk (Where_s w, int expected_err, const char *path,
            uid_t owner, gid_t group) {
  START_TIMER;
  int rc = lchown(path, owner, group);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "lchown(%s, %d, %d)",
               path, owner, group);
}

int closek (Where_s w, int expected_err, int fd) {
  START_TIMER;
  int rc = close(fd);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "close(%d)", fd);
}

int creatk (Where_s w, int expected_err, const char *path, mode_t mode) {
  START_TIMER;
  int fd = creat(path, mode);
  STOP_TIMER;
  return Check(w, fd, expected_err, fd, "creat(%s, 0%o)",
               path, mode);
}

int dupk (Where_s w, int expected_err, int oldfd) {
  START_TIMER;
  int newfd = dup(oldfd);
  STOP_TIMER;
  return Check(w, newfd, expected_err, newfd, "dup(%d)", oldfd);
}

int dup2k (Where_s w, int expected_err, int oldfd, int newfd) {
  START_TIMER;
  int fd = dup2(oldfd, newfd);
  STOP_TIMER;
  return Check(w, fd, expected_err, newfd, "dup2(%d, %d)", oldfd, newfd);
}

#ifdef __linux__
int dup3k (Where_s w, int expected_err, int oldfd, int newfd, int flags) {
  START_TIMER;
  int rc = dup3(oldfd, newfd, flags);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "dup3(%d, %d, %d)",
               oldfd, newfd, flags);
}
#endif

int fcntlk (Where_s w, int expected_err, int fd, int cmd, void *arg) {
  START_TIMER;
  int rc = fcntl(fd, cmd, arg);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "fcntl(%d, %d, %p)", fd, cmd, arg);
}

int flockk (Where_s w, int expected_err, int fd, int operation) {
  START_TIMER;
  int rc = flock(fd, operation);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "flock(%d, %d)", fd, operation);
}

int fsynck (Where_s w, int expected_err, int fd) {
  START_TIMER;
  int rc = fsync(fd);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "fsync(%d)", fd);
}

#ifdef __linux__
int fdatasynck (Where_s w, int expected_err, int fd) {
  START_TIMER;
  int rc = fdatasync(fd);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "fdatasync(%d)", fd);
}
#endif

int ioctlk (Where_s w, int expected_err, int fd, int request, void *arg) {
  START_TIMER;
  int rc = ioctl(fd, request, arg);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "ioctl(%d, %d, %p)", fd, request, arg);
}

int linkk (Where_s w, int expected_err,
           const char *oldpath, const char *newpath) {
  START_TIMER;
  int rc = link(oldpath, newpath);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "link(%s, %s)", oldpath, newpath);
}

s64 lseekk (Where_s w, int expected_err, int fd,
            s64 offset, int whence, s64 seek) {
  START_TIMER;
  s64 rc = lseek(fd, offset, whence);
  STOP_TIMER;
  return Check(w, rc, expected_err, seek, "lseek(%d, %lld, %d)",
               fd, offset, whence);
}

int mkdirk (Where_s w, int expected_err, const char *path, mode_t mode) {
  START_TIMER;
  int rc = mkdir(path, mode);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "mkdir(%s, 0%o)", path, mode);
}

void *mmapk (Where_s w, bool is_null, void *addr, size_t length,
             int prot, int flags, int fd, off_t offset) {
  START_TIMER;
  void *map = mmap(addr, length, prot, flags, fd, offset);
  STOP_TIMER;
  return CheckPtr(w, map, is_null, "mmap(%p, %zd, 0%o, 0%o, %d, %zd)",
                  addr, length, prot, flags, fd, offset);
}

int munmapk (Where_s w, int expected_err, void *addr, size_t length) {
  START_TIMER;
  int rc = munmap(addr, length);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "munmap(%p, %zd)", addr, length);
}

int openk (Where_s w, int expected_err,
           const char *path, int flags, mode_t mode) {
  START_TIMER;
  int fd = open(path, flags, mode);
  STOP_TIMER;
  return Check(w, fd, expected_err, fd, "open(%s, 0x%x, 0%o)",
               path, flags, mode);
}

#ifdef __linux__
int openatk (Where_s w, int expected_err, int dirfd,
           const char *path, int flags, mode_t mode) {
  START_TIMER;
  int fd = openat(dirfd, path, flags, mode);
  STOP_TIMER;
  return Check(w, fd, expected_err, fd, "openat(%d, %s, 0x%x, 0%o)",
               dirfd, path, flags, mode);
}
#endif

s64 preadk (Where_s w, int expected_err, int fd,
            void *buf, size_t nbyte, size_t size, s64 offset) {
  START_TIMER;
  s64 rc = pread(fd, buf, nbyte, offset);
  STOP_TIMER;
  return Check(w, rc, expected_err, size, "pread(%d, %p, %zu, %lld)",
               fd, buf, nbyte, offset);
}

s64 pwritek (Where_s w, int expected_err, int fd,
             void *buf, size_t nbyte, size_t size, s64 offset) {
  START_TIMER;
  s64 rc = pwrite(fd, buf, nbyte, offset);
  STOP_TIMER;
  return Check(w, rc, expected_err, size, "pwrite(%d, %p, %zu, %lld)",
               fd, buf, nbyte, offset);
}

s64 readk (Where_s w, int expected_err, int fd,
           void *buf, size_t nbyte, size_t size) {
  START_TIMER;
  s64 rc = read(fd, buf, nbyte);
  STOP_TIMER;
  return Check(w, rc, expected_err, size, "read(%d, %p, %zu)",
               fd, buf, nbyte);
}

s64 readlinkk (Where_s w, int expected_err, const char *path,
               void *buf, size_t nbyte, size_t size) {
  START_TIMER;
  s64 rc = readlink(path, buf, nbyte);
  STOP_TIMER;
  return Check(w, rc, expected_err, size, "readlink(%s, %p, %zu)",
               path, buf, nbyte);
}

#ifdef __linux__
s64 readlinkatk (Where_s w, int expected_err, int fd, const char *path,
               void *buf, size_t nbyte, size_t size) {
  START_TIMER;
  s64 rc = readlinkat(fd, path, buf, nbyte);
  STOP_TIMER;
  return Check(w, rc, expected_err, size, "readlinkat(%d, %s, %p, %zu)",
               fd, path, buf, nbyte);
}
#endif

int rmdirk (Where_s w, int expected_err, const char *path) {
  START_TIMER;
  int rc = rmdir(path);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "rmdir(%s)", path);
}

int statk (Where_s w, int expected_err, const char *path, struct stat *buf) {
  START_TIMER;
  int rc = stat(path, buf);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "stat(%s, %p)", path, buf);
}

int fstatk (Where_s w, int expected_err, int fd, struct stat *buf) {
  START_TIMER;
  int rc = fstat(fd, buf);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "fstat(%d, %p)", fd, buf);
}

int lstatk (Where_s w, int expected_err, const char *path, struct stat *buf) {
  START_TIMER;
  int rc = lstat(path, buf);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "lstat(%s, %p)", path, buf);
}

int statfsk (Where_s w, int expected_err, const char *path,
             struct statfs *buf) {
  START_TIMER;
  int rc = statfs(path, buf);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "statfs(%s, %p)", path, buf);
}

int fstatfsk (Where_s w, int expected_err, int fd, struct statfs *buf) {
  START_TIMER;
  int rc = fstatfs(fd, buf);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "fstatfs(%d, %p)", fd, buf);
}

#ifdef __linux__
int statvfsk (Where_s w, int expected_err, const char *path,
              struct statvfs *buf) {
  START_TIMER;
  int rc = statvfs(path, buf);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "statvfs(%s, %p)", path, buf);
}

int fstatvfsk (Where_s w, int expected_err, int fd, struct statvfs *buf) {
  START_TIMER;
  int rc = fstatvfs(fd, buf);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "fstatvfs(%d, %p)", fd, buf);
}
#endif

int symlinkk (Where_s w, int expected_err,
           const char *oldpath, const char *newpath) {
  START_TIMER;
  int rc = symlink(oldpath, newpath);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "symlink(%s, %s)", oldpath, newpath);
}

void synck (Where_s w) {
  START_TIMER;
  sync();
  STOP_TIMER;
  Check(w, 0, 0, 0, "sync()");
}

int truncatek (Where_s w, int expected_err, const char *path, s64 length) {
  START_TIMER;
  int rc = truncate(path, length);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "truncate(%s, %lld)", path, length);
}

int ftruncatek (Where_s w, int expected_err, int fd, s64 length) {
  START_TIMER;
  int rc = ftruncate(fd, length);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "ftruncate(%d, %lld)", fd, length);
}

int unlinkk (Where_s w, int expected_err, const char *path) {
  START_TIMER;
  int rc = unlink(path);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "unlink(%s)", path);
}

s64 writek (Where_s w, int expected_err, int fd,
            void *buf, size_t nbyte, size_t size) {
  START_TIMER;
  s64 rc = write(fd, buf, nbyte);
  STOP_TIMER;
  return Check(w, rc, expected_err, nbyte, "write(%d, %p, %zu)",
               fd, buf, nbyte);
}

int closedirk (Where_s w, int expected_err, DIR *dir) {
  START_TIMER;
  int rc = closedir(dir);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "closedir(%p)", dir);
}

DIR *opendirk (Where_s w, bool is_null, const char *path) {
  START_TIMER;
  DIR *dir = opendir(path);
  STOP_TIMER;
  return CheckPtr(w, dir, is_null, "opendir(%s)", path);
}

struct dirent *readdirk (Where_s w, DIR *dir) {
  START_TIMER;
  struct dirent *ent = readdir(dir);
  STOP_TIMER;
  /* Check only for verbose because NULL indicates EOF */
  Check(w, 0, 0, 0, "readdir(%p)", dir);
  return ent;
}

int readdir_rk (Where_s w, int expected_err, DIR *dir, struct dirent *entry,
                struct dirent **result) {
  START_TIMER;
  s64 rc = readdir_r(dir, entry, result);
  STOP_TIMER;
  return Check(w, rc, expected_err, 0, "readdir_r(%p, %p, %p)",
               dir, entry, result);
}

void rewinddirk (Where_s w, DIR *dir) {
  START_TIMER;
  rewinddir(dir);
  STOP_TIMER;
  Check(w, 0, 0, 0, "rewinddir(%p)", dir);
}

void seekdirk (Where_s w, DIR *dir, long offset) {
  START_TIMER;
  seekdir(dir, offset);
  STOP_TIMER;
  Check(w, 0, 0, 0, "seekdir(%p, %ld)", dir, offset);
}

long telldirk (Where_s w, int expected_err, DIR *dir) {
  START_TIMER;
  long offset = telldir(dir);
  STOP_TIMER;
  return Check(w, offset, expected_err, offset, "teldir(%p)", dir);
}
