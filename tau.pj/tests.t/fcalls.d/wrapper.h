/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _WRAPPER_H_
#define _WRAPPER_H_ 1

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <dirent.h>

#include <style.h>
#include <where.h>

int  chdirk(Where_s where, int expected_err, const char *path);
int  fchdirk(Where_s where, int expected_err, int fd);

int  chmodk(Where_s where, int expected_err, const char *path, mode_t mode);
int  fchmodk(Where_s where, int expected_err, int fd, mode_t mode);

int  chownk(Where_s w, int expected_err, const char *path,
            uid_t owner, gid_t group);
int  fchownk(Where_s w, int expected_err, int fd,
             uid_t owner, gid_t group);
int  lchownk(Where_s w, int expected_err, const char *path,
             uid_t owner, gid_t group);

int  closek(Where_s w, int expected_err, int fd);
int  creatk(Where_s w, int expected_err, const char *path, mode_t mode);

int  dupk  (Where_s w, int expected_err, int oldfd);
int  dup2k (Where_s w, int expected_err, int oldfd, int newfd);
int  dup3k (Where_s w, int expected_err, int oldfd, int newfd, int flags);

int  fcntlk(Where_s w, int expected_err, int fd, int cmd, void *arg);
int  flockk(Where_s w, int expected_err, int fd, int operation);

int  fsynck    (Where_s w, int expected_err, int fd);
int  fdatasynck(Where_s w, int expected_err, int fd);

int  ioctlk(Where_s w, int expected_err, int fd, int request, void *arg);
int  linkk(Where_s w, int expected_err,
           const char *oldpath, const char *newpath);
s64  lseekk(Where_s w, int expected_err, int fd,
            s64 offset, int whence, s64 seek);
int  mkdirk(Where_s w, int expected_err, const char *path, mode_t mode);

void *mmapk (Where_s w, bool is_null, void *addr, size_t length,
             int prot, int flags, int fd, off_t offset);
int  munmapk(Where_s w, int expected_err, void *addr, size_t length);

int  openk  (Where_s w, int expected_err,
             const char *path, int flags, mode_t mode);
int  openatk(Where_s w, int expected_err, int dirfd,
             const char *path, int flags, mode_t mode);

s64  preadk (Where_s w, int expected_err, int fd,
            void *buf, size_t nbyte, size_t size, s64 offset);
s64  pwritek(Where_s w, int expected_err, int fd,
            void *buf, size_t nbyte, size_t size, s64 offset);

s64  readk      (Where_s w, int expected_err, int fd,
                 void *buf, size_t nbyte, size_t size);
s64  readlinkk  (Where_s w, int expected_err, const char *path,
                 void *buf, size_t nbyte, size_t size);
s64  readlinkatk(Where_s w, int expected_err, int fd, const char *path,
                 void *buf, size_t nbyte, size_t size);

int  rmdirk  (Where_s w, int expected_err, const char *path);

int statk  (Where_s w, int expected_err, const char *path, struct stat *buf);
int fstatk (Where_s w, int expected_err, int fd, struct stat *buf);
int lstatk (Where_s w, int expected_err, const char *path, struct stat *buf);

int statfsk  (Where_s w, int expected_err, const char *path,
              struct statfs *buf);
int fstatfsk (Where_s w, int expected_err, int fd, struct statfs *buf);
int statvfsk (Where_s w, int expected_err, const char *path,
              struct statvfs *buf);
int fstatvfsk(Where_s w, int expected_err, int fd, struct statvfs *buf);

int symlinkk (Where_s w, int expected_err,
           const char *oldpath, const char *newpath);

void synck(Where_s w);

int  truncatek (Where_s w, int expected_err, const char *path, s64 length);
int  ftruncatek(Where_s w, int expected_err, int fd, s64 length);
int  unlinkk   (Where_s w, int expected_err, const char *path);
s64  writek    (Where_s w, int expected_err, int fd, void *buf,
                size_t nbyte, size_t size);


int  closedirk(Where_s w, int expected_err, DIR *dir);
DIR *opendirk(Where_s w, bool is_null, const char *path);
struct dirent *readdirk(Where_s w, DIR *dir);
int  readdir_rk(Where_s w, int expected_err, DIR *dir,
                struct dirent *entry, struct dirent **result);
void rewinddirk (Where_s w, DIR *dir);
void seekdirk(Where_s w, DIR *dir, long offset);
long telldirk(Where_s w, int expected_err, DIR *dir);

#endif  /* _WRAPPER_H_ */
