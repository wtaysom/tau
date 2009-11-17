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

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <style.h>
#include <kernelsymbols.h>
#include <kmem.h>

static void init (void)
{
	static bool	inited = FALSE;

	if (inited) return;
	inited = TRUE;

	initsyms();
	initkmem();
}

static addr getaddr (char *name)
{
	Sym_s	*sym;

	sym = findsym(name);
	if (!sym) {
		fprintf(stderr, "Couldn't find %s address\n", name);
		exit(2);
	}
	return sym_address(sym);
}

/*********************************************************************/
static addr	BarrierAddress;
static u64	Barrier, OldBarrier;

static double Barrierx = -1.0;

static double barrier (double x, int id)
{
	unused(id);
//printf("%s x=%g\n", __func__, x);
	if (x > Barrierx) {
		OldBarrier = Barrier;
		Barrierx = x;
		readkmem(BarrierAddress, &Barrier, sizeof(Barrier));
	}
	return Barrier - OldBarrier;
}

static void init_barrier (void)
{
	static bool	inited = FALSE;

	if (inited) return;
	inited = TRUE;

	init();

	BarrierAddress = getaddr("Barrier");
	readkmem(BarrierAddress, &Barrier, sizeof(Barrier));
}

/*********************************************************************/
typedef struct {
	ulong	writeIn;
	ulong	writeOut;
	ulong	writeInXdata;  /* Other objects dependent on this being written */
	ulong	writeOutXdata;
	ulong	readIn;
	ulong	readOut;
	ulong	readWIn;
	ulong	readWOut;
} ZLSSIOsInst_s;

ZLSSIOsInst_s	IOsInst, IOsInst_old;
addr	IOsInst_address;
double	IOsInst_x = -1.0;

void init_IOsInst (void)
{
	static bool	inited = FALSE;

	if (inited) return;
	inited = TRUE;

	init();

	IOsInst_address = getaddr("IOsInst");
	readkmem(IOsInst_address, &IOsInst, sizeof(IOsInst));
}

static inline void IOsInst_update (double x)
{
	if (x > IOsInst_x) {
		IOsInst_old = IOsInst;
		IOsInst_x = x;
		readkmem(IOsInst_address, &IOsInst, sizeof(IOsInst));
	}
}

double IOsInst_writeIn (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	return IOsInst.writeIn - IOsInst_old.writeIn;
}

double IOsInst_writeOut (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	return IOsInst.writeOut - IOsInst_old.writeOut;
}

double IOsInst_pendingWrites (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	if (IOsInst.writeOut > IOsInst.writeIn) {
		return 0;
	}
	return IOsInst.writeIn - IOsInst.writeOut;
}

double IOsInst_writeInXdata (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	return IOsInst.writeInXdata - IOsInst_old.writeInXdata;
}

double IOsInst_writeOutXdata (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	return IOsInst.writeOutXdata - IOsInst_old.writeOutXdata;
}

double IOsInst_pendingWritesX (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	if (IOsInst.writeOutXdata > IOsInst.writeInXdata) {
		return 0;
	}
	return IOsInst.writeInXdata - IOsInst.writeOutXdata;
}

double IOsInst_writeTotalIn (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	return (IOsInst.writeIn + IOsInst.writeInXdata)
		- (IOsInst_old.writeIn + IOsInst_old.writeInXdata);
}

double IOsInst_writeTotalOut (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	return (IOsInst.writeOut + IOsInst.writeOutXdata)
		- (IOsInst_old.writeOut + IOsInst_old.writeOutXdata);
}

double IOsInst_pendingTotalWrites (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	if ((IOsInst.writeOut + IOsInst.writeOutXdata)
			> (IOsInst.writeIn + IOsInst.writeInXdata)) {
		return 0;
	}
	return (IOsInst.writeIn + IOsInst.writeInXdata)
		- (IOsInst.writeOut + IOsInst.writeOutXdata);
}

double IOsInst_readIn (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	return IOsInst.readIn - IOsInst_old.readIn;
}

double IOsInst_readOut (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	return IOsInst.readOut - IOsInst_old.readOut;
}

