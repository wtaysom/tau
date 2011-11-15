/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _SYSCALL_H_
#define _SYSCALL_H_ 1

#ifdef __x86_64__

enum {
	sys_read,
	sys_write,
	sys_open,
	sys_close,
	sys_newstat,
	sys_newfstat,
	sys_newlstat,
	sys_poll,
	/* 8 */
	sys_lseek,
	sys_mmap,
	sys_mprotect,
	sys_munmap,
	sys_brk,
	sys_rt_sigaction,
	sys_rt_sigprocmask,
	stub_rt_sigreturn,
	/* 16 */
	sys_ioctl,
	sys_pread64,
	sys_pwrite64,
	sys_readv,
	sys_writev,
	sys_access,
	sys_pipe,
	sys_select,
	/* 24 */
	sys_sched_yield,
	sys_mremap,
	sys_msync,
	sys_mincore,
	sys_madvise,
	sys_shmget,
	sys_shmat,
	sys_shmctl,
	/* 32 */
	sys_dup,
	sys_dup2,
	sys_pause,
	sys_nanosleep,
	sys_getitimer,
	sys_alarm,
	sys_setitimer,
	sys_getpid,
	/* 40 */
	sys_sendfile64,
	sys_socket,
	sys_connect,
	sys_accept,
	sys_sendto,
	sys_recvfrom,
	sys_sendmsg,
	sys_recvmsg,
	/* 48 */
	sys_shutdown,
	sys_bind,
	sys_listen,
	sys_getsockname,
	sys_getpeername,
	sys_socketpair,
	sys_setsockopt,
	sys_getsockopt,
	/* 56 */
	stub_clone,
	stub_fork,
	stub_vfork,
	stub_execve,
	sys_exit,
	sys_wait4,
	sys_kill,
	sys_newuname,
	/* 64 */
	sys_semget,
	sys_semop,
	sys_semctl,
	sys_shmdt,
	sys_msgget,
	sys_msgsnd,
	sys_msgrcv,
	sys_msgctl,
	/* 72 */
	sys_fcntl,
	sys_flock,
	sys_fsync,
	sys_fdatasync,
	sys_truncate,
	sys_ftruncate,
	sys_getdents,
	sys_getcwd,
	/* 80 */
	sys_chdir,
	sys_fchdir,
	sys_rename,
	sys_mkdir,
	sys_rmdir,
	sys_creat,
	sys_link,
	sys_unlink,
	/* 88 */
	sys_symlink,
	sys_readlink,
	sys_chmod,
	sys_fchmod,
	sys_chown,
	sys_fchown,
	sys_lchown,
	sys_umask,
	/* 96 */
	sys_gettimeofday,
	sys_getrlimit,
	sys_getrusage,
	sys_sysinfo,
	sys_times,
	sys_ptrace,
	sys_getuid,
	sys_syslog,
/* at the very end the stuff that never runs during the benchmarks */
	sys_getgid,
	sys_setuid,
	sys_setgid,
	sys_geteuid,
	sys_getegid,
	sys_setpgid,
	sys_getppid,
	sys_getpgrp,
	/* 112 */
	sys_setsid,
	sys_setreuid,
	sys_setregid,
	sys_getgroups,
	sys_setgroups,
	sys_setresuid,
	sys_getresuid,
	sys_setresgid,
	/* 120 */
	sys_getresgid,
	sys_getpgid,
	sys_setfsuid,
	sys_setfsgid,
	sys_getsid,
	sys_capget,
	sys_capset,
	/* 127 */
	sys_rt_sigpending,
	sys_rt_sigtimedwait,
	sys_rt_sigqueueinfo,
	sys_rt_sigsuspend,
	stub_sigaltstack,
	sys_utime,
	sys_mknod,
/* Only needed for a.out */
	sys_userlib,
	sys_personality,
	/* 136 */
	sys_ustat,
	sys_statfs,
	sys_fstatfs,
	sys_sysfs,
	/* 140 */
	sys_getpriority,
	sys_setpriority,
	sys_sched_setparam,
	sys_sched_getparam,
	sys_sched_setscheduler,
	sys_sched_getscheduler,
	sys_sched_get_priority_max,
	sys_sched_get_priority_min,
	sys_sched_rr_get_interval,
	/* 149 */
	sys_mlock,
	sys_munlock,
	sys_mlockall,
	sys_munlockall,
	/* 153 */
	sys_vhangup,
	/* 154 */
	sys_modify_ldt,
	/* 155 */
	sys_pivot_root,
	/* 156 */
	sys_sysctl,
	/* 157 */
	sys_prctl,
	sys_arch_prctl,
	/* 159 */
	sys_adjtimex,
	/* 160 */
	sys_setrlimit,
	/* 161 */
	sys_chroot,
	/* 162 */
	sys_sync,
	/* 163 */
	sys_acct,
	/* 164 */
	sys_settimeofday,
	/* 165 */
	sys_mount,
	sys_umount,
	/* 167 */
	sys_swapon,
	sys_swapoff,
	/* 169 */
	sys_reboot,
	/* 170 */
	sys_sethostname,
	sys_setdomainname,
	/* 172 */
	stub_iopl,
	sys_ioperm,
	/* 174 */
	sys_create_module,
	sys_init_module,
	sys_delete_module,
	sys_get_kernel_syms,
	sys_query_module,
	/* 179 */
	sys_quotactl,
	/* 180 */
	sys_nfsservctl,
/* reserved for LiS/STREAMS */
	sys_getpmsg,
	sys_putpmsg,
/* reserved for AFS */
	sys_afs_suscall,
/* reserved for tux */
	sys_tux_call,
	/* 185 */
	sys_security,
	/* 186 */
	sys_gettid,
	/* 187 */
	sys_readahead,
	sys_setxattr,
	sys_lsetxattr,
	sys_fsetxattr,
	sys_getxattr,
	sys_lgetxattr,
	sys_fgetxattr,
	sys_listxattr,
	sys_llistxattr,
	sys_flistxattr,
	sys_removexattr,
	sys_lremovexattr,
	sys_fremovexattr,
	sys_tkill,
	sys_time,
	sys_futex,
	sys_sched_setaffinity,
	sys_sched_getaffinity,
	sys_set_thread_area,	/* use arch_prctl */
	sys_io_setup,
	sys_io_destroy,
	sys_io_getevents,
	sys_io_submit,
	sys_io_cancel,
	sys_get_thread_area,	/* use arch_prctl */
	sys_lookup_dcookie,
	sys_epoll_create,
	sys_epoll_ctl_old,
	sys_epoll_wait_old,
	sys_remap_file_pages,
	sys_getdents64,
	sys_set_tid_address,
	sys_restart_syscall,
	sys_semtimedop,
	sys_fadvise64,
	sys_timer_create,
	sys_timer_settime,
	sys_timer_gettime,
	sys_timer_getoverrun,
	sys_timer_delete,
	sys_clock_settime,
	sys_clock_gettime,
	sys_clock_getres,
	sys_clock_nanosleep,
	sys_exit_group,
	sys_epoll_wait,
	sys_epoll_ctl,
	sys_tgkill,
	sys_utimes,
	sys_vserver,
	sys_mbind,
	sys_set_mempolicy,
	sys_get_mempolicy,
	sys_mq_open,
	sys_mq_unlink,
	sys_mq_timedsend,
	sys_mq_timedreceive,
	sys_mq_notify,
	sys_mq_getsetattr,
	sys_kexec_load,
	sys_waitid,
	sys_add_key,
	sys_request_key,
	sys_keyctl,
	sys_ioprio_set,
	sys_ioprio_get,
	sys_inotify_init,
	sys_inotify_add_watch,
	sys_inotify_rm_watch,
	sys_migrate_pages,
	sys_openat,
	sys_mkdirat,
	sys_mknodat,
	sys_fchownat,
	sys_futimesat,
	sys_newfstatat,
	sys_unlinkat,
	sys_renameat,
	sys_linkat,
	sys_symlinkat,
	sys_readlinkat,
	sys_fchmodat,
	sys_faccessat,
	sys_pselect6,
	sys_ppoll,
	sys_unshare,
	sys_set_robust_list,
	sys_get_robust_list,
	sys_splice,
	sys_tee,
	sys_sync_file_range,
	sys_vmsplice,
	sys_move_pages,
	sys_utimensat,
	sys_epoll_pwait,
	sys_signalfd,
	sys_timerfd_create,
	sys_eventfd,
	sys_fallocate,
	sys_timerfd_settime,
	sys_timerfd_gettime,
	sys_accept4,
	sys_signalfd4,
	sys_eventfd2,
	sys_epoll_create1,
	sys_dup3,
	sys_pipe2,
	sys_inotify_init1,
	sys_preadv,
	sys_pwritev,
	sys_rt_tgsigqueueinfo,
	sys_perf_event_open,
	sys_recvmmsg,
	sys_fanotify_init,
	sys_fanotify_mark,
	sys_prlimit64,
	sys_name_to_handle_at,
	sys_open_by_handle_at,
	sys_clock_adjtime,
	/* 306 */
	sys_syncfs,
	sys_sendmmsg,
	sys_setns,
	NUM_SYS_CALLS
};

