/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _WRAPPER_H_
#define _WRAPPER_H_ 1

#include <sys/types.h>
#include <dirent.h>

#include <style.h>
#include <where.h>

int  chdirk(Where_s where, int expected_err, const char *path);
int  closek(Where_s w, int expected_err, int fd);
int  closedirk(Where_s w, int expected_err, DIR *dir);
int  creatk(Where_s w, int expected_err, const char *path, mode_t mode);
s64  lseekk(Where_s w, int expected_err, int fd,
            s64 offset, int whence, s64 seek);
int  mkdirk(Where_s w, int expected_err, const char *path, mode_t mode);
int  openk (Where_s w, int expected_err, const char *path,
            int flags, mode_t mode);
DIR *opendirk(Where_s w, bool is_null, const char *path);
s64  readk (Where_s w, int expected_err, int fd, void *buf,
            size_t nbyte, size_t size);
struct dirent *readdirk(Where_s w, DIR *dir);
int  readdir_rk(Where_s w, int expected_err, DIR *dir,
                struct dirent *entry, struct dirent **result);
int  rmdirk(Where_s w, int expected_err, const char *path);
void seekdirk(Where_s w, DIR *dir, long offset);
long telldirk(Where_s w, int expected_err, DIR *dir);
s64  writek(Where_s w, int expected_err, int fd, void *buf,
            size_t nbyte, size_t size);

#endif  /* _WRAPPER_H_ */
