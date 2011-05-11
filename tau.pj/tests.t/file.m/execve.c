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
 *
 * EXECVE(2)           Linux Programmer's Manual           EXECVE(2)
 *
 *
 *
 * NAME
 *     execve - execute program
 *
 * SYNOPSIS
 *     #include <unistd.h>
 *
 *     int  execve(const  char  *filename,  char  *const  argv  [], char *const
 *     envp[]);
 *
 * DESCRIPTION
 *     execve() executes the program pointed to by filename.  filename must  be
 *     either a binary executable, or a script starting with a line of the form
 *     "#! interpreter [arg]".  In the latter case, the interpreter must  be  a
 *     valid  pathname  for  an  executable which is not itself a script, which
 *     will be invoked as interpreter [arg] filename.
 *
 *     argv is an array of argument strings passed to the new program.  envp is
 *     an  array  of  strings,  conventionally of the form key=value, which are
 *     passed as environment to the new program.  Both, argv and envp  must  be
 *     terminated  by  a null pointer.  The argument vector and environment can
 *     be accessed by the called program's main function, when it is defined as
 *     int main(int argc, char *argv[], char *envp[]).
 *
 *     execve()  does not return on success, and the text, data, bss, and stack
 *     of the calling process are overwritten by that of  the  program  loaded.
 *     The  program  invoked  inherits  the calling process's PID, and any open
 *     file descriptors that are not set to close on exec.  Signals pending  on
 *     the  calling  process  are cleared.  Any signals set to be caught by the
 *     calling process are reset to their default behaviour.  The SIGCHLD  sig-
 *     nal (when set to SIG_IGN) may or may not be reset to SIG_DFL.
 *
 *     If the current program is being ptraced, a SIGTRAP is sent to it after a
 *     successful execve().
 *
 *     If the set-uid bit is set on the program file pointed to by filename the
 *     effective user ID of the calling process is changed to that of the owner
 *     of the program file.  Similarly, when the set-gid  bit  of  the  program
 *     file  is set the effective group ID of the calling process is set to the
 *     group of the program file.
 *
 *     If the executable is an a.out dynamically-linked binary executable  con-
 *     taining  shared-library  stubs,  the  Linux  dynamic  linker ld.so(8) is
 *     called at the start of execution to bring needed shared  libraries  into
 *     core and link the executable with them.
 *
 *     If  the  executable  is  a dynamically-linked ELF executable, the inter-
 *     preter named in the PT_INTERP segment is used to load the needed  shared
 *     libraries.   This  interpreter is typically /lib/ld-linux.so.1 for bina-
 *     ries linked with the Linux libc version  5,  or  /lib/ld-linux.so.2  for
 *     binaries linked with the GNU libc version 2.
 *
 * RETURN VALUE
 *     On success, execve() does not return, on error -1 is returned, and errno
 *     is set appropriately.
 *
 * ERRORS
 *     EACCES The file or a script interpreter is not a regular file.
 *
 *     EACCES Execute permission is denied for the file  or  a  script  or  ELF
 *            interpreter.
 *
 *     EACCES The file system is mounted noexec.
 *
 *     EPERM  The file system is mounted nosuid, the user is not the superuser,
 *            and the file has an SUID or SGID bit set.
 *
 *     EPERM  The process is being traced, the user is not  the  superuser  and
 *            the file has an SUID or SGID bit set.
 *
 *     E2BIG  The argument list is too big.
 *
 *     ENOEXEC
 *            An  executable  is  not  in a recognised format, is for the wrong
 *            architecture, or has some other format error that means it cannot
 *            be executed.
 *
 *     EFAULT filename points outside your accessible address space.
 *
 *     ENAMETOOLONG
 *            filename is too long.
 *
 *     ENOENT The  file filename or a script or ELF interpreter does not exist,
 *            or a shared library needed for  file  or  interpreter  cannot  be
 *            found.
 *
 *     ENOMEM Insufficient kernel memory was available.
 *
 *     ENOTDIR
 *            A  component  of  the  path prefix of filename or a script or ELF
 *            interpreter is not a directory.
 *
 *     EACCES Search permission is denied on a component of the path prefix  of
 *            filename or the name of a script interpreter.
 *
 *     ELOOP  Too many symbolic links were encountered in resolving filename or
 *            the name of a script or ELF interpreter.
 *
 *     ETXTBSY
 *            Executable was open for writing by one or more processes.
 *
 *     EIO    An I/O error occurred.
 *
 *     ENFILE The limit on the total number of files open  on  the  system  has
 *            been reached.
 *
 *     EMFILE The process has the maximum number of files open.
 *
 *     EINVAL An  ELF  executable  had  more  than one PT_INTERP segment (i.e.,
 *            tried to name more than one interpreter).
 *
 *     EISDIR An ELF interpreter was a directory.
 *
 *     ELIBBAD
 *            An ELF interpreter was not in a recognised format.
 *
 * CONFORMING TO
 *     SVr4, SVID, X/OPEN, BSD 4.3.  POSIX does not document the  #!   behavior
 *     but is otherwise compatible.  SVr4 documents additional error conditions
 *     EAGAIN, EINTR, ELIBACC, ENOLINK,  EMULTIHOP;  POSIX  does  not  document
 *     ETXTBSY,  EPERM,  EFAULT,  ELOOP, EIO, ENFILE, EMFILE, EINVAL, EISDIR or
 *     ELIBBAD error conditions.
 *
 * NOTES
 *     SUID and SGID processes can not be ptrace()d.
 *
 *     Linux ignores the SUID and SGID bits on scripts.
 *
 *     The result of mounting a filesystem nosuid  vary  between  Linux  kernel
 *     versions:  some will refuse execution of SUID/SGID executables when this
 *     would give the user powers she did not have already (and return  EPERM),
 *     some will just ignore the SUID/SGID bits and exec successfully.
 *
 *     A maximum line length of 127 characters is allowed for the first line in
 *     a #! executable shell script.
 *
 * SEE ALSO
 *     chmod(2), fork(2), execl(3), environ(5), ld.so(8)
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <puny.h>

int main (int argc, char *argv[], char *envp[])
{
	int	rc;

	punyopt(argc, argv, NULL, NULL);
	rc = execve(Option.file, argv, envp);
	if (rc != 0)
	{
		perror("execve");
		printf("execve=%d\n", rc);
		exit(1);
	}
	return 0;
}