double IOsInst_pendingReads (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	return IOsInst.readIn - IOsInst.readOut;
}

double IOsInst_readWIn (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	return IOsInst.readWIn - IOsInst_old.readWIn;
}

double IOsInst_readWOut (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	return IOsInst.readWOut - IOsInst_old.readWOut;
}

double IOsInst_pendingReadsW (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	return IOsInst.readWIn - IOsInst.readWOut;
}

double IOsInst_readTotal (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	return (IOsInst.readOut + IOsInst.readWOut)
		- (IOsInst_old.readOut + IOsInst_old.readWOut);
}

double IOsInst_pendingTotalReads (double x, int id)
{
	unused(id);
	IOsInst_update(x);
	return (IOsInst.readIn + IOsInst.readWIn)
		- (IOsInst.readOut + IOsInst.readWOut);
}


/*********************************************************************/
addr	PJTAddress;
struct PJT_s {
	ulong	xlatch;
	ulong	nobufs;
	ulong	dup;
} PJT, OldPJT;

double PJTx = -1.0;

void init_pjt (void)
{
	static bool	inited = FALSE;

	if (inited) return;
	inited = TRUE;

	init();

	PJTAddress = getaddr("PJT");
	readkmem(PJTAddress, &PJT, sizeof(PJT));
}

double pjt_xlatch (double x, int id)
{
	unused(id);
//printf("%s x=%g\n", __func__, x);
	if (x > PJTx) {
		OldPJT = PJT;
		PJTx = x;
		readkmem(PJTAddress, &PJT, sizeof(PJT));
	}
	return PJT.xlatch - OldPJT.xlatch;
}

double pjt_nobufs (double x, int id)
{
	unused(id);
	if (x > PJTx) {
		OldPJT = PJT;
		PJTx = x;
		readkmem(PJTAddress, &PJT, sizeof(PJT));
	}
	return PJT.nobufs - OldPJT.nobufs;
}

double pjt_dup (double x, int id)
{
	unused(id);
	if (x > PJTx) {
		OldPJT = PJT;
		PJTx = x;
		readkmem(PJTAddress, &PJT, sizeof(PJT));
	}
	return PJT.dup - OldPJT.dup;
}

/***************************************************************************/
addr	WaitingAddress;
struct Waiting_s {
	ulong	wait;
	ulong	granted;
} Waiting;

void init_waiting (void)
{
	static bool	inited = FALSE;

	if (inited) return;
	inited = TRUE;

	init();

	WaitingAddress = getaddr("Waiting");
	readkmem(WaitingAddress, &Waiting, sizeof(Waiting));
}

double waiting (double, int id)
{
	unused(id);
	readkmem(WaitingAddress, &Waiting, sizeof(Waiting));
	return Waiting.wait - Waiting.granted;
}


/***************************************************************************/
addr	ZlssDevIOAddress;
struct ZlssDevIO_s {
	ulong	reads_started;
	ulong	reads_finished;
	ulong	writes_started;
	ulong	writes_finished;
 } ZlssDevIO, OldDevIO;

double	ZlssDevIO_x = -1.0;

static void init_ZlssDevIO (void)
{
	static bool	inited = FALSE;

	if (inited) return;
	inited = TRUE;

	init();

	ZlssDevIOAddress = getaddr("ZlssDevIO");
	readkmem(ZlssDevIOAddress, &ZlssDevIO, sizeof(ZlssDevIO));
}

static inline void ZlssDevIO_update (double x)
{
	if (x > ZlssDevIO_x) {
		OldDevIO = ZlssDevIO;
		ZlssDevIO_x = x;
		readkmem(ZlssDevIOAddress, &ZlssDevIO, sizeof(ZlssDevIO));
	}
}

double pending_reads (double x, int id)
{
	unused(id);
	ZlssDevIO_update(x);
	return ZlssDevIO.reads_started - ZlssDevIO.reads_finished;
}

double pending_writes (double x, int id)
{
	unused(id);
	ZlssDevIO_update(x);
	return ZlssDevIO.writes_started - ZlssDevIO.writes_finished;
}

