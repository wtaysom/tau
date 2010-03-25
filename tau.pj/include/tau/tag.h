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

#ifndef _TAU_TAG_H_
#define _TAU_TAG_H_ 1

typedef void	(*method_f)(void *msg);

enum {	EBADMETHOD	= 2000, /* Bad method number */
};

/*
 * Pointer to type must be first field of an object
 */
typedef struct type_s {
	const char	*ty_name;
	unsigned	ty_num_methods;
	method_f	ty_destroy;
	method_f	ty_method[0];
} type_s;

typedef struct object_s {
	type_s	*o_type;
} object_s;

void tag_loop    (void);

#endif
