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
 * OPENDIR(3)          Linux Programmer's Manual          OPENDIR(3)
 *
 * NAME
 *   opendir - open a directory
 *
 * SYNOPSIS
 *   #include <sys/types.h>
 *   #include <dirent.h>
 *
 *   DIR *opendir(const char *name);
 *
 * DESCRIPTION
 *   The  opendir()  function  opens  a directory stream corresponding to the
 *   directory name, and returns a pointer  to  the  directory  stream.   The
 *   stream is positioned at the first entry in the directory.
 *
 * RETURN VALUE
 *   The opendir() function returns a pointer to the directory stream or NULL
 *   if an error occurred.
 *
 * ERRORS
 *   EACCES Permission denied.
 *
 *   EMFILE Too many file descriptors in use by process.
 *
 *   ENFILE Too many files are currently open in the system.
 *
 *   ENOENT Directory does not exist, or name is an empty string.
 *
 *   ENOMEM Insufficient memory to complete the operation.
 *
 *   ENOTDIR
 *          name is not a directory.
 *
 * NOTES
 *   The underlying file descriptor of the directory stream can  be  obtained
 *   using dirfd(3).
 *
 * CONFORMING TO
 *   SVID 3, POSIX, BSD 4.3
 *
 * SEE ALSO
 *   open(2),  closedir(3),  dirfd(3),  readdir(3), rewinddir(3), scandir(3),
 *   seekdir(3), telldir(3)
 *----------------------------------------------------------------------------
 *
 * READDIR(3)          Linux Programmer's Manual          READDIR(3)
 *
 * NAME
 *   readdir - read a directory
 *
 * SYNOPSIS
 *   #include <sys/types.h>
 *
 *   #include <dirent.h>
 *
 *   struct dirent *readdir(DIR *dir);
 *
 * DESCRIPTION
 *   The  readdir()  function  returns a pointer to a dirent structure repre-
 *   senting the next directory entry in the directory stream pointed  to  by
 *   dir.   It  returns  NULL  on  reaching  the  end-of-file  or if an error
 *   occurred.
 *
 *   According to POSIX, the dirent structure contains a field char  d_name[]
 *   of unspecified size, with at most NAME_MAX characters preceding the ter-
 *   minating null character.  Use of other fields will harm the  portability
 *   of your programs.  POSIX-2001 also documents the field ino_t d_ino as an
 *   XSI extension.
 *
 *   The data returned by readdir() may be overwritten by subsequent calls to
 *   readdir() for the same directory stream.
 *
 * RETURN VALUE
 *   The  readdir() function returns a pointer to a dirent structure, or NULL
 *   if an error occurs or end-of-file is reached.
 *
 * ERRORS
 *   EBADF  Invalid directory stream descriptor dir.
 *
 * CONFORMING TO
 *   SVID 3, POSIX, BSD 4.3
 *
 * SEE ALSO
 *   read(2), closedir(3), dirfd(3),  opendir(3),  rewinddir(3),  scandir(3),
 *   seekdir(3), telldir(3)
 *----------------------------------------------------------------------------
 *
 * TELLDIR(3)          Linux Programmer's Manual          TELLDIR(3)
 *
 * NAME
 *   telldir - return current location in directory stream.
 *
 * SYNOPSIS
 *   #include <dirent.h>
 *
 *   off_t telldir(DIR *dir);
 *
 * DESCRIPTION
 *   The  telldir() function returns the current location associated with the
 *   directory stream dir.
 *
 * RETURN VALUE
 *   The telldir() function returns the current  location  in  the  directory
 *   stream or -1 if an error occurs.
 *
 * ERRORS
 *   EBADF  Invalid directory stream descriptor dir.
 *
 * CONFORMING TO
 *   BSD 4.3
 *
 * SEE ALSO
 *   opendir(3),  readdir(3),  closedir(3),  rewinddir(3),  seekdir(3), scan
 *   dir(3)
 *----------------------------------------------------------------------------
 *
 * SEEKDIR(3)          Linux Programmer's Manual          SEEKDIR(3)
 *
 * NAME
 *   seekdir  -  set the position of the next readdir() call in the directory
 *   stream.
 *
 * SYNOPSIS
 *   #include <dirent.h>
 *
 *   void seekdir(DIR *dir, off_t offset);
 *
 * DESCRIPTION
 *   The seekdir() function sets the location in the  directory  stream  from
 *   which the next readdir() call will start.  seekdir() should be used with
 *   an offset returned by telldir().
 *
 * RETURN VALUE
 *   The seekdir() function returns no value.
 *
 * CONFORMING TO
 *   BSD 4.3
 *
 * SEE ALSO
 *   lseek(2), opendir(3), readdir(3), closedir(3), rewinddir(3), telldir(3),
 *   scandir(3)
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <dirent.h>
#include <unistd.h>

#include <style.h>
#include <debug.h>
#include <puny.h>

bool Wait = FALSE;

void prDirentry (struct dirent *d)
{
	printf("\t%10llu %s\n", (u64)d->d_ino, d->d_name);
	if (Wait) getchar();
}

int doDirectory (char *name)
{
	DIR		*dir;
	int		rc;
	unsigned	i;
	off_t		pos;
	struct dirent	*d;

	printf("%s:\n", name);

	for (pos = 0;;) {
		dir = opendir(name);
		if (!dir) {
			rc = errno;
			fprintf(stderr,
				"opendir:%s %s\n", name, strerror(errno));
			return rc;
		}
		seekdir(dir, pos);
		for (i = 0; i < 1; ++i) {
			d = readdir(dir);
			if (!d) {
				closedir(dir);
				return 0;
			}
			prDirentry(d);
		}
		pos = telldir(dir);
		closedir(dir);
	}
	closedir(dir);
	return 0;
}

void myopt (int c)
{
	switch (c) {
	case 'w':
		Wait = TRUE;
		break;
	default:
		usage();
		break;
	}
}

int main (int argc, char *argv[])
{
	int	i;
	int	rc;

	punyopt(argc, argv, myopt, "w");
	if (argc == optind) {
		rc = doDirectory(Option.dir);
		return rc;
	}
	for (i = optind; i < argc; ++i) {
		rc = doDirectory(argv[i]);
		if (rc != 0) return rc;
	}
	return 0;
}
