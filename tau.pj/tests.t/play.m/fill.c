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

#include <sys/types.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

#include <style.h>
#include <eprintf.h>
#include <debug.h>

enum { PAGE_SIZE = 128,
	FILL_LIMIT = PAGE_SIZE - sizeof(u16)};

typedef struct fill_s {
	u16	f_next;
	char	f_data[FILL_LIMIT];
} fill_s;

typedef struct dir_s {
	ino_t	d_ino;
#ifdef __linux__
	off_t	d_off;
#endif
	u8	d_type;
	char	*d_name;
} dir_s;

enum size_checks { FILL_SIZE_GOOD = 1 / (sizeof(fill_s) == PAGE_SIZE) };

void init_fill (fill_s *f)
{
	f->f_next = 0;
}

#define PACK(_dest, _field, _i)	{				\
	memcpy( &(_dest)[(_i)], &(_field), sizeof(_field));	\
	(_i) += sizeof(_field);					\
}

#define UNPACK(_field, _src, _i) {				\
	memcpy( &(_field),  &(_src)[(_i)], sizeof(_field));	\
	(_i) += sizeof(_field);					\
}

unint fill_dirent (fill_s *f, struct dirent *d)
{
	unint	size;
	unint	i;

	size = strlen(d->d_name) + 1;
	size += sizeof(d->d_ino);
#ifdef __linux__
	size += sizeof(d->d_off);
#endif
	size += sizeof(d->d_type);

	if (size > FILL_LIMIT - f->f_next) return 0;

	i = f->f_next;
	f->f_next += size;
	PACK(f->f_data, d->d_ino, i);
#ifdef __linux__
	PACK(f->f_data, d->d_off, i);
#endif
	PACK(f->f_data, d->d_type, i);
	strcpy( &f->f_data[i], d->d_name);
	return size;
}

unint unpack (
	fill_s	*f,
	unint	offset,
	dir_s	*d)
{
	unint	i;

	if (offset >= f->f_next) return 0;

	i = offset;
	UNPACK(d->d_ino, f->f_data, i);
#ifdef __linux__
	UNPACK(d->d_off, f->f_data, i);
#endif
	UNPACK(d->d_type, f->f_data, i);
	d->d_name = &f->f_data[i];
	i += strlen(d->d_name) + 1;

	return i;
}

void dump_fill (fill_s *f)
{
	dir_s	d;
	unint	offset;

//	prmem("fill", f, sizeof(*f));
	for (offset = 0;;) {
		offset = unpack(f, offset, &d);
		if (!offset) break;
#ifdef __linux__
		printf("%ld %8llx %8llx %x %s\n", offset,
			(u64)d.d_ino, (u64)d.d_off, d.d_type, d.d_name);
#else
		printf("%ld %8llx %x %s\n", offset,
			(u64)d.d_ino, d.d_type, d.d_name);
#endif
	}
}

void filldir (char *path)
{
	struct dirent	*d;
	DIR		*dir;
	fill_s		fill;

	dir = opendir(path);
	if (!dir) {
		eprintf("opendir %s:\n", path);
		return;
	}
	init_fill( &fill);
	for (;;) {
		d = readdir(dir);
		if (!d) break;
		while (!fill_dirent( &fill, d)) {
			dump_fill( &fill);
			init_fill( &fill);
		}
	}
	dump_fill( &fill);
}

int main (int argc, char *argv[])
{
	char	*path = ".";

	debugon();
	if (argc > 1) {
		path = argv[1];
	}
	filldir(path);
	return 0;
}
