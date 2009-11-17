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

#ifndef _MDB_DEV_H_
#define _MDB_DEV_H_ 1

#define DEV_NAME        "kmdb"
#define FULL_DEV_NAME   "/dev/" DEV_NAME

enum { MDB_READ, MDB_WRITE, MDB_PID2TASK, MDB_OPS };

typedef struct Mdb_s {
	unsigned	mdb_cmd;	/* Command */
	union {
		struct {
			unsigned	mdb_size;	/* Size of user buffer */
			unsigned long	mdb_buf;	/* User buffer */
			unsigned long	mdb_addr;	/* Address to read/write */
		};
		struct {
			unsigned	pid_pid;	/* Pid to convert */
			unsigned long	pid_task;	/* Task for pid */
		};
	};
} Mdb_s;


#endif

