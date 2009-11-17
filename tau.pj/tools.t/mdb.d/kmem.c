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

#define _XOPEN_SOURCE 500
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#include <style.h>
#include <debug.h>
#include <kmem.h>

#include <mdb.h>

char	*Format8[]  = { " %2x",    " %.2x" };
char	*Format16[] = { " %4x",    " %.4x" };
char	*Format32[] = { " %8lx",   " %.8lx" };
char	*Format64[] = { " %16llx", " %.16llx" };

#if __LP64__
char	*FormatAddr[] = { " %16llx", " %.16llx" };
#else
char	*FormatAddr[] = { " %8lx", " %.8lx" };
#endif

/*
 * r - read data from kernel
 * p - put data to kernel
 * pr - print data to screen
 */
static void rdata (addr address, void *data, unint size)
{
	unint	n;

	n = readkmem(address, data, size);
	if (n < size) {
		err_jmp("not enough data");
	}
}

u8 r8 (addr address)
{
	u8	x;

	rdata(address, &x, sizeof(x));
	return x;
}

u16 r16 (addr address)
{
	u16	x;

	rdata(address, &x, sizeof(x));
	return x;
}

u32 r32 (addr address)
{
	u32	x;

	rdata(address, &x, sizeof(x));
	return x;
}

u64 r64 (addr address)
{
	u64	x;

	rdata(address, &x, sizeof(x));
	return x;
}

addr raddr (addr address)
{
	addr	x;

	rdata(address, &x, sizeof(x));
	return x;
}

void w8 (addr address, u8 x)
{
	writekmem(address, &x, sizeof(x));
}

void w16 (addr address, u16 x)
{
	writekmem(address, &x, sizeof(x));
}

void w32 (addr address, u32 x)
{
	writekmem(address, &x, sizeof(x));
}

void w64 (addr address, u64 x)
{
	writekmem(address, &x, sizeof(x));
}

void waddr (addr address, addr x)
{
	writekmem(address, &x, sizeof(x));
}


static inline u8 printable (u8 c)
{
	return isprint(c) ? c : '.';
}

unint linechar (addr address)
{
	unint	i;

	for (i = 0; i < U8_LINE; i++, address++) {
		printf("%c", printable(r8(address)));
	}
	return U8_LINE;
}

unint line8 (addr address)
{
	unint	i;

	for (i = 0; i < U8_LINE; i++, address++) {
		if (i == (U8_LINE/2)) printf(" ");
		printf(Format8[Zerofill], r8(address));
	}
	return U8_LINE;
}

unint line16 (addr address)
{
	unint	i;

	for (i = 0; i < U16_LINE; i++, address += sizeof(u16)) {
		printf(Format16[Zerofill], r16(address));
	}
	return U8_LINE;
}

unint line32 (addr address)
{
	unint	i;

	for (i = 0; i < U32_LINE; i++, address += sizeof(u32)) {
		printf(Format32[Zerofill], r32(address));
	}
	return U8_LINE;
}

unint line64 (addr address)
{
	unint	i;

	for (i = 0; i < U64_LINE; i++, address += sizeof(u64)) {
		printf(Format64[Zerofill], r64(address));
	}
	return U8_LINE;
}

unint lineaddr (addr address)
{
	unint	i;

	for (i = 0; i < ADDR_LINE; i++, address += sizeof(addr)) {
		printf(FormatAddr[Zerofill], raddr(address));
	}
	return U8_LINE;
}

unint prstack (addr address)
{
	Sym_s	*sym;
	u32	offset;
	addr	x;

	printf("%8lx:", address);
	x = raddr(address);

	sym = findname(x);
	printf(FormatAddr[Zerofill], x);
	if (!sym) {
		printf("\n");
		return sizeof(addr);
	}
	offset = x - sym_address(sym);
	if (offset) {
		if (offset < 0x10000) {
			printf(" %s+0x%x\n", sym_name(sym), offset);
		} else {
			printf("\n");
		}
	} else {
		printf("  %s\n", sym_name(sym));
	}
	return sizeof(addr);
}

#if 0

int main(int argc, char *argv[])
{
	int		fd;
	int		bytesRead;
	int		longsToRead = 16;
	ulong buffer[1024] = {0};
	ulong address, seekAdd;

	if (argc < 2) {
		printf("Usage:  memread  <address> [countLongs]\n");
		printf("    <address>    : Memory Address to read from\n");
		printf("    [countLongs] : Number of LONGS to read, default=16\n");
		printf("                   Maximum = 1024, (4096 bytes) \n");
		return 1;
	}

	if (argc > 2) {
		longsToRead = atoi(argv[2]);
	}
	if (longsToRead > 1024) {
		longsToRead = 1024;
	}

	address = strtoul(argv[1], NULL, 16);
	if (Debug) printf("Address = %lx, longstoread =%u\n", address, longsToRead);
	if (read_kmem(address, longsToRead * sizeof(ulong))) {
		fprintf(stderr, "read_kmem failed at %ulx\n", address);
		return errno;
	}

	prmem(Address, Length / sizeof(ulong));
	return 0;
}
#endif

