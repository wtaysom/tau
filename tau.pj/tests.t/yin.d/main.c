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
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#include <style.h>
#include <eprintf.h>
#include <cmd.h>

#include <yin.h>

enum {
	BUF_SIZE = 4096
};

char	Buf[BUF_SIZE];

int cat_file (char *name)
{
	int	fd;
	int	bytes;
	int	i;

	fd = open(name, O_RDONLY);
	if (fd == -1) {
		perror(name);
		return errno;
	}
	for (;;) {
		bytes = read(fd, Buf, BUF_SIZE);
		if (bytes == 0) break;
		if (bytes == -1) {
			perror("read");
			return errno;
		}
		for (i = 0; i < bytes; ++i) {
			putchar(Buf[i]);
		}
	}
	close(fd);
	return 0;
}

int catp (int argc, char *argv[])
{
	int	rc;

	if (argc == 1) return 0;
	rc = cat_file(argv[1]);
	if (rc != 0)
	{
		printf("Problems catting %s\n", argv[1]);
		return rc;
	}
	return 0;
}

int in_file (char *name)
{
	int	fd;
	int	bytes;
	int	i;
	int	c;

	fd = open(name, O_WRONLY|O_CREAT|O_TRUNC, 0666);
	if (fd == -1) {
		perror(name);
		return errno;
	}
	for (i = 0;; i++) {
		c = getchar();
		if (c == -1) {
			bytes = write(fd, Buf, i);
			if (bytes == -1) {
				perror("write 1");
				return errno;
			}
			break;
		}
		if (i == BUF_SIZE) {
			bytes = write(fd, Buf, BUF_SIZE);
			if (bytes == -1) {
				perror("write 2");
				return errno;
			}
			i = 0;
		}
		Buf[i++] = c;
	}
	close(fd);
	return 0;
}

int inp (int argc, char *argv[])
{
	int	rc;

	if (argc == 1) return 0;
	rc = in_file(argv[1]);
	if (rc != 0) {
		printf("Problems filling %s\n", argv[1]);
		return rc;
	}
	return 0;
}

int cdp (int argc, char *argv[])
{
	int	rc;

	if (argc == 1) return 0;
	rc = chdir(argv[1]);
	if (rc == -1) {
		perror(argv[1]);
		return errno;
	}
	return 0;
}

void pr_stat_nice (struct stat *sb)
{

	static char File_type[] = { '0', 'p', 'c', '3', 'd', '5', 'b', '7',
					'-', '9', 'l', 'B', 's', 'D', 'E', 'F' };
	char	*time = ctime( &sb->st_mtime);
	int	n;

	n = strlen(time);
	time[n-1] = '\0';

	printf("%c%4o %6d %8llu %s ", //"%d %o %d %d %qd %s ",
		File_type[(sb->st_mode >> 12) & 017],
		sb->st_mode & 07777,	/* inode protection mode */
		sb->st_uid,		/* user-id of owner */
		(u64)sb->st_size,	/* file size, in bytes */
		time			/* time of last data modification */
	);
}

void pr_stat (struct stat *sb)
{
	printf("%llu:%llu %llo %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu ",
		(u64)sb->st_dev,	/* device inode resides on */
		(u64)sb->st_ino,	/* inode's number */
		(u64)sb->st_mode,	/* inode protection mode */
		(u64)sb->st_nlink,	/* number or hard links to the file */
		(u64)sb->st_uid,	/* user-id of owner */
		(u64)sb->st_gid,	/* group-id of owner */
		(u64)sb->st_rdev,	/* device type,	for special file inode */
		(u64)sb->st_size,	/* file size, in bytes */
		(u64)sb->st_blocks,	/* blocks allocated for file */
		(u64)sb->st_blksize,	/* optimal file sys I/O ops blocksize */
		(u64)sb->st_atime,	/* time of last access */
		(u64)sb->st_mtime,	/* time of last data modification */
		(u64)sb->st_ctime	/* time of last file status change */
		//sb->st_flags,	/* user defined flags for file */
		//sb->st_gen	/* file generation number */
	);
}

void pr_stat_sizes (struct stat *sb)
{
	printf("%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
		(unint)sizeof(sb->st_dev),
		(unint)sizeof(sb->st_ino),	
		(unint)sizeof(sb->st_mode),
		(unint)sizeof(sb->st_nlink),
		(unint)sizeof(sb->st_uid),
		(unint)sizeof(sb->st_gid),	
		(unint)sizeof(sb->st_rdev),
		(unint)sizeof(sb->st_size),
		(unint)sizeof(sb->st_blocks),
		(unint)sizeof(sb->st_blksize),
		(unint)sizeof(sb->st_atime),	
		(unint)sizeof(sb->st_mtime),
		(unint)sizeof(sb->st_ctime)
	);
}

bool	doStat = TRUE;

int lsp (int argc, char *argv[])
{
	struct dirent	*entry;
	struct stat	sb;
	DIR		*dir;
	int		rc;

	dir = opendir(".");
	if (dir == NULL) {
		perror(".");
		return errno;
	}
	for (;;) {
		entry = readdir(dir);
		if (entry == NULL) break;
		if (doStat) {
			rc = stat(entry->d_name, &sb);
			if (rc == -1) {
				printf("Couldn't stat:%s\n", entry->d_name);
				continue;
			}
			pr_stat_nice( &sb);
		}
		printf("%s\n", entry->d_name);
	}
	return 0;
}

static int lp (int argc, char *argv[])
{
	u64	id;
	int	rc;

	if (argc <= 1) {
		printf("nothing to lookup\n");
		return -1;
	}
printf("argv[1]=%s\n", argv[1]);
	rc = ylookup(argv[1], &id);
	if (rc) return rc;

	printf("id=%lld\n", id);
	return 0;
}

void init_cmd (void)
{
	CMD(cat,"# print file");
	CMD(in, "# input to file");
	CMD(cd, "# change directory");
	CMD(ls, "# list files");
	CMD(l,  "# lookup file");
}

int main (int argc, char *argv[])
{
	setprogname(argv[0]);
	init_shell(NULL);
	init_cmd();
	ylogin();
	init_tst();
	return shell();
}
