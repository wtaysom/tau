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

#ifndef _FM_H_
#define _FM_H_ 1

int init_fs (const char *dev);
int Lsdir   (const char *path);
int Mknode  (const char *path, unint mode);
int Creat   (const char *path);
int Open    (const char *path, u64 *key);
int Close   (u64 key);
snint Read  (u64 key, void *buf, snint num_bytes);
snint Write (u64 key, void *buf, snint num_bytes);
s64 Seek    (u64, s64 offset);

#endif
