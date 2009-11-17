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

#include <signal.h>
#include <calc.h>

void setSignals (void)
{
	signal(SIGHUP,	finish);
	signal(SIGINT,	SIG_IGN);
	signal(SIGQUIT,	finish);
	signal(SIGTRAP,	finish);
	signal(SIGABRT,	finish);
	signal(SIGFPE,	finish);
	signal(SIGILL,	finish);
	signal(SIGBUS,	finish);
	signal(SIGSEGV,	finish);
	signal(SIGTSTP,	finish);
}
