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

#ifndef _M_H_
#define _M_H_

#ifndef _STYLE_H_
#include <style.h>
#endif

#ifndef _KERNELSYMBOLS_H_
#include <kernelsymbols.h>
#endif

#define DISASM ENABLE
#define READLINE ENABLE


enum {	U8_LINE       = 16,
	U16_LINE      = U8_LINE/sizeof(u16),
	U32_LINE      = U8_LINE/sizeof(u32),
	U64_LINE      = U8_LINE/sizeof(u64),
	ADDR_LINE     = U8_LINE/sizeof(addr),
	DEFAULT_LINES = 16,
	MAX_LINES     = 128 };

bool	Debug;
bool	Zerofill;

void err_jmp   (char *msg);
void invokeCmd (int argc, char *argv[]);

void initeval (void);
u64  eval    (char *p);

void  initkmem (void);

u8   r8    (addr address);
u16  r16   (addr address);
u32  r32   (addr address);
u64  r64   (addr address);
addr raddr (addr address);

void w8    (addr address, u8 x);
void w16   (addr address, u16 x);
void w32   (addr address, u32 x);
void w64   (addr address, u64 x);
void waddr (addr address, addr x);

unint linechar (addr address);
unint line8    (addr address);
unint line16   (addr address);
unint line32   (addr address);
unint line64   (addr address);
unint lineaddr (addr address);

unint prstack  (addr address);

addr disasm (addr address);


#endif
