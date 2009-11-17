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

/*
 * A sack is a type of dynamic array where the array indicies
 * are assigned by the array.
 *	1. Never blocks on reference
 *	2. Entry 0 should be invalid
 */
#include <stdlib.h>
#include <string.h>

#include "sack.h"
			
unsigned sack_put (sack_s *sk, void *d)
{
	void		**p;
	void		**new;
	unsigned	i;

	p = sk->sk_next;
	if (!p) {	/* Grow handle area */
		if (!sk->sk_end) {
			sk->sk_end = 1;
		}
		new = malloc((2*sk->sk_end) * sizeof(void *));
		if (!new) {
			return 0;
		}
		memset(new, 0, (2*sk->sk_end) * sizeof(void *));
		if (sk->sk_data) {
			memcpy(new, sk->sk_data, sk->sk_end * sizeof(void *));
			free(sk->sk_data);
		}
		sk->sk_next = &new[sk->sk_end];
		for (i = sk->sk_end; i < 2*sk->sk_end - 1; i++) {
			new[i] = &new[i+1];
		}
		new[i] = 0;
		sk->sk_data = new;
		sk->sk_end *= 2;
		p = sk->sk_next;
	}
	sk->sk_next = *sk->sk_next;
	*p = d;
	return p - sk->sk_data;
}

void *sack_get (sack_s *sk, unsigned n)
{
	void	*d;

	if (!sk->sk_data) {
		return NULL;
	}
	if (n >= sk->sk_end) {
		return NULL;
	}
	d = sk->sk_data[n];
	if (!d) {
		return NULL;
	}
	if (((void **)d >= sk->sk_data)
		&& ((void **)d < &sk->sk_data[sk->sk_end]))
	{
		return NULL;
	}
	return d;
}

void *sack_remove (sack_s *sk, unsigned n)
{
	void	*d;

	d = sack_get(sk, n);
	if (d) {
		sk->sk_data[n] = sk->sk_next;
		sk->sk_next = &sk->sk_data[n];
	}
	return d;
}

void sack_free (sack_s *sk)
{
	free(sk->sk_data);
}

int sack_for_each (sack_s *sk, sack_fn fn, void *data)
{
	void		*d;
	unsigned	i;
	int		rc;

	for (i = 0; i < sk->sk_end; i++) {
		d = sk->sk_data[i];
		if (((void **)d >= sk->sk_data)
			&& ((void **)d < &sk->sk_data[sk->sk_end]))
		{
			rc = fn(d, data);
			if (rc) return rc; 
		}
	}
	return 0;	
}

void sack_init (sack_s *sk)
{
	sk->sk_data = 0;
	sk->sk_next = 0;
	sk->sk_end  = 0;
}

