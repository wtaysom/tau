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
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void redo (int sig)
{
	sigset_t	set;

	printf("sig caught\n");
	sigfillset( &set);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
	execve("/home/taysom/playbin/sig", NULL, NULL);
}

int main (int argc, char *argv[])
{
	char	buf[10];
	int	rc;

	printf("started\n");
	signal(SIGUSR2, redo);
	rc = read(0, buf, 1);
	printf("rc=%d\n", rc);
	return 0;
}