double reads_started (double x, int id)
{
	unused(id);
	ZlssDevIO_update(x);
	return ZlssDevIO.reads_started - OldDevIO.reads_started;
}

double writes_started (double x, int id)
{
	unused(id);
	ZlssDevIO_update(x);
	return ZlssDevIO.writes_started - OldDevIO.writes_started;
}

double reads_finished (double x, int id)
{
	unused(id);
	ZlssDevIO_update(x);
	return ZlssDevIO.reads_finished - OldDevIO.reads_finished;
}

double writes_finished (double x, int id)
{
	unused(id);
	ZlssDevIO_update(x);
	return ZlssDevIO.writes_finished - OldDevIO.writes_finished;
}

/***************************************************************************/
typedef struct Disk_s {
	unsigned	reads;
	unsigned	reads_merged;
	unsigned	sectors_read;
	unsigned	msecs_reading;

	unsigned	writes;
	unsigned	writes_merged;
	unsigned	sectors_written;
	unsigned	msecs_writing;

	unsigned	ios_in_progress;
	unsigned	msecs_in_progress;
	unsigned	msecs_weighted;
} Disk_s;

Disk_s	Disk_old, Disk_new;
char	Disk_buf[4096];
double	Disk_x = -1.0;
char	*Disk;

static void read_disk (void)
{
	int	fd;
	int	rc;

	fd = open(Disk, O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(2);
	}

	rc = read(fd, Disk_buf, sizeof(Disk_buf));
	if (rc == -1) {
		perror("read");
		exit(2);
	}
	Disk_buf[rc] = '\0';

	rc = sscanf(Disk_buf, "%u %u %u %u" " %u %u %u %u" " %u %u %u\n",
		&Disk_new.reads, &Disk_new.reads_merged,
		&Disk_new.sectors_read, &Disk_new.msecs_reading,

		&Disk_new.writes, &Disk_new.writes_merged,
		&Disk_new.sectors_written, &Disk_new.msecs_writing,

		&Disk_new.ios_in_progress, &Disk_new.msecs_in_progress,
		&Disk_new.msecs_weighted);
	close(fd);
}

void init_disk (char *disk)
{
	static bool	inited = FALSE;

	if (inited) return;
	inited = TRUE;

	init();

	Disk = disk;
	read_disk();
}

static inline void disk_update (double x)
{
	if (x > Disk_x) {
		Disk_x = x;
		Disk_old = Disk_new;
		read_disk();
	}
}

#define DISK(_a)					\
	double disk_ ## _a (double x, int id)		\
	{						\
		unused(id);				\
		disk_update(x);				\
		return Disk_new._a - Disk_old._a;	\
	}

#define PROGRESS(_a)					\
	double disk_ ## _a (double x, int id)		\
	{						\
		unused(id);				\
		disk_update(x);				\
		return Disk_new._a;			\
	}

DISK(reads)
DISK(reads_merged);
DISK(sectors_read);
DISK(msecs_reading);

DISK(writes);
DISK(writes_merged);
DISK(sectors_written);
DISK(msecs_writing);

PROGRESS(ios_in_progress);
PROGRESS(msecs_in_progress);
PROGRESS(msecs_weighted);


/***************************************************************************/
typedef struct Part_s {
	unsigned	reads;
	unsigned	sectors_read;
	unsigned	writes;
	unsigned	sectors_written;
} Part_s;

Part_s	Part_old, Part_new;
char	Part_buf[4096];
double	Part_x = -1.0;
char	*Partition;

static void read_part (void)
{
	int	fd;
	int	rc;

	fd = open(Partition, O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(2);
	}

	rc = read(fd, Part_buf, sizeof(Part_buf));
	if (rc == -1) {
		perror("read");
		exit(2);
	}
	Part_buf[rc] = '\0';

	rc = sscanf(Part_buf, "%u %u %u %u\n",
		&Part_new.reads, &Part_new.sectors_read,
		&Part_new.writes, &Part_new.sectors_written);
	close(fd);
}

void init_part (char *partition)
{
	static bool	inited = FALSE;

	if (inited) return;
	inited = TRUE;

	init();

	Partition = partition;
	read_part();
}

