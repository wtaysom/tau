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
 * CREATE Long file names
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <style.h>
#include <mylib.h>
#include <puny.h>
#include <eprintf.h>

/*
 * UTF-8 is a encoding of Unicode developed by Bell Labs
 * and adopted as a standard.  It has numerous advantages
 * over regular unicode for serial communication.  The best
 * is that C ASCII strings have the same representation.
 *
 * 0xxxxxxx
 * 110xxxxx 10xxxxxx
 * 1110xxxx 10xxxxxx 10xxxxxx
 *
 * Information taken from "Java in a Nutshell":
 *
 * 1. All ASCII characters are one-byte UTF-8 characters.  A legal
 *    ASCII string is a legal UTF-8 string.
 *
 * 2. Any non-ASCII character (i.e., any character with the high-
 *    order bit set) is part of a multibyte character.
 *
 * 3. The first byte of any UTF-8 character indicates the number
 *    of additional bytes in the character.
 *
 * 4. The first byte of a multibyte character is easily distinguished
 *    from the subsequent bytes.  Thus, it is easy to locate the start
 *    of a character from an arbitrary position in a data stream.
 *
 * 5. It is easy to convert between UTF-8 and Unicode.
 *
 * 6. The UTF-8 encoding is relatively compact.  For text with a large
 *    percentage of ASCII characters, is it more compact than Unicode.  In
 *    the worst case, a UTF-8 string is only 50% larger than the corresponding
 *    Unicode string.
 */


char BigChar[] = { 0xe0, 0x80, 0x80 };


void usage (void)
{
	pr_usage("-f<file_name> -i<num_iterations> -l<loops>");
}

int main (int argc, char *argv[])
{
	char		*name;
	int		fd;
	int		rc;
	unsigned	i;
	unsigned	n = 1000;
	u64		l;

	punyopt(argc, argv, NULL, NULL);
	n = Option.iterations;
	name = Option.file;
	for (l = 0; l < Option.loops; l++) {
		startTimer();
		for (i = 0; i < n; ++i) {
			fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);
			if (fd == -1) {
				perror("open");
				exit(1);
			}
			close(fd);
			rc = unlink(name);
			if (rc == -1) {
				perror("unlink");
				exit(1);
			}
		}
		stopTimer();
		prTimer();
		printf("\n");
	}
	return 0;
}

