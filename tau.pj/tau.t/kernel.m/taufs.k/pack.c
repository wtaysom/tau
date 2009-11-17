/*
 * Internal routine for packing UUID's
 *
 * Copyright (C) 1996, 1997 Theodore Ts'o.
 *
 * %Begin-Header%
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ALL OF
 * WHICH ARE HEREBY DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF NOT ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * %End-Header%
 */

#include <linux/kernel.h>
#include <linux/module.h>

#include "uuidP.h"

void uuid_pack (const struct uuid *uu, guid_t ptr)
{
	u32	tmp;
	u8	*out = ptr;

	tmp = uu->time_low;
	out[3] = (u8) tmp;
	tmp >>= 8;
	out[2] = (u8) tmp;
	tmp >>= 8;
	out[1] = (u8) tmp;
	tmp >>= 8;
	out[0] = (u8) tmp;

	tmp = uu->time_mid;
	out[5] = (u8) tmp;
	tmp >>= 8;
	out[4] = (u8) tmp;

	tmp = uu->time_hi_and_version;
	out[7] = (u8) tmp;
	tmp >>= 8;
	out[6] = (u8) tmp;

	tmp = uu->clock_seq;
	out[9] = (u8) tmp;
	tmp >>= 8;
	out[8] = (u8) tmp;

	memcpy(out+10, uu->node, 6);
}

