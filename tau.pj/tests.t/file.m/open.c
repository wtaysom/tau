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
 * OPEN(2)                       System calls                       OPEN(2)
 * 
 * 
 * 
 * NAME
 *        open, creat - open and possibly create a file or device
 * 
 * SYNOPSIS
 *        #include <sys/types.h>
 *        #include <sys/stat.h>
 *        #include <fcntl.h>
 * 
 *        int open(const char *pathname, int flags);
 *        int open(const char *pathname, int flags, mode_t mode);
 *        int creat(const char *pathname, mode_t mode);
 * 
 * DESCRIPTION
 *        The  open() system call is used to convert a pathname into a file
 *        descriptor (a small, non-negative integer for use  in  subsequent
 *        I/O as with read, write, etc.).  When the call is successful, the
 *        file descriptor returned will be the lowest file  descriptor  not
 *        currently  open  for  the  process.  This call creates a new open
 *        file, not shared with any other process.  (But shared open  files
 *        may  arise via the fork(2) system call.)  The new file descriptor
 *        is set to remain open across exec functions (see fcntl(2)).   The
 *        file offset is set to the beginning of the file.
 * 
 *        The  parameter flags is one of O_RDONLY, O_WRONLY or O_RDWR which
 *        request opening the file  read-only,  write-only  or  read/write,
 *        respectively, bitwise-or'd with zero or more of the following:
 * 
 *        O_CREAT
 *               If  the file does not exist it will be created.  The owner
 *               (user ID) of the file is set to the effective user  ID  of
 *               the  process. The group ownership (group ID) is set either
 *               to the effective group ID of the process or to  the  group
 *               ID  of  the parent directory (depending on filesystem type
 *               and mount options, and the mode of the  parent  directory,
 *               see,  e.g.,  the mount options bsdgroups and sysvgroups of
 *               the ext2 filesystem, as described in mount(8)).
 * 
 *        O_EXCL When used with O_CREAT, if the file already exists  it  is
 *               an  error  and the open will fail. In this context, a sym?
 *               bolic link exists, regardless  of  where  its  points  to.
 *               O_EXCL  is broken on NFS file systems, programs which rely
 *               on it for performing locking tasks  will  contain  a  race
 *               condition.   The solution for performing atomic file lock?
 *               ing using a lockfile is to create a  unique  file  on  the
 *               same  fs  (e.g.,  incorporating  hostname  and  pid),  use
 *               link(2) to make a link to the lockfile. If link()  returns
 *               0,  the lock is successful.  Otherwise, use stat(2) on the
 *               unique file to check if its link count has increased to 2,
 *               in which case the lock is also successful.
 * 
 *        O_NOCTTY
 *               If  pathname refers to a terminal device ? see tty(4) ? it
 *               will not become the process's controlling terminal even if
 *               the process does not have one.
 * 
 *        O_TRUNC
 *               If  the  file already exists and is a regular file and the
 *               open mode allows writing (i.e., is O_RDWR or O_WRONLY)  it
 *               will  be  truncated to length 0.  If the file is a FIFO or
 *               terminal device file, the O_TRUNC flag is ignored.  Other?
 *               wise the effect of O_TRUNC is unspecified.
 * 
 *        O_APPEND
 *               The  file is opened in append mode. Before each write, the
 *               file pointer is positioned at the end of the file,  as  if
 *               with  lseek.   O_APPEND may lead to corrupted files on NFS
 *               file systems if more than one process appends  data  to  a
 *               file  at  once.   This  is  because  NFS  does not support
 *               appending to a file, so the client kernel has to  simulate
 *               it, which can't be done without a race condition.
 * 
 *        O_NONBLOCK or O_NDELAY
 *               When  possible,  the  file is opened in non-blocking mode.
 *               Neither the open nor any subsequent operations on the file
 *               descriptor  which  is returned will cause the calling pro?
 *               cess to wait.  For the handling of  FIFOs  (named  pipes),
 *               see  also  fifo(4).  This mode need not have any effect on
 *               files other than FIFOs.
 * 
 *        O_SYNC The file is opened for synchronous I/O. Any writes on  the
 *               resulting  file  descriptor will block the calling process
 *               until the data has been physically written to the underly?
 *               ing hardware.  See RESTRICTIONS below, though.
 * 
 *        O_NOFOLLOW
 *               If pathname is a symbolic link, then the open fails.  This
 *               is a FreeBSD extension, which was added to Linux  in  ver?
 *               sion 2.1.126.  Symbolic links in earlier components of the
 *               pathname will still be followed.  The headers  from  glibc
 *               2.0.100  and later include a definition of this flag; ker?
 *               nels before 2.1.126 will ignore it if used.
 * 
 *        O_DIRECTORY
 *               If pathname is not a directory, cause the  open  to  fail.
 *               This  flag is Linux-specific, and was added in kernel ver?
 *               sion  2.1.126,  to  avoid  denial-of-service  problems  if
 *               opendir(3)  is called on a FIFO or tape device, but should
 *               not be used outside of the implementation of opendir.
 * 
 *        O_DIRECT
 *               Try to minimize cache effects of the I/O to and from  this
 *               file.  In general this will degrade performance, but it is
 *               useful in special situations, such as when applications do
 *               their own caching.  File I/O is done directly to/from user
 *               space buffers.  The I/O is synchronous, i.e., at the  com?
 *               pletion  of  the  read(2) or write(2) system call, data is
 *               guaranteed to have  been  transferred.   Under  Linux  2.4
 *               transfer  sizes, and the alignment of user buffer and file
 *               offset must all be multiples of the logical block size  of
 *               the  file  system.  Under  Linux 2.6 alignment to 512-byte
 *               boundaries suffices.
 *               A semantically similar  interface  for  block  devices  is
 *               described in raw(8).
 * 
 *        O_ASYNC
 *               Generate  a  signal  (SIGIO  by  default,  but this can be
 *               changed via fcntl(2)) when input or output becomes  possi?
 *               ble  on this file descriptor.  This feature is only avail?
 *               able for terminals,  pseudo-terminals,  and  sockets.  See
 *               fcntl(2) for further details.
 * 
 *        O_LARGEFILE
 *               On  32-bit  systems  that  support the Large Files System,
 *               allow files whose sizes cannot be represented in  31  bits
 *               to be opened.
 * 
 *        Some of these optional flags can be altered using fcntl after the
 *        file has been opened.
 * 
 *        The argument mode specifies the permissions to use in case a  new
 *        file  is  created.  It  is modified by the process's umask in the
 *        usual way: the permissions  of  the  created  file  are  (mode  &
 *        ~umask).   Note that this mode only applies to future accesses of
 *        the newly created file; the open call that  creates  a  read-only
 *        file may well return a read/write file descriptor.
 * 
 *        The following symbolic constants are provided for mode:
 * 
 *        S_IRWXU
 *               00700  user  (file owner) has read, write and execute per?
 *               mission
 * 
 *        S_IRUSR (S_IREAD)
 *               00400 user has read permission
 * 
 *        S_IWUSR (S_IWRITE)
 *               00200 user has write permission
 * 
 *        S_IXUSR (S_IEXEC)
 *               00100 user has execute permission
 * 
 *        S_IRWXG
 *               00070 group has read, write and execute permission
 * 
 *        S_IRGRP
 *               00040 group has read permission
 * 
 *        S_IWGRP
 *               00020 group has write permission
 * 
 *        S_IXGRP
 *               00010 group has execute permission
 * 
 *        S_IRWXO
 *               00007 others have read, write and execute permission
 * 
 *        S_IROTH
 *               00004 others have read permission
 * 
 *        S_IWOTH
 *               00002 others have write permisson
 * 
 *        S_IXOTH
 *               00001 others have execute permission
 * 
 *        mode must be specified when O_CREAT  is  in  the  flags,  and  is
 *        ignored otherwise.
 * 
 *        creat    is    equivalent   to   open   with   flags   equal   to
 *        O_CREAT|O_WRONLY|O_TRUNC.
 * 
 * RETURN VALUE
 *        open and creat return the new file descriptor, or -1 if an  error
 *        occurred  (in which case, errno is set appropriately).  Note that
 *        open can open device special files, but creat cannot create  them
 *        - use mknod(2) instead.
 * 
 *        On  NFS  file systems with UID mapping enabled, open may return a
 *        file descriptor but e.g. read(2) requests are denied with EACCES.
 *        This  is because the client performs open by checking the permis?
 *        sions, but UID mapping is performed by the server upon  read  and
 *        write requests.
 * 
 *        If  the file is newly created, its atime, ctime, mtime fields are
 *        set to the current time, and so are the ctime and mtime fields of
 *        the parent directory.  Otherwise, if the file is modified because
 *        of the O_TRUNC flag, its ctime and mtime fields are  set  to  the
 *        current time.
 * 
 * 
 * ERRORS
 *        EEXIST pathname  already exists and O_CREAT and O_EXCL were used.
 * 
 *        EISDIR pathname refers to a directory and  the  access  requested
 *               involved writing (that is, O_WRONLY or O_RDWR is set).
 * 
 *        EACCES The requested access to the file is not allowed, or one of
 *               the directories in pathname did not allow search (execute)
 *               permission, or the file did not exist yet and write access
 *               to the parent directory is not allowed.
 * 
 *        ENAMETOOLONG
 *               pathname was too long.
 * 
 *        ENOENT O_CREAT is not set and the named file does not exist.  Or,
 *               a  directory  component in pathname does not exist or is a
 *               dangling symbolic link.
 * 
 *        ENOTDIR
 *               A component used as a directory in  pathname  is  not,  in
 *               fact,  a directory, or O_DIRECTORY was specified and path?
 *               name was not a directory.
 * 
 *        ENXIO  O_NONBLOCK | O_WRONLY is set, the named file is a FIFO and
 *               no process has the file open for reading.  Or, the file is
 *               a device special file and no corresponding device  exists.
 * 
 *        ENODEV pathname  refers  to  a  device special file and no corre?
 *               sponding device exists.  (This is a Linux kernel bug -  in
 *               this situation ENXIO must be returned.)
 * 
 *        EROFS  pathname  refers  to  a file on a read-only filesystem and
 *               write access was requested.
 * 
 *        ETXTBSY
 *               pathname refers to an executable image which is  currently
 *               being executed and write access was requested.
 * 
 *        EFAULT pathname points outside your accessible address space.
 * 
 *        ELOOP  Too  many  symbolic  links  were  encountered in resolving
 *               pathname, or O_NOFOLLOW was specified but pathname  was  a
 *               symbolic link.
 * 
 *        ENOSPC pathname was to be created but the device containing path?
 *               name has no room for the new file.
 * 
 *        ENOMEM Insufficient kernel memory was available.
 * 
 *        EMFILE The process already has the maximum number of files  open.
 * 
 *        ENFILE The  limit on the total number of files open on the system
 *               has been reached.
 * 
 * NOTE
 *        Under Linux, the O_NONBLOCK flag indicates that one wants to open
 *        but  does  not  necessarily  have the intention to read or write.
 *        This is typically used to open devices in order  to  get  a  file
 *        descriptor for use with ioctl(2).
 * 
 * CONFORMING TO
 *        SVr4,  SVID, POSIX, X/OPEN, BSD 4.3.  The O_NOFOLLOW and O_DIREC?
 *        TORY flags are  Linux-specific.   One  may  have  to  define  the
 *        _GNU_SOURCE macro to get their definitions.
 * 
 *        The (undefined) effect of O_RDONLY | O_TRUNC various among imple?
 *        mentations. On many systems the file is actually truncated.
 * 
 *        The O_DIRECT flag was introduced in SGI IRIX, where it has align?
 *        ment restrictions similar to those of Linux 2.4.  IRIX has also a
 *        fcntl(2)  call  to  query  appropriate  alignments,  and   sizes.
 *        FreeBSD 4.x introduced a flag of same name, but without alignment
 *        restrictions.  Support was added under Linux  in  kernel  version
 *        2.4.10.  Older Linux kernels simply ignore this flag.
 * 
 * BUGS
 *        "The  thing  that  has always disturbed me about O_DIRECT is that
 *        the whole interface is just stupid, and was probably designed  by
 *        a  deranged  monkey on some serious mind-controlling substances."
 *        -- Linus
 * 
 * RESTRICTIONS
 *        There are many  infelicities  in  the  protocol  underlying  NFS,
 *        affecting amongst others O_SYNC and O_NDELAY.
 * 
 *        POSIX  provides for three different variants of synchronised I/O,
 *        corresponding to the flags O_SYNC,  O_DSYNC  and  O_RSYNC.   Cur?
 *        rently (2.1.130) these are all synonymous under Linux.
 * 
 * SEE ALSO
 *        read(2),   write(2),   fcntl(2),   close(2),  link(2),  mknod(2),
 *        mount(2),  stat(2),  umask(2),  unlink(2),  socket(2),  fopen(3),
 *        fifo(4)
 * 
 * 
 * 
 * Linux                          1999-06-03                        OPEN(2)
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <eprintf.h>
#include <puny.h>

int Flags = O_CREAT | O_TRUNC | O_WRONLY;
int Mode = 0660;

void myopt (int c)
{
	switch (c) {
	case 'g':
		Flags = strtoll(optarg, NULL, 0);
		break;
	case 'm':
		Mode = strtoll(optarg, NULL, 0);
		break;
	default:
		usage();
		break;
	}
}

void usage (void)
{
	pr_usage("-f<file> -g<flags> -m<mode>");
}

int main (int argc, char *argv[])
{
	int	fd;

	punyopt(argc, argv, myopt, "g:m:");
	fd = open(Option.file, Flags, Mode);
	if (fd == -1) fatal("open %s:", Option.file);
	close(fd);

	return 0;
}
