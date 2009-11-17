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

#ifndef _CMD_H_
#define _CMD_H_ 1

#define CMD(_x, _h)	add_cmd( _x ## p, # _x, _h )

typedef int	(*cmd_f)(int argc, char *argv[]);
typedef int	(*argv_f)(char *arg, void *data);

void init_shell (cmd_f quit_callback);
void stop_shell (void);
int  shell      (void);
void add_cmd    (cmd_f fn, char *name, char *help);
int  map_argv   (int argc, char *argv[], argv_f fn, void *data);
void execute    (int argc, char *argv[]);
int  line2argv  (char *line, char ***argvp);

#endif

