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

#include <QtGui>

#include "plotter.h"

void start_f_g  (Plotter &plotter);
void start_barrier  (Plotter &plotter);
void start_disk_io (Plotter &plotter);
void start_partition_io (Plotter &plotter);
void start_zlss_writes (Plotter &plotter);
void start_disk_io (Plotter &plotter);
void start_partition_io (Plotter &plotter);
void start_write_io (Plotter &plotter);
void start_read_io (Plotter &plotter);
void start_writes (Plotter &plotter);
void start_zlss_io (Plotter &plotter);
void start_pending_reads (Plotter &plotter);
void start_pending_writes (Plotter &plotter);
void start_reads_finished (Plotter &plotter);
void start_writes_finished (Plotter &plotter);
void start_waiting (Plotter &plotter);

void start_context_switches (Plotter &plotter);

void start_kernel_variables (QApplication &a, Plotter &plotter);

int main(int argc, char *argv[])
{
	QApplication	app(argc, argv);

	Plotter plotter;
	start_kernel_variables(app, plotter);

#if 0
	Plotter plotter_ctxt;
	start_context_switches(plotter_ctxt);
#endif

	return app.exec();
}
