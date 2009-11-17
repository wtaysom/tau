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

#ifndef _TAU_MSG_UTIL_H_
#define _TAU_MSG_UTIL_H_ 1

#ifndef _TAU_TAG_H_
#include <tau/tag.h>
#endif

ki_t make_gate (
	void		*tag,
	unsigned	type);

ki_t make_datagate (
	void		*tag,
	unsigned	type,
	void		*start,
	unsigned	length);

type_s *make_type (char *name, method_f destroy, ...);

#endif
