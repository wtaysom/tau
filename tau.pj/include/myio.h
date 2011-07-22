/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/

#ifndef _MYIO_H_
#define _MYIO_H_

#include <sys/types.h>
#include <sys/stat.h>

void io_error (const char *what, const char *who);

void	chdirq(const char *path);
void	closeq(int fd);
void	fstatq(int fd, struct stat *sb);
void	lseekq(int fd, off_t offset) ;
void	mkdirq(const char *path);
int	openq(const char *path, int flags);
ssize_t	readq(int fd, void *buf, size_t nbytes);
void	rmdirq(const char *path);
void	statq(const char *path, struct stat *sb);
void	unlinkq(const char *path);
ssize_t	writeq(int fd, const void *buf, size_t nbytes);

void	rmtreeq(const char *path);

typedef void (*dir_f)(char *name, void *arg, int level);
extern void walk_dir (char *name, dir_f f, void *arg, int level);

typedef void (*file_f)(char *name, void *arg);
void walk_tree (char *name, file_f fn, void *arg);

#endif
