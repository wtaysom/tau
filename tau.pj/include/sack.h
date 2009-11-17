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

#ifndef _SACK_H_
#define _SACK_H_ 1

typedef struct sack_s {
	void		**sk_data;
	void		**sk_next;
	unsigned	sk_end;
} sack_s;

typedef int	(*sack_fn)(void *d, void *data);

unsigned sack_put (sack_s *sk, void *d);
void *sack_get (sack_s *sk, unsigned n);
void *sack_remove (sack_s *sk, unsigned n);
void sack_free (sack_s *sk);
void sack_init (sack_s *sk);
int sack_for_each (sack_s *sk, sack_fn fn, void *data);

#endif
