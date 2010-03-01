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

#ifndef _JILL_H_
#define _JILL_H_ 1

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

enum {	JILL_IO_SIZE  = 1<<16,
	JILL_MAX_PATH = 1<<12,
	JILL_MAX_FDN  = 1<<10,
	JILL_NAME_LEN = (1<<8) - 1 };

enum {	JILL_SW_REG = 0,
	JILL_SW_OPS };

enum {	JILL_ROOT_KEY = 0,
	JILL_NEW_CONNECTION,
	JILL_CREATE,
	JILL_DELETE,
	JILL_OPEN,
	JILL_READ,
	JILL_WRITE,
	JILL_ENUMERATE,
	JILL_GET_INFO,
	JILL_ZID_OPEN,
	JILL_OPS };

#ifndef _NSS_INTERNAL_
	enum { zVALID_GET_INFO_MASK = (zGET_STD_INFO | zGET_NAME | zGET_ALL_NAMES |
			zGET_PRIMARY_NAMESPACE | zGET_TIMES_IN_SECS |
			zGET_TIMES_IN_MICROS | zGET_IDS | zGET_STORAGE_USED |
			zGET_BLOCK_SIZE | zGET_COUNTS | zGET_DATA_STREAM_INFO |
			zGET_DELETED_INFO | zGET_MAC_METADATA | zGET_UNIX_METADATA |
			zGET_VOLUME_INFO | zGET_VOL_SALVAGE_INFO | zGET_POOL_INFO | 
			zGET_DIR_QUOTA | zGET_INH_RIGHTS_MASK | zGET_ALL_TRUSTEES )};
#endif


typedef struct jmsg_s {
	sys_s   q;
	union {
		METHOD_S;
		body_u  b;
		struct {
			METHOD_S;
			u64	ji_zid;
			u32	ji_id;
			u32	ji_task;
			u32	ji_name_space;
			u32	ji_unused;
			u64	ji_requested_rights;
			u64	ji_file_type;
			u64	ji_file_attributes;
			u64	ji_create_flags;
		};
		struct {
			METHOD_S;
			u64	jo_offset;	// Starting offset for read/write
			u64	jo_length;	// Bytes actually read/written
			u64	jo_cookie;	// Cookie for enumeration
			u64	jo_info_mask;
		};
	};
} jmsg_s;

#endif