static inline void part_update (double x)
{
	if (x > Part_x) {
		Part_x = x;
		Part_old = Part_new;
		read_part();
	}
}

double part_reads (double x, int id)
{
	unused(id);
	part_update(x);
	return Part_new.reads - Part_old.reads;
}

double part_sectors_read (double x, int id)
{
	unused(id);
	part_update(x);
	return (Part_new.sectors_read - Part_old.sectors_read)/8;
}

double part_writes (double x, int id)
{
	unused(id);
	part_update(x);
	return Part_new.writes - Part_old.writes;
}

double part_sectors_written (double x, int id)
{
	unused(id);
	part_update(x);
	return (Part_new.sectors_written - Part_old.sectors_written)/8;
}

/********************************************************************/

void start_barrier (Plotter &plotter)
{
	int	color = 0;

	init_barrier();
	init_ZlssDevIO();

	plotter.setWindowTitle(QObject::tr("Writes"));
	plotter.setCurveData(color++, pending_writes);
	plotter.setCurveData(color++, writes_finished);
	plotter.setCurveData(color++, barrier);
	plotter.show();
}

void start_inst_write (Plotter &plotter)
{
	int	color = 0;

	init_IOsInst();

	plotter.setWindowTitle(QObject::tr("IOsInst Write"));
	plotter.setCurveData(color++, IOsInst_writeTotalIn);
	plotter.setCurveData(color++, IOsInst_writeTotalOut);
	plotter.setCurveData(color++, IOsInst_pendingTotalWrites);
	plotter.show();
}

void start_zlss_writes (Plotter &plotter)
{
	int	color = 0;

	init_ZlssDevIO();

	plotter.setWindowTitle(QObject::tr("ZlssDevIO"));
	plotter.setCurveData(color++, writes_started);
	plotter.setCurveData(color++, writes_finished);
	plotter.setCurveData(color++, pending_writes);
	plotter.show();
}

void start_disk_io (Plotter &plotter)
{
	int	color = 0;

	init_disk("/sys/block/hda/stat");

	/* Data from the /sys/block/hda */
	plotter.setWindowTitle(QObject::tr("Disk I/O"));
	plotter.setCurveData(color++, disk_sectors_read);
	plotter.setCurveData(color++, disk_sectors_written);
	plotter.setCurveData(color++, disk_ios_in_progress);
	plotter.show();
}

void start_partition_io (Plotter &plotter)
{
	int	color = 0;

	init_part("/sys/block/hda/hda3/stat");

	plotter.setWindowTitle(QObject::tr("Partition I/O"));
	plotter.setCurveData(color++, part_reads);
	plotter.setCurveData(color++, part_writes);
	plotter.setCurveData(color++, part_sectors_read);
	plotter.setCurveData(color++, part_sectors_written);
	plotter.show();
}

void start_write_io (Plotter &plotter)
{
	int	color = 0;

	init_IOsInst();

	plotter.setWindowTitle(QObject::tr("IOsInst Write"));
	plotter.setCurveData(color++, IOsInst_writeIn);
	plotter.setCurveData(color++, IOsInst_writeOut);
	plotter.setCurveData(color++, IOsInst_pendingWrites);
	plotter.setCurveData(color++, IOsInst_writeInXdata);
	plotter.setCurveData(color++, IOsInst_writeOutXdata);
	plotter.setCurveData(color++, IOsInst_pendingWritesX);
	plotter.show();
}

void start_read_io (Plotter &plotter)
{
	int	color = 0;

	init_IOsInst();

	plotter.setWindowTitle(QObject::tr("IOsInst Read"));
	plotter.setCurveData(color++, IOsInst_readIn);
	plotter.setCurveData(color++, IOsInst_readOut);
	plotter.setCurveData(color++, IOsInst_pendingReads);
	plotter.setCurveData(color++, IOsInst_readWIn);
	plotter.setCurveData(color++, IOsInst_readWOut);
	plotter.setCurveData(color++, IOsInst_pendingReadsW);
	plotter.show();
}

