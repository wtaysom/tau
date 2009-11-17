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

#ifndef _CRC_T_
#define _CRC_T_

#ifndef _STYLE_H_
#include <style.h>
#endif

#define INITIAL_CRC64 0xffffffffffffffffULL
#define INITIAL_CRC32 0xffffffffUL

#ifdef __KERNEL__
	u64 hash_string_64_tau (const char *t);
	u32 hash_string_32_tau (const char *t);
#endif
	u64 update_crc64 (const void *buf, unint len, u64 crc);
	u64 crc64 (const void *buf, unint len);
	u64 hash_string_64 (const char *t);

	u32 crc32 (const void *buf, unint len);
	u32 hash_unicode_32 (const u16 *t);
	u32 hash_string_32 (const char *t);
	u32 string_crc32 (const char *s);
	u32 update_crc32 (const void *buf, unint len, u32 crc);

#endif
