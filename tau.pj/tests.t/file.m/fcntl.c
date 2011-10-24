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
 * FCNTL(2)                  Linux Programmer's Manual                  FCNTL(2)
 *
 *
 *
 * NAME
 *        fcntl - manipulate file descriptor
 *
 * SYNOPSIS
 *        #include <unistd.h>
 *        #include <fcntl.h>
 *
 *        int fcntl(int fd, int cmd);
 *        int fcntl(int fd, int cmd, long arg);
 *        int fcntl(int fd, int cmd, struct flock *lock);
 *
 * DESCRIPTION
 *        fcntl  performs  one  of  various miscellaneous operations on fd.  The
 *        operation in question is determined by cmd.
 *
 *    Handling close-on-exec
 *        F_DUPFD
 *               Find the lowest numbered available file descriptor greater than
 *               or equal to arg and make it be a copy of fd.  This is different
 *               form dup2(2) which uses exactly the descriptor specified.
 *
 *               The old and new descriptors may be used  interchangeably.  They
 *               share  locks, file position pointers and flags; for example, if
 *               the file position is modified by using  lseek  on  one  of  the
 *               descriptors, the position is also changed for the other.
 *
 *               The  two  descriptors do not share the close-on-exec flag, how-
 *               ever.  The close-on-exec flag of the copy is off, meaning  that
 *               it will not be closed on exec.
 *
 *               On success, the new descriptor is returned.
 *
 *        F_GETFD
 *               Read  the  close-on-exec flag.  If the FD_CLOEXEC bit is 0, the
 *               file will remain open across exec, otherwise it will be closed.
 *
 *        F_SETFD
 *               Set  the  close-on-exec  flag  to  the  value  specified by the
 *               FD_CLOEXEC bit of arg.
 *
 *    The file status flags
 *        A file descriptor has certain associated flags, initialized by open(2)
 *        and  possibly  modified  by  fcntl(2).   The  flags are shared between
 *        copies (made with dup(2), fork(2), etc.) of the same file  descriptor.
 *
 *        The flags and their semantics are described in open(2).
 *
 *        F_GETFL
 *               Read the file descriptor's flags.
 *
 *        F_SETFL
 *               Set the file status flags part of the descriptor's flags to the
 *               value specified by arg.  Remaining bits (access mode, file cre-
 *               ation  flags)  in  arg  are ignored.  On Linux this command can
 *               only change the O_APPEND,  O_NONBLOCK,  O_ASYNC,  and  O_DIRECT
 *               flags.
 *
 *
 *    Advisory locking
 *        F_GETLK,  F_SETLK  and F_SETLKW are used to acquire, release, and test
 *        for the existence of record locks (also known as file-segment or file-
 *        region  locks).   The  third argument lock is a pointer to a structure
 *        that has at least the following fields (in unspecified order).
 *
 *          struct flock {
 *              ...
 *              short l_type;    // Type of lock: F_RDLCK, F_WRLCK, F_UNLCK
 *              short l_whence;  // How to interpret l_start:
 *                               //     SEEK_SET, SEEK_CUR, SEEK_END
 *              off_t l_start;   // Starting offset for lock
 *              off_t l_len;     // Number of bytes to lock
 *              pid_t l_pid;     // PID of process blocking our lock
 *                               // (F_GETLK only)
 *              ...
 *          };
 *
 *        The l_whence, l_start, and l_len fields of this structure specify  the
 *        range  of  bytes  we wish to lock.  l_start is the starting offset for
 *        the lock, and is interpreted relative to either: the start of the file
 *        (if  l_whence  is  SEEK_SET);  the current file offset (if l_whence is
 *        SEEK_CUR); or the end of the file (if l_whence is SEEK_END).   In  the
 *        final  two cases, l_start can be a negative number provided the offset
 *        does not lie before the start of the file.  l_len  is  a  non-negative
 *        integer (but see the NOTES below) specifying the number of bytes to be
 *        locked.  Bytes past the end of the file may be locked, but  not  bytes
 *        before  the start of the file.  Specifying 0 for l_len has the special
 *        meaning: lock all bytes starting at the location specified by l_whence
 *        and  l_start  through to the end of file, no matter how large the file
 *        grows.
 *
 *        The l_type field can be used to place a  read  (F_RDLCK)  or  a  write
 *        (F_WDLCK)  lock  on  a  file.  Any number of processes may hold a read
 *        lock (shared lock) on a file region, but only one process may  hold  a
 *        write  lock  (exclusive  lock).  An  exclusive lock excludes all other
 *        locks, both shared and exclusive.  A single process can hold only  one
 *        type of lock on a file region; if a new lock is applied to an already-
 *        locked region, then the existing lock is converted to the the new lock
 *        type.  (Such conversions may involve splitting, shrinking, or coalesc-
 *        ing with an existing lock if the byte range specified by the new  lock
 *        does not precisely coincide with the range of the existing lock.)
 *
 *        F_SETLK
 *               Acquire a lock (when l_type is F_RDLCK or F_WRLCK) or release a
 *               lock (when l_type is F_UNLCK) on the  bytes  specified  by  the
 *               l_whence,  l_start, and l_len fields of lock.  If a conflicting
 *               lock is held by another process, this call returns -1 and  sets
 *               errno to EACCES or EAGAIN.
 *
 *        F_SETLKW
 *               As  for F_SETLK, but if a conflicting lock is held on the file,
 *               then wait for that lock to be released.  If a signal is  caught
 *               while waiting, then the call is interrupted and (after the sig-
 *               nal handler has  returned)  returns  immediately  (with  return
 *               value -1 and errno set to EINTR).
 *
 *        F_GETLK
 *               On  input  to this call, lock describes a lock we would like to
 *               place on the file.  If the lock could be placed,  fcntl()  does
 *               not  actually place it, but returns F_UNLCK in the l_type field
 *               of lock and leaves the other fields of the structure unchanged.
 *               If one or more incompatible locks would prevent this lock being
 *               placed, then fcntl() returns details about one of  these  locks
 *               in  the l_type, l_whence, l_start, and l_len fields of lock and
 *               sets l_pid to be the PID of the process holding that lock.
 *
 *        In order to place a read lock, fd must be open for reading.  In  order
 *        to  place  a  write  lock, fd must be open for writing.  To place both
 *        types of lock, open a file read-write.
 *
 *        As well as being removed by an  explicit  F_UNLCK,  record  locks  are
 *        automatically released when the process terminates or if it closes any
 *        file descriptor referring to a file on which locks are held.  This  is
 *        bad:  it  means  that  a  process  can  lose  the locks on a file like
 *        /etc/passwd or /etc/mtab when  for  some  reason  a  library  function
 *        decides to open, read and close it.
 *
 *        Record locks are not inherited by a child created via fork(2), but are
 *        preserved across an execve(2).
 *
 *        Because of the buffering performed by the stdio(3) library, the use of
 *        record  locking  with  routines in that package should be avoided; use
 *        read(2) and write(2) instead.
 *
 *
 *    Mandatory locking
 *        (Non-POSIX.)  The above record locks may be either advisory or  manda-
 *        tory,  and  are  advisory by default.  To make use of mandatory locks,
 *        mandatory locking must be enabled  (using  the  "-o  mand"  option  to
 *        mount(8))  for  the  file  system containing the file to be locked and
 *        enabled on the file itself (by disabling group execute  permission  on
 *        the file and enabling the set-GID permission bit).
 *
 *        Advisory  locks are not enforced and are useful only between cooperat-
 *        ing processes. Mandatory locks are enforced for all processes.
 *
 *
 *    Managing signals
 *        F_GETOWN, F_SETOWN, F_GETSIG and  F_SETSIG  are  used  to  manage  I/O
 *        availability signals:
 *
 *        F_GETOWN
 *               Get  the  process ID or process group currently receiving SIGIO
 *               and SIGURG signals for events on file descriptor  fd.   Process
 *               groups are returned as negative values.
 *
 *        F_SETOWN
 *               Set the process ID or process group that will receive SIGIO and
 *               SIGURG signals for  events  on  file  descriptor  fd.   Process
 *               groups  are  specified using negative values.  (F_SETSIG can be
 *               used to specify a different signal instead of SIGIO).
 *
 *               If you set the O_ASYNC status flag on a file descriptor (either
 *               by  providing  this flag with the open(2) call, or by using the
 *               F_SETFL command of fcntl), a  SIGIO  signal  is  sent  whenever
 *               input or output becomes possible on that file descriptor.
 *
 *               The  process  or  process  group  to  receive the signal can be
 *               selected by using the F_SETOWN command to the  fcntl  function.
 *               If  the  file  descriptor  is  a  socket, this also selects the
 *               recipient of SIGURG signals that are delivered when out-of-band
 *               data  arrives on that socket.  (SIGURG is sent in any situation
 *               where select(2) would report the socket as  having  an  "excep-
 *               tional  condition".)   If  the file descriptor corresponds to a
 *               terminal device, then SIGIO signals are sent to the  foreground
 *               process group of the terminal.
 *
 *        F_GETSIG
 *               Get  the  signal sent when input or output becomes possible.  A
 *               value of zero means SIGIO is sent.  Any other value  (including
 *               SIGIO)  is the signal sent instead, and in this case additional
 *               info is available to  the  signal  handler  if  installed  with
 *               SA_SIGINFO.
 *
 *        F_SETSIG
 *               Sets  the signal sent when input or output becomes possible.  A
 *               value of zero means to send  the  default  SIGIO  signal.   Any
 *               other  value  (including  SIGIO) is the signal to send instead,
 *               and in this case additional info is  available  to  the  signal
 *               handler if installed with SA_SIGINFO.
 *
 *               By using F_SETSIG with a non-zero value, and setting SA_SIGINFO
 *               for the signal handler (see  sigaction(2)),  extra  information
 *               about I/O events is passed to the handler in a siginfo_t struc-
 *               ture.  If the si_code field indicates the source  is  SI_SIGIO,
 *               the  si_fd  field gives the file descriptor associated with the
 *               event.  Otherwise, there is no indication which  file  descrip-
 *               tors  are  pending,  and  you  should  use the usual mechanisms
 *               (select(2), poll(2),  read(2)  with  O_NONBLOCK  set  etc.)  to
 *               determine which file descriptors are available for I/O.
 *
 *               By  selecting  a POSIX.1b real time signal (value >= SIGRTMIN),
 *               multiple I/O events may be queued using the  same  signal  num-
 *               bers.   (Queuing  is  dependent  on  available  memory).  Extra
 *               information is available if SA_SIGINFO is set  for  the  signal
 *               handler, as above.
 *
 *        Using these mechanisms, a program can implement fully asynchronous I/O
 *        without using select(2) or poll(2) most of the time.
 *
 *        The use of O_ASYNC, F_GETOWN, F_SETOWN is specific to BSD  and  Linux.
 *        F_GETSIG  and F_SETSIG are Linux-specific.  POSIX has asynchronous I/O
 *        and the aio_sigevent structure to achieve similar  things;  these  are
 *        also available in Linux as part of the GNU C Library (Glibc).
 *
 *
 *    Leases
 *        F_SETLEASE  and F_GETLEASE (Linux 2.4 onwards) are used (respectively)
 *        to establish and retrieve the current setting of the calling process's
 *        lease  on  the file referred to by fd.  A file lease provides a mecha-
 *        nism whereby the process holding the lease  (the  "lease  holder")  is
 *        notified  (via  delivery of a signal) when another process (the "lease
 *        breaker") tries to open(2) or truncate(2) that file.
 *
 *        F_SETLEASE
 *               Set or remove a file lease according to which of the  following
 *               values is specified in the integer arg:
 *
 *
 *               F_RDLCK
 *                      Take  out  a read lease.  This will cause us to be noti-
 *                      fied when another process opens the file for writing  or
 *                      truncates it.
 *
 *               F_WRLCK
 *                      Take  out a write lease.  This will cause us to be noti-
 *                      fied when another process opens the file (for reading or
 *                      writing)  or  truncates it.  A write lease may be placed
 *                      on a file only if no other  process  currently  has  the
 *                      file open.
 *
 *               F_UNLCK
 *                      Remove our lease from the file.
 *
 *        A process may hold only one type of lease on a file.
 *
 *        Leases  may  only be taken out on regular files.  An unprivileged pro-
 *        cess may only take out a lease on a file whose UID  matches  the  file
 *        system UID of the process.
 *
 *        F_GETLEASE
 *               Indicates what type of lease we hold on the file referred to by
 *               fd by returning either F_RDLCK, F_WRLCK, or  F_UNLCK,  indicat-
 *               ing,  respectively,  that  the  calling process holds a read, a
 *               write, or no lease on the file.  (The third argument to fcntl()
 *               is omitted.)
 *
 *        When  a process (the "lease breaker") performs an open() or truncate()
 *        that conflicts with a lease established  via  F_SETLEASE,  the  system
 *        call  is  blocked by the kernel, unless the O_NONBLOCK flag was speci-
 *        fied to open(), in which case the system call  will  return  with  the
 *        error EWOULDBLOCK.  The kernel notifies the lease holder by sending it
 *        a signal (SIGIO by default).   The  lease  holder  should  respond  to
 *        receipt of this signal by doing whatever cleanup is required in prepa-
 *        ration for the file to be accessed by another process (e.g.,  flushing
 *        cached  buffers)  and  then  either  remove or downgrade its lease.  A
 *        lease is removed by performing an F_SETLEASE command specifying arg as
 *        F_UNLCK.   If  we  currently  hold  a write lease on the file, and the
 *        lease breaker is opening the file for reading, then it  is  sufficient
 *        to downgrade the lease to a read lease.  This is done by performing an
 *        F_SETLEASE command specifying arg as F_RDLCK.
 *
 *        If the lease holder fails to downgrade or remove the lease within  the
 *        number  of seconds specified in /proc/sys/fs/lease-break-time then the
 *        kernel forcibly removes or downgrades the lease holder's lease.
 *
 *        Once the lease has been voluntarily or forcibly removed or downgraded,
 *        and  assuming the lease breaker has not unblocked its system call, the
 *        kernel permits the lease breaker's system call to proceed.
 *
 *        The default signal used to notify the lease holder is SIGIO, but  this
 *        can  be changed using the F_SETSIG command to fcntl ().  If a F_SETSIG
 *        command is performed (even one specifying SIGIO), and the signal  han-
 *        dler  is established using SA_SIGINFO, then the handler will receive a
 *        siginfo_t sructure as its second argument, and the si_fd field of this
 *        argument  will  hold  the  descriptor of the leased file that has been
 *        accessed by another process.  (This is  useful  if  the  caller  holds
 *        leases against multiple files).
 *
 *    File and directory change notification
 *        F_NOTIFY
 *               (Linux  2.4  onwards)  Provide  notification when the directory
 *               referred to by fd or any of  the  files  that  it  contains  is
 *               changed.  The events to be notified are specified in arg, which
 *               is a bit mask specified by ORing together zero or more  of  the
 *               following bits:
 *
 *
 *               Bit         Description (event in directory)
 *               -------------------------------------------------------------
 *               DN_ACCESS   A file was accessed (read, pread, readv)
 *               DN_MODIFY   A file was modified (write, pwrite,
 *                           writev, truncate, ftruncate)
 *               DN_CREATE   A file was created (open, creat, mknod,
 *                           mkdir, link, symlink, rename)
 *               DN_DELETE   A file was unlinked (unlink, rename to
 *                           another directory, rmdir)
 *               DN_RENAME   A file was renamed within this
 *                           directory (rename)
 *               DN_ATTRIB   The attributes of a file were changed
 *                           (chown, chmod, utime[s])
 *
 *               (In  order  to  obtain these definitions, the _GNU_SOURCE macro
 *               must be defined before including <fcntl.h>.)
 *
 *               Directory  notifications  are  normally  "one-shot",  and   the
 *               application  must re-register to receive further notifications.
 *               Alternatively, if DN_MULTISHOT is included in arg, then notifi-
 *               cation will remain in effect until explicitly removed.
 *
 *               A series of F_NOTIFY requests is cumulative, with the events in
 *               arg being added to the set already monitored.  To disable noti-
 *               fication of all events, make an F_NOTIFY call specifying arg as
 *               0.
 *
 *               Notification occurs via delivery of a signal.  The default sig-
 *               nal  is  SIGIO, but this can be changed using the F_SETSIG com-
 *               mand to fcntl().   In  the  latter  case,  the  signal  handler
 *               receives  a  siginfo_t structure as its second argument (if the
 *               handler was established using SA_SIGINFO) and the  si_fd  field
 *               of  this structure contains the file descriptor which generated
 *               the notification (useful when establishing notification on mul-
 *               tiple directories).
 *
 *               Especially when using DN_MULTISHOT, a POSIX.1b real time signal
 *               should be used for notication, so that  multiple  notifications
 *               can be queued.
 *
 * RETURN VALUE
 *        For a successful call, the return value depends on the operation:
 *
 *        F_DUPFD  The new descriptor.
 *
 *        F_GETFD  Value of flag.
 *
 *        F_GETFL  Value of flags.
 *
 *        F_GETOWN Value of descriptor owner.
 *
 *        F_GETSIG Value  of signal sent when read or write becomes possible, or
 *                 zero for traditional SIGIO behaviour.
 *
 *        All other commands
 *                 Zero.
 *
 *        On error, -1 is returned, and errno is set appropriately.
 *
 * ERRORS
 *        EACCES or EAGAIN
 *               Operation is prohibited by locks held by other processes.   Or,
 *               operation is prohibited because the file has been memory-mapped
 *               by another process.
 *
 *        EBADF  fd is not an open file descriptor, or the command  was  F_SETLK
 *               or  F_SETLKW  and  the  file descriptor open mode doesn't match
 *               with the type of lock requested.
 *
 *        EDEADLK
 *               It was detected that the specified F_SETLKW command would cause
 *               a deadlock.
 *
 *        EFAULT lock is outside your accessible address space.
 *
 *        EINTR  For  F_SETLKW,  the  command  was interrupted by a signal.  For
 *               F_GETLK and F_SETLK, the command was interrupted  by  a  signal
 *               before  the  lock  was  checked  or acquired.  Most likely when
 *               locking a remote file (e.g. locking over NFS),  but  can  some-
 *               times happen locally.
 *
 *        EINVAL For  F_DUPFD,  arg  is  negative or is greater than the maximum
 *               allowable value.  For F_SETSIG, arg is not an allowable  signal
 *               number.
 *
 *        EMFILE For F_DUPFD, the process already has the maximum number of file
 *               descriptors open.
 *
 *        ENOLCK Too many segment locks open, lock table is full,  or  a  remote
 *               locking protocol failed (e.g. locking over NFS).
 *
 *        EPERM  Attempted  to  clear  the  O_APPEND flag on a file that has the
 *               append-only attribute set.
 *
 * NOTES
 *        The errors returned by dup2  are  different  from  those  returned  by
 *        F_DUPFD.
 *
 *        Since  kernel  2.0,  there is no interaction between the types of lock
 *        placed by flock(2) and fcntl(2).
 *
 *        POSIX 1003.1-2001 allows l_len to be negative.  (And  if  it  is,  the
 *        interval  described  by  the lock covers bytes l_start+l_len up to and
 *        including l_start-1.)  This is supported by Linux since  Linux  2.4.21
 *        and 2.5.49.
 *
 *        Several  systems  have  more  fields  in  struct  flock  such  as e.g.
 *        l_sysid.  Clearly, l_pid alone is not going to be very useful  if  the
 *        process holding the lock may live on a different machine.
 *
 *
 * CONFORMING TO
 *        SVr4,  SVID,  POSIX,  X/OPEN,  BSD  4.3.  Only the operations F_DUPFD,
 *        F_GETFD, F_SETFD, F_GETFL, F_SETFL, F_GETLK, F_SETLK and F_SETLKW  are
 *        specified in POSIX.1.  F_GETOWN and F_SETOWN are BSDisms not supported
 *        in SVr4; F_GETSIG and  F_SETSIG  are  specific  to  Linux.   F_NOTIFY,
 *        F_GETLEASE,   and   F_SETLEASE   are   Linux  specific.   (Define  the
 *        _GNU_SOURCE macro before including <fcntl.h> to obtain  these  defini-
 *        tions.)   The  flags  legal for F_GETFL/F_SETFL are those supported by
 *        open(2)  and  vary  between  these  systems;   O_APPEND,   O_NONBLOCK,
 *        O_RDONLY,  and O_RDWR are specified in POSIX.1.  SVr4 supports several
 *        other options and flags not documented here.
 *
 *        SVr4 documents additional EIO, ENOLINK and EOVERFLOW error conditions.
 *
 * SEE ALSO
 *        dup2(2), flock(2), lockf(3), open(2), socket(2)
 *
 *        See    also    locks.txt,    mandatory.txt,    and    dnotify.txt   in
 *        /usr/src/linux/Documentation.
 *
 *
 *
 * Linux-2.6.3                       2004-03-03
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <style.h>
#include <mystdlib.h>
#include <eprintf.h>
#include <puny.h>

#ifdef __APPLE__
int main (int argc, char *argv[])
{
	fatal("Leases not available on APPLE");
	return 0;
}

#else

typedef enum Cmd_t {
	cEND = 0,
	cOPEN,
	cCLOSE,
	cREAD,
	cWRITE,
	cEXIT
} Cmd_t;

typedef struct Msg_s {
	Cmd_t	m_cmd;
} Msg_s;

int	Command[2];
int	Response[2];


void rtncmd (Msg_s *msg)
{
	int	rc;

	rc = write(Response[1], msg, sizeof(*msg));
	if (rc == -1) {
		perror("rtncmd");
		exit(1);
	}
}

void getcmd (Msg_s *msg)
{
	int	rc;

	rc = read(Command[0], msg, sizeof(*msg));
	if (rc == -1) {
		perror("getcmd");
		exit(1);
	}
}

void sendcmd (Msg_s *msg)
{
	int	rc;

	rc = write(Command[1], msg, sizeof(*msg));
	if (rc == -1) {
		perror("call:write commmand");
		exit(1);
	}
	rc = read(Response[0], msg, sizeof(*msg));
	if ((rc == -1) || (rc == 0)) {
		perror("call:receive response");
		exit(1);
	}
}


void child (char *name, unsigned n)
{
	Msg_s	msg;
	int	fd = -1;

	close(Command[1]);
	close(Response[0]);

	for (;;) {
		getcmd( &msg);
		printf("child command=%d\n", msg.m_cmd);
		switch (msg.m_cmd) {
		case cOPEN:
			fd = open(name, O_RDWR);
			if (fd == -1) {
				eprintf("Couldn't create %s:", name);
			}
			break;
		case cCLOSE:
			close(fd);
			break;
		case cREAD:
		case cWRITE:
		case cEND:
		case cEXIT:
			rtncmd( &msg);
			exit(0);
			break;
		default:
			eprintf("Unknown command = %d\n", msg.m_cmd);
			break;
		}
		rtncmd( &msg);
	}
}

Cmd_t	Cmd[] = {
	cOPEN,
	cEXIT,
	cEND
};

void parent_act (int signum, siginfo_t *info, void *dontknow)
{
	fprintf(stderr, "Received signal %d on fd=%d\n", signum, info->si_fd);
	close(info->si_fd);
}


void parent (char *name, unsigned n)
{
	struct sigaction	act;
	Cmd_t	*cmd;
	Msg_s	msg;
	int	status;
	pid_t	pid;
	int	rc;
	int	fd;
#if 0
	int	i;

	close(fd);
#endif
	close(Command[0]);
	close(Response[1]);

	bzero( &act, sizeof(act));
	act.sa_sigaction = parent_act;

	rc = sigaction(SIGIO, &act, NULL);
	if (rc) eprintf("sigaction=%d:", rc);


	for (cmd = Cmd; *cmd; cmd++) {
		fprintf(stderr, "parent cmd=%d\n", *cmd);
		switch (*cmd) {
		case cOPEN:
			fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);
			if (fd == -1) {
				eprintf("Couldn't create %s:", name);
			}
			rc = fcntl(fd, F_SETLEASE, F_WRLCK);
			if (rc == -1) {
				eprintf("parent fcntl:");
			}
			break;
		case cCLOSE:
		case cREAD:
		case cWRITE:
		case cEND:
		case cEXIT:
			exit(0);
			break;
		default:
			eprintf("Unknown command = %d\n", msg.m_cmd);
			break;
		}
		msg.m_cmd = *cmd;
		sendcmd( &msg);
		printf("Response=%d\n", msg.m_cmd);
	}

printf("going to wait\n");
	pid = wait( &status);
	printf("pid:%d status=%d\n", pid, status>>8);
}

void usage (void)
{
	pr_usage("-f<file_name> -i<num_iterations>");
}

int main (int argc, char *argv[])
{
	pid_t	pid;
	int	rc;

	punyopt(argc, argv, NULL, NULL);

	rc = pipe(Command);
	if (rc == -1) {
		perror("Command pipe");
	}
	rc = pipe(Response);
	if (rc == -1) {
		perror("Response pipe");
	}

	pid = fork();
	if (pid == -1) {
		perror("fork");
		return 1;
	}
	if (pid) {
		parent(Option.file, Option.iterations);
	} else {
		child(Option.file, Option.iterations);
	}
	return 0;
}
#endif
