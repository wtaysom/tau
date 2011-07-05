/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#include <fcalls.h>

/*
 * Basic read/write tests
 */

void rw_test (void)
{
	char	buf[227];
	char	file[] = "file_a";
	int	fd;

	fd = open(file, O_CREAT | O_TRUNC | O_RDWR, 0666);
	write(fd, buf, sizeof(buf));
	write(fd, buf, 17);
	lseek(fd, 0, 0);
	read(fd, buf, sizeof(buf));
	readSz(fd, buf, sizeof(buf), 17);
	close(fd);
}