#else

enum {
	sys_restart_syscall,	/* 0 - old setup,() system call, used for restarting */
	sys_exit,
	ptregs_fork,
	sys_read,
	sys_write,
	sys_open,		/* 5 */
	sys_close,
	sys_waitpid,
	sys_creat,
	sys_link,
	sys_unlink,		/* 10 */
	ptregs_execve,
	sys_chdir,
	sys_time,
	sys_mknod,
	sys_chmod,		/* 15 */
	sys_lchown16,
	sys_ni_syscall_17,	/* old break syscall holder */
	sys_stat,
	sys_lseek,
	sys_getpid,		/* 20 */
	sys_mount,
	sys_oldumount,
	sys_setuid16,
	sys_getuid16,
	sys_stime,		/* 25 */
	sys_ptrace,
	sys_alarm,
	sys_fstat,
	sys_pause,
	sys_utime,		/* 30 */
	sys_ni_syscall_31,	/* old stty syscall holder */
	sys_ni_syscall_32,	/* old gtty syscall holder */
	sys_access,
	sys_nice,
	sys_ni_syscall_35,		/* 35 - old ftime syscall holder */
	sys_sync,
	sys_kill,
	sys_rename,
	sys_mkdir,
	sys_rmdir,		/* 40 */
	sys_dup,
	sys_pipe,
	sys_times,
	sys_ni_syscall_44,	/* old prof syscall holder */
	sys_brk,		/* 45 */
	sys_setgid16,
	sys_getgid16,
	sys_signal,
	sys_geteuid16,
	sys_getegid16,		/* 50 */
	sys_acct,
	sys_umount,		/* recycled never used phys() */
	sys_ni_syscall_53,	/* old lock syscall holder */
	sys_ioctl,
	sys_fcntl,		/* 55 */
	sys_ni_syscall_56,	/* old mpx syscall holder */
	sys_setpgid,
	sys_ni_syscall_58,	/* old ulimit syscall holder */
	sys_olduname,
	sys_umask,		/* 60 */
	sys_chroot,
	sys_ustat,
	sys_dup2,
	sys_getppid,
	sys_getpgrp,		/* 65 */
	sys_setsid,
	sys_sigaction,
	sys_sgetmask,
	sys_ssetmask,
	sys_setreuid16,		/* 70 */
	sys_setregid16,
	sys_sigsuspend,
	sys_sigpending,
	sys_sethostname,
	sys_setrlimit,		/* 75 */
	sys_old_getrlimit,
	sys_getrusage,
	sys_gettimeofday,
	sys_settimeofday,
	sys_getgroups16,	/* 80 */
	sys_setgroups16,
	old_select,
	sys_symlink,
	sys_lstat,
	sys_readlink,		/* 85 */
	sys_uselib,
	sys_swapon,
	sys_reboot,
	sys_old_readdir,
	old_mmap,		/* 90 */
	sys_munmap,
	sys_truncate,
	sys_ftruncate,
	sys_fchmod,
	sys_fchown16,		/* 95 */
	sys_getpriority,
	sys_setpriority,
	sys_ni_syscall_98,	/* old profil syscall holder */
	sys_statfs,
	sys_fstatfs,		/* 100 */
	sys_ioperm,
	sys_socketcall,
	sys_syslog,
	sys_setitimer,
	sys_getitimer,		/* 105 */
	sys_newstat,
	sys_newlstat,
	sys_newfstat,
	sys_uname,
	ptregs_iopl,		/* 110 */
	sys_vhangup,
	sys_ni_syscall_113,	/* old idle, system call */
	ptregs_vm86old,
	sys_wait4,
	sys_swapoff,		/* 115 */
	sys_sysinfo,
	sys_ipc,
	sys_fsync,
	ptregs_sigreturn,
	ptregs_clone,		/* 120 */
	sys_setdomainname,
	sys_newuname,
	sys_modify_ldt,
	sys_adjtimex,
	sys_mprotect,		/* 125 */
	sys_sigprocmask,
	sys_ni_syscall_127,	/* old create_module, */
	sys_init_module,
	sys_delete_module,
	sys_ni_syscall_130,	/* 130:	old get_kernel_syms, */
	sys_quotactl,
	sys_getpgid,
	sys_fchdir,
	sys_bdflush,
	sys_sysfs,		/* 135 */
	sys_personality,
	sys_ni_syscall_137,	/* reserved for afs_syscall */
	sys_setfsuid16,
	sys_setfsgid16,
	sys_llseek,		/* 140 */
	sys_getdents,
	sys_select,
	sys_flock,
	sys_msync,
	sys_readv,		/* 145 */
	sys_writev,
	sys_getsid,
	sys_fdatasync,
	sys_sysctl,
	sys_mlock,		/* 150 */
	sys_munlock,
	sys_mlockall,
	sys_munlockall,
	sys_sched_setparam,
	sys_sched_getparam,  	 /* 155 */
	sys_sched_setscheduler,
	sys_sched_getscheduler,
	sys_sched_yield,
	sys_sched_get_priority_max,
	sys_sched_get_priority_min,  /* 160 */
	sys_sched_rr_get_interval,
	sys_nanosleep,
	sys_mremap,
	sys_setresuid16,
	sys_getresuid16,	/* 165 */
	ptregs_vm86,
	sys_ni_syscall_167,	/* Old sys_query_module */
	sys_poll,
	sys_nfsservctl,
	sys_setresgid16,	/* 170 */
	sys_getresgid16,
	sys_prctl,
	ptregs_rt_sigreturn,
	sys_rt_sigaction,
	sys_rt_sigprocmask,	/* 175 */
	sys_rt_sigpending,
	sys_rt_sigtimedwait,
	sys_rt_sigqueueinfo,
	sys_rt_sigsuspend,
	sys_pread64,		/* 180 */
	sys_pwrite64,
	sys_chown16,
	sys_getcwd,
	sys_capget,
	sys_capset,		/* 185 */
	ptregs_sigaltstack,
	sys_sendfile,
	sys_ni_syscall_188,	/* reserved for streams1 */
	sys_ni_syscall_189,	/* reserved for streams2 */
	ptregs_vfork,		/* 190 */
	sys_getrlimit,
	sys_mmap_pgoff,
	sys_truncate64,
	sys_ftruncate64,
	sys_stat64,		/* 195 */
	sys_lstat64,
	sys_fstat64,
	sys_lchown,
	sys_getuid,
	sys_getgid,		/* 200 */
	sys_geteuid,
	sys_getegid,
	sys_setreuid,
	sys_setregid,
	sys_getgroups,		/* 205 */
	sys_setgroups,
	sys_fchown,
	sys_setresuid,
	sys_getresuid,
	sys_setresgid,		/* 210 */
	sys_getresgid,
	sys_chown,
	sys_setuid,
	sys_setgid,
	sys_setfsuid,		/* 215 */
	sys_setfsgid,
	sys_pivot_root,
	sys_mincore,
	sys_madvise,
	sys_getdents64,		/* 220 */
	sys_fcntl64,
	sys_ni_syscall_222,	/* reserved for TUX */
	sys_ni_syscall_223,
	sys_gettid,
	sys_readahead,		/* 225 */
	sys_setxattr,
	sys_lsetxattr,
	sys_fsetxattr,
	sys_getxattr,
	sys_lgetxattr,		/* 230 */
	sys_fgetxattr,
	sys_listxattr,
	sys_llistxattr,
	sys_flistxattr,
	sys_removexattr,	/* 235 */
	sys_lremovexattr,
	sys_fremovexattr,
	sys_tkill,
	sys_sendfile64,
	sys_futex,		/* 240 */
	sys_sched_setaffinity,
	sys_sched_getaffinity,
	sys_set_thread_area,
	sys_get_thread_area,
	sys_io_setup,		/* 245 */
	sys_io_destroy,
	sys_io_getevents,
	sys_io_submit,
	sys_io_cancel,
	sys_fadvise64,		/* 250 */
	sys_ni_syscall_251,
	sys_exit_group,
	sys_lookup_dcookie,
	sys_epoll_create,
	sys_epoll_ctl,		/* 255 */
	sys_epoll_wait,
 	sys_remap_file_pages,
 	sys_set_tid_address,
 	sys_timer_create,
 	sys_timer_settime,	/* 260 */
 	sys_timer_gettime,
 	sys_timer_getoverrun,
 	sys_timer_delete,
 	sys_clock_settime,
 	sys_clock_gettime,	/* 265 */
 	sys_clock_getres,
 	sys_clock_nanosleep,
	sys_statfs64,
	sys_fstatfs64,
	sys_tgkill,		/* 270 */
	sys_utimes,
 	sys_fadvise64_64,
	sys_ni_syscall_273,	/* sys_vserver */
	sys_mbind,
	sys_get_mempolicy,	/* 275 */
	sys_set_mempolicy,
	sys_mq_open,
	sys_mq_unlink,
	sys_mq_timedsend,
	sys_mq_timedreceive,	/* 280 */
	sys_mq_notify,
	sys_mq_getsetattr,
	sys_kexec_load,
	sys_waitid,
	sys_ni_syscall_285,	/* 285 */ /* available */
	sys_add_key,
	sys_request_key,
	sys_keyctl,
	sys_ioprio_set,
	sys_ioprio_get,		/* 290 */
	sys_inotify_init,
	sys_inotify_add_watch,
	sys_inotify_rm_watch,
	sys_migrate_pages,
	sys_openat,		/* 295 */
	sys_mkdirat,
	sys_mknodat,
	sys_fchownat,
	sys_futimesat,
	sys_fstatat64,		/* 300 */
	sys_unlinkat,
	sys_renameat,
	sys_linkat,
	sys_symlinkat,
	sys_readlinkat,		/* 305 */
	sys_fchmodat,
	sys_faccessat,
	sys_pselect6,
	sys_ppoll,
	sys_unshare,		/* 310 */
	sys_set_robust_list,
	sys_get_robust_list,
	sys_splice,
	sys_sync_file_range,
	sys_tee,		/* 315 */
	sys_vmsplice,
	sys_move_pages,
	sys_getcpu,
	sys_epoll_pwait,
	sys_utimensat,		/* 320 */
	sys_signalfd,
	sys_timerfd_create,
	sys_eventfd,
	sys_fallocate,
	sys_timerfd_settime,	/* 325 */
	sys_timerfd_gettime,
	sys_signalfd4,
	sys_eventfd2,
	sys_epoll_create1,
	sys_dup3,		/* 330 */
	sys_pipe2,
	sys_inotify_init1,
	sys_preadv,
	sys_pwritev,
	sys_rt_tgsigqueueinfo,	/* 335 */
	sys_perf_event_open,
	NUM_SYS_CALLS
};
#endif

extern const char *Syscall[];
extern const int Num_syscalls;

#endif