void start_writes (Plotter &plotter)
{
	int	color = 0;

	init_IOsInst();
	init_barrier();

	plotter.setWindowTitle(QObject::tr("Writes"));
	plotter.setCurveData(color++, pending_writes);
	plotter.setCurveData(color++, writes_finished);
	plotter.setCurveData(color++, waiting);
	plotter.setCurveData(color++, pjt_xlatch);
	plotter.setCurveData(color++, barrier);
	plotter.setCurveData(color++, pjt_nobufs);
	plotter.show();
}

void start_zlss_io (Plotter &plotter)
{
	int	color = 0;

	init_ZlssDevIO();
	init_barrier();

	plotter.setWindowTitle(QObject::tr("ZlssDevIO"));
	plotter.setCurveData(color++, pending_reads);
	plotter.setCurveData(color++, pending_writes);
	plotter.setCurveData(color++, reads_started);
	plotter.setCurveData(color++, writes_started);
	plotter.setCurveData(color++, reads_finished);
	plotter.setCurveData(color++, writes_finished);
	plotter.show();
}

void start_pending_reads (Plotter &plotter)
{
	init_ZlssDevIO();

	plotter.setWindowTitle(QObject::tr("Pending Reads"));
	plotter.setCurveData(0, pending_reads);
	plotter.show();
}

void start_pending_writes (Plotter &plotter)
{
	init_ZlssDevIO();

	plotter.setWindowTitle(QObject::tr("Pending Writes"));
	plotter.setCurveData(0, pending_writes);
	plotter.show();
}

void start_reads_finished (Plotter &plotter)
{
	init_ZlssDevIO();

	plotter.setWindowTitle(QObject::tr("Finished Reads"));
	plotter.setCurveData(3, reads_finished);
	plotter.show();
}

void start_writes_finished (Plotter &plotter)
{
	init_ZlssDevIO();

	plotter.setWindowTitle(QObject::tr("Finished Writes"));
	plotter.setCurveData(4, writes_finished);
	plotter.show();
}

void start_waiting (Plotter &plotter)
{
	init_ZlssDevIO();

	plotter.setWindowTitle(QObject::tr("Waiting"));
	plotter.setCurveData(5, waiting);
	plotter.show();
}

/********************************************************************/

enum { MAX_KVAR = 6 };

typedef struct kvar_s	kvar_s;
struct kvar_s {
	char	*kv_name;
	addr	kv_address;
	u64	kv_old;
	u64	kv_new;
	double	kv_x;
};

static kvar_s	Kvar[MAX_KVAR];
static int	Knext = 0;

static void init_kvar (void)
{
	static bool	inited = FALSE;

	if (inited) return;
	inited = TRUE;

	init();
}

static void alloc_kvar (char *arg)
{
	kvar_s	*kv;

	if (Knext >= MAX_KVAR) {
		fprintf(stderr, "Can only plot %d kernel variables\n", MAX_KVAR);
		return;
	}

	kv = &Kvar[Knext++];

	kv->kv_name = arg;
	kv->kv_address = getaddr(arg);
	if (!kv->kv_address) {
		fprintf(stderr, "Couldn't find %s\n", arg);
		return;
	}
	readkmem(kv->kv_address, &kv->kv_new, sizeof(long));
}

static void register_kernel_variables (QApplication &a)
{
	init_kvar();

	for (int i = 1; i < a.argc(); i++) {		// a.argc() == argc
		printf("%d. %s\n", i, a.argv()[i]);	// a.argv()[i] == argv[i]
		alloc_kvar(a.argv()[i]);
	}
}

static double update_kvar (double x, int id)
{
	kvar_s	*kv;

	if (id > Knext) return 0.0;

	kv = &Kvar[id];
	if (x > kv->kv_x) {
		kv->kv_x = x;
		kv->kv_old = kv->kv_new;
		readkmem(kv->kv_address, &kv->kv_new, sizeof(long));
	}
	return kv->kv_new - kv->kv_old;
}

void start_kernel_variables (QApplication &a, Plotter &plotter)
{
	int	id;

	register_kernel_variables(a);

	plotter.setWindowTitle(QObject::tr("Kernel Variables"));
	for (id = 0; id < Knext; id++) {
		plotter.setCurveData(id, update_kvar);
	}
	plotter.show();
}

