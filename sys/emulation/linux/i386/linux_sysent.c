/*
 * System call switch table.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * $DragonFly: src/sys/emulation/linux/i386/linux_sysent.c,v 1.18 2006/06/13 08:12:03 dillon Exp $
 * created from DragonFly: src/sys/emulation/linux/i386/syscalls.master,v 1.9 2006/06/05 07:26:10 dillon Exp 
 */

#include "opt_compat.h"
#include <sys/param.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <emulation/linux/linux_sysproto.h>
#include "linux.h"
#include "linux_proto.h"
#include <emulation/43bsd/stat.h>

#define AS(name) ((sizeof(struct name) - sizeof(struct sysmsg)) / sizeof(register_t))

/* The casts are bogus but will do for now. */
struct sysent linux_sysent[] = {
#define	nosys	linux_nosys
	{ 0, (sy_call_t *)sys_nosys },			/* 0 = setup */
	{ AS(exit_args), (sy_call_t *)sys_exit },	/* 1 = exit */
	{ 0, (sy_call_t *)sys_linux_fork },		/* 2 = linux_fork */
	{ AS(read_args), (sy_call_t *)sys_read },	/* 3 = read */
	{ AS(write_args), (sy_call_t *)sys_write },	/* 4 = write */
	{ AS(linux_open_args), (sy_call_t *)sys_linux_open },	/* 5 = linux_open */
	{ AS(close_args), (sy_call_t *)sys_close },	/* 6 = close */
	{ AS(linux_waitpid_args), (sy_call_t *)sys_linux_waitpid },	/* 7 = linux_waitpid */
	{ AS(linux_creat_args), (sy_call_t *)sys_linux_creat },	/* 8 = linux_creat */
	{ AS(linux_link_args), (sy_call_t *)sys_linux_link },	/* 9 = linux_link */
	{ AS(linux_unlink_args), (sy_call_t *)sys_linux_unlink },	/* 10 = linux_unlink */
	{ AS(linux_execve_args), (sy_call_t *)sys_linux_execve },	/* 11 = linux_execve */
	{ AS(linux_chdir_args), (sy_call_t *)sys_linux_chdir },	/* 12 = linux_chdir */
	{ AS(linux_time_args), (sy_call_t *)sys_linux_time },	/* 13 = linux_time */
	{ AS(linux_mknod_args), (sy_call_t *)sys_linux_mknod },	/* 14 = linux_mknod */
	{ AS(linux_chmod_args), (sy_call_t *)sys_linux_chmod },	/* 15 = linux_chmod */
	{ AS(linux_lchown16_args), (sy_call_t *)sys_linux_lchown16 },	/* 16 = linux_lchown16 */
	{ 0, (sy_call_t *)sys_nosys },			/* 17 = break */
	{ AS(linux_stat_args), (sy_call_t *)sys_linux_stat },	/* 18 = linux_stat */
	{ AS(linux_lseek_args), (sy_call_t *)sys_linux_lseek },	/* 19 = linux_lseek */
	{ 0, (sy_call_t *)sys_linux_getpid },		/* 20 = linux_getpid */
	{ AS(linux_mount_args), (sy_call_t *)sys_linux_mount },	/* 21 = linux_mount */
	{ AS(linux_oldumount_args), (sy_call_t *)sys_linux_oldumount },	/* 22 = linux_oldumount */
	{ AS(linux_setuid16_args), (sy_call_t *)sys_linux_setuid16 },	/* 23 = linux_setuid16 */
	{ 0, (sy_call_t *)sys_linux_getuid16 },		/* 24 = linux_getuid16 */
	{ 0, (sy_call_t *)sys_linux_stime },		/* 25 = linux_stime */
	{ AS(linux_ptrace_args), (sy_call_t *)sys_linux_ptrace },	/* 26 = linux_ptrace */
	{ AS(linux_alarm_args), (sy_call_t *)sys_linux_alarm },	/* 27 = linux_alarm */
	{ AS(linux_fstat_args), (sy_call_t *)sys_linux_fstat },	/* 28 = linux_fstat */
	{ 0, (sy_call_t *)sys_linux_pause },		/* 29 = linux_pause */
	{ AS(linux_utime_args), (sy_call_t *)sys_linux_utime },	/* 30 = linux_utime */
	{ 0, (sy_call_t *)sys_nosys },			/* 31 = stty */
	{ 0, (sy_call_t *)sys_nosys },			/* 32 = gtty */
	{ AS(linux_access_args), (sy_call_t *)sys_linux_access },	/* 33 = linux_access */
	{ AS(linux_nice_args), (sy_call_t *)sys_linux_nice },	/* 34 = linux_nice */
	{ 0, (sy_call_t *)sys_nosys },			/* 35 = ftime */
	{ 0, (sy_call_t *)sys_sync },			/* 36 = sync */
	{ AS(linux_kill_args), (sy_call_t *)sys_linux_kill },	/* 37 = linux_kill */
	{ AS(linux_rename_args), (sy_call_t *)sys_linux_rename },	/* 38 = linux_rename */
	{ AS(linux_mkdir_args), (sy_call_t *)sys_linux_mkdir },	/* 39 = linux_mkdir */
	{ AS(linux_rmdir_args), (sy_call_t *)sys_linux_rmdir },	/* 40 = linux_rmdir */
	{ AS(dup_args), (sy_call_t *)sys_dup },		/* 41 = dup */
	{ AS(linux_pipe_args), (sy_call_t *)sys_linux_pipe },	/* 42 = linux_pipe */
	{ AS(linux_times_args), (sy_call_t *)sys_linux_times },	/* 43 = linux_times */
	{ 0, (sy_call_t *)sys_nosys },			/* 44 = prof */
	{ AS(linux_brk_args), (sy_call_t *)sys_linux_brk },	/* 45 = linux_brk */
	{ AS(linux_setgid16_args), (sy_call_t *)sys_linux_setgid16 },	/* 46 = linux_setgid16 */
	{ 0, (sy_call_t *)sys_linux_getgid16 },		/* 47 = linux_getgid16 */
	{ AS(linux_signal_args), (sy_call_t *)sys_linux_signal },	/* 48 = linux_signal */
	{ 0, (sy_call_t *)sys_linux_geteuid16 },	/* 49 = linux_geteuid16 */
	{ 0, (sy_call_t *)sys_linux_getegid16 },	/* 50 = linux_getegid16 */
	{ AS(acct_args), (sy_call_t *)sys_acct },	/* 51 = acct */
	{ AS(linux_umount_args), (sy_call_t *)sys_linux_umount },	/* 52 = linux_umount */
	{ 0, (sy_call_t *)sys_nosys },			/* 53 = lock */
	{ AS(linux_ioctl_args), (sy_call_t *)sys_linux_ioctl },	/* 54 = linux_ioctl */
	{ AS(linux_fcntl_args), (sy_call_t *)sys_linux_fcntl },	/* 55 = linux_fcntl */
	{ 0, (sy_call_t *)sys_nosys },			/* 56 = mpx */
	{ AS(setpgid_args), (sy_call_t *)sys_setpgid },	/* 57 = setpgid */
	{ 0, (sy_call_t *)sys_nosys },			/* 58 = ulimit */
	{ 0, (sy_call_t *)sys_linux_olduname },		/* 59 = linux_olduname */
	{ AS(umask_args), (sy_call_t *)sys_umask },	/* 60 = umask */
	{ AS(chroot_args), (sy_call_t *)sys_chroot },	/* 61 = chroot */
	{ AS(linux_ustat_args), (sy_call_t *)sys_linux_ustat },	/* 62 = linux_ustat */
	{ AS(dup2_args), (sy_call_t *)sys_dup2 },	/* 63 = dup2 */
	{ 0, (sy_call_t *)sys_getppid },		/* 64 = getppid */
	{ 0, (sy_call_t *)sys_getpgrp },		/* 65 = getpgrp */
	{ 0, (sy_call_t *)sys_setsid },			/* 66 = setsid */
	{ AS(linux_sigaction_args), (sy_call_t *)sys_linux_sigaction },	/* 67 = linux_sigaction */
	{ 0, (sy_call_t *)sys_linux_sgetmask },		/* 68 = linux_sgetmask */
	{ AS(linux_ssetmask_args), (sy_call_t *)sys_linux_ssetmask },	/* 69 = linux_ssetmask */
	{ AS(linux_setreuid16_args), (sy_call_t *)sys_linux_setreuid16 },	/* 70 = linux_setreuid16 */
	{ AS(linux_setregid16_args), (sy_call_t *)sys_linux_setregid16 },	/* 71 = linux_setregid16 */
	{ AS(linux_sigsuspend_args), (sy_call_t *)sys_linux_sigsuspend },	/* 72 = linux_sigsuspend */
	{ AS(linux_sigpending_args), (sy_call_t *)sys_linux_sigpending },	/* 73 = linux_sigpending */
	{ AS(sethostname_args), (sy_call_t *)sys_osethostname },	/* 74 = osethostname */
	{ AS(linux_setrlimit_args), (sy_call_t *)sys_linux_setrlimit },	/* 75 = linux_setrlimit */
	{ AS(linux_old_getrlimit_args), (sy_call_t *)sys_linux_old_getrlimit },	/* 76 = linux_old_getrlimit */
	{ AS(getrusage_args), (sy_call_t *)sys_getrusage },	/* 77 = getrusage */
	{ AS(gettimeofday_args), (sy_call_t *)sys_gettimeofday },	/* 78 = gettimeofday */
	{ AS(settimeofday_args), (sy_call_t *)sys_settimeofday },	/* 79 = settimeofday */
	{ AS(linux_getgroups16_args), (sy_call_t *)sys_linux_getgroups16 },	/* 80 = linux_getgroups16 */
	{ AS(linux_setgroups16_args), (sy_call_t *)sys_linux_setgroups16 },	/* 81 = linux_setgroups16 */
	{ AS(linux_old_select_args), (sy_call_t *)sys_linux_old_select },	/* 82 = linux_old_select */
	{ AS(linux_symlink_args), (sy_call_t *)sys_linux_symlink },	/* 83 = linux_symlink */
	{ AS(ostat_args), (sy_call_t *)sys_ostat },	/* 84 = ostat */
	{ AS(linux_readlink_args), (sy_call_t *)sys_linux_readlink },	/* 85 = linux_readlink */
	{ AS(linux_uselib_args), (sy_call_t *)sys_linux_uselib },	/* 86 = linux_uselib */
	{ AS(swapon_args), (sy_call_t *)sys_swapon },	/* 87 = swapon */
	{ AS(linux_reboot_args), (sy_call_t *)sys_linux_reboot },	/* 88 = linux_reboot */
	{ AS(linux_readdir_args), (sy_call_t *)sys_linux_readdir },	/* 89 = linux_readdir */
	{ AS(linux_mmap_args), (sy_call_t *)sys_linux_mmap },	/* 90 = linux_mmap */
	{ AS(munmap_args), (sy_call_t *)sys_munmap },	/* 91 = munmap */
	{ AS(linux_truncate_args), (sy_call_t *)sys_linux_truncate },	/* 92 = linux_truncate */
	{ AS(linux_ftruncate_args), (sy_call_t *)sys_linux_ftruncate },	/* 93 = linux_ftruncate */
	{ AS(fchmod_args), (sy_call_t *)sys_fchmod },	/* 94 = fchmod */
	{ AS(fchown_args), (sy_call_t *)sys_fchown },	/* 95 = fchown */
	{ AS(getpriority_args), (sy_call_t *)sys_getpriority },	/* 96 = getpriority */
	{ AS(setpriority_args), (sy_call_t *)sys_setpriority },	/* 97 = setpriority */
	{ 0, (sy_call_t *)sys_nosys },			/* 98 = profil */
	{ AS(linux_statfs_args), (sy_call_t *)sys_linux_statfs },	/* 99 = linux_statfs */
	{ AS(linux_fstatfs_args), (sy_call_t *)sys_linux_fstatfs },	/* 100 = linux_fstatfs */
	{ AS(linux_ioperm_args), (sy_call_t *)sys_linux_ioperm },	/* 101 = linux_ioperm */
	{ AS(linux_socketcall_args), (sy_call_t *)sys_linux_socketcall },	/* 102 = linux_socketcall */
	{ AS(linux_syslog_args), (sy_call_t *)sys_linux_syslog },	/* 103 = linux_syslog */
	{ AS(linux_setitimer_args), (sy_call_t *)sys_linux_setitimer },	/* 104 = linux_setitimer */
	{ AS(linux_getitimer_args), (sy_call_t *)sys_linux_getitimer },	/* 105 = linux_getitimer */
	{ AS(linux_newstat_args), (sy_call_t *)sys_linux_newstat },	/* 106 = linux_newstat */
	{ AS(linux_newlstat_args), (sy_call_t *)sys_linux_newlstat },	/* 107 = linux_newlstat */
	{ AS(linux_newfstat_args), (sy_call_t *)sys_linux_newfstat },	/* 108 = linux_newfstat */
	{ 0, (sy_call_t *)sys_linux_uname },		/* 109 = linux_uname */
	{ AS(linux_iopl_args), (sy_call_t *)sys_linux_iopl },	/* 110 = linux_iopl */
	{ 0, (sy_call_t *)sys_linux_vhangup },		/* 111 = linux_vhangup */
	{ 0, (sy_call_t *)sys_nosys },			/* 112 = idle */
	{ 0, (sy_call_t *)sys_linux_vm86old },		/* 113 = linux_vm86old */
	{ AS(linux_wait4_args), (sy_call_t *)sys_linux_wait4 },	/* 114 = linux_wait4 */
	{ 0, (sy_call_t *)sys_linux_swapoff },		/* 115 = linux_swapoff */
	{ AS(linux_sysinfo_args), (sy_call_t *)sys_linux_sysinfo },	/* 116 = linux_sysinfo */
	{ AS(linux_ipc_args), (sy_call_t *)sys_linux_ipc },	/* 117 = linux_ipc */
	{ AS(fsync_args), (sy_call_t *)sys_fsync },	/* 118 = fsync */
	{ AS(linux_sigreturn_args), (sy_call_t *)sys_linux_sigreturn },	/* 119 = linux_sigreturn */
	{ AS(linux_clone_args), (sy_call_t *)sys_linux_clone },	/* 120 = linux_clone */
	{ AS(setdomainname_args), (sy_call_t *)sys_setdomainname },	/* 121 = setdomainname */
	{ AS(linux_newuname_args), (sy_call_t *)sys_linux_newuname },	/* 122 = linux_newuname */
	{ AS(linux_modify_ldt_args), (sy_call_t *)sys_linux_modify_ldt },	/* 123 = linux_modify_ldt */
	{ 0, (sy_call_t *)sys_linux_adjtimex },		/* 124 = linux_adjtimex */
	{ AS(mprotect_args), (sy_call_t *)sys_mprotect },	/* 125 = mprotect */
	{ AS(linux_sigprocmask_args), (sy_call_t *)sys_linux_sigprocmask },	/* 126 = linux_sigprocmask */
	{ 0, (sy_call_t *)sys_linux_create_module },	/* 127 = linux_create_module */
	{ 0, (sy_call_t *)sys_linux_init_module },	/* 128 = linux_init_module */
	{ 0, (sy_call_t *)sys_linux_delete_module },	/* 129 = linux_delete_module */
	{ 0, (sy_call_t *)sys_linux_get_kernel_syms },	/* 130 = linux_get_kernel_syms */
	{ 0, (sy_call_t *)sys_linux_quotactl },		/* 131 = linux_quotactl */
	{ AS(getpgid_args), (sy_call_t *)sys_getpgid },	/* 132 = getpgid */
	{ AS(fchdir_args), (sy_call_t *)sys_fchdir },	/* 133 = fchdir */
	{ 0, (sy_call_t *)sys_linux_bdflush },		/* 134 = linux_bdflush */
	{ AS(linux_sysfs_args), (sy_call_t *)sys_linux_sysfs },	/* 135 = linux_sysfs */
	{ AS(linux_personality_args), (sy_call_t *)sys_linux_personality },	/* 136 = linux_personality */
	{ 0, (sy_call_t *)sys_nosys },			/* 137 = afs_syscall */
	{ AS(linux_setfsuid16_args), (sy_call_t *)sys_linux_setfsuid16 },	/* 138 = linux_setfsuid16 */
	{ AS(linux_setfsgid16_args), (sy_call_t *)sys_linux_setfsgid16 },	/* 139 = linux_setfsgid16 */
	{ AS(linux_llseek_args), (sy_call_t *)sys_linux_llseek },	/* 140 = linux_llseek */
	{ AS(linux_getdents_args), (sy_call_t *)sys_linux_getdents },	/* 141 = linux_getdents */
	{ AS(linux_select_args), (sy_call_t *)sys_linux_select },	/* 142 = linux_select */
	{ AS(flock_args), (sy_call_t *)sys_flock },	/* 143 = flock */
	{ AS(linux_msync_args), (sy_call_t *)sys_linux_msync },	/* 144 = linux_msync */
	{ AS(readv_args), (sy_call_t *)sys_readv },	/* 145 = readv */
	{ AS(writev_args), (sy_call_t *)sys_writev },	/* 146 = writev */
	{ AS(linux_getsid_args), (sy_call_t *)sys_linux_getsid },	/* 147 = linux_getsid */
	{ AS(linux_fdatasync_args), (sy_call_t *)sys_linux_fdatasync },	/* 148 = linux_fdatasync */
	{ AS(linux_sysctl_args), (sy_call_t *)sys_linux_sysctl },	/* 149 = linux_sysctl */
	{ AS(mlock_args), (sy_call_t *)sys_mlock },	/* 150 = mlock */
	{ AS(munlock_args), (sy_call_t *)sys_munlock },	/* 151 = munlock */
	{ AS(mlockall_args), (sy_call_t *)sys_mlockall },	/* 152 = mlockall */
	{ 0, (sy_call_t *)sys_munlockall },		/* 153 = munlockall */
	{ AS(sched_setparam_args), (sy_call_t *)sys_sched_setparam },	/* 154 = sched_setparam */
	{ AS(sched_getparam_args), (sy_call_t *)sys_sched_getparam },	/* 155 = sched_getparam */
	{ AS(linux_sched_setscheduler_args), (sy_call_t *)sys_linux_sched_setscheduler },	/* 156 = linux_sched_setscheduler */
	{ AS(linux_sched_getscheduler_args), (sy_call_t *)sys_linux_sched_getscheduler },	/* 157 = linux_sched_getscheduler */
	{ 0, (sy_call_t *)sys_sched_yield },		/* 158 = sched_yield */
	{ AS(linux_sched_get_priority_max_args), (sy_call_t *)sys_linux_sched_get_priority_max },	/* 159 = linux_sched_get_priority_max */
	{ AS(linux_sched_get_priority_min_args), (sy_call_t *)sys_linux_sched_get_priority_min },	/* 160 = linux_sched_get_priority_min */
	{ AS(sched_rr_get_interval_args), (sy_call_t *)sys_sched_rr_get_interval },	/* 161 = sched_rr_get_interval */
	{ AS(nanosleep_args), (sy_call_t *)sys_nanosleep },	/* 162 = nanosleep */
	{ AS(linux_mremap_args), (sy_call_t *)sys_linux_mremap },	/* 163 = linux_mremap */
	{ AS(linux_setresuid16_args), (sy_call_t *)sys_linux_setresuid16 },	/* 164 = linux_setresuid16 */
	{ AS(linux_getresuid16_args), (sy_call_t *)sys_linux_getresuid16 },	/* 165 = linux_getresuid16 */
	{ 0, (sy_call_t *)sys_linux_vm86 },		/* 166 = linux_vm86 */
	{ 0, (sy_call_t *)sys_linux_query_module },	/* 167 = linux_query_module */
	{ AS(poll_args), (sy_call_t *)sys_poll },	/* 168 = poll */
	{ 0, (sy_call_t *)sys_linux_nfsservctl },	/* 169 = linux_nfsservctl */
	{ AS(linux_setresgid16_args), (sy_call_t *)sys_linux_setresgid16 },	/* 170 = linux_setresgid16 */
	{ AS(linux_getresgid16_args), (sy_call_t *)sys_linux_getresgid16 },	/* 171 = linux_getresgid16 */
	{ 0, (sy_call_t *)sys_linux_prctl },		/* 172 = linux_prctl */
	{ AS(linux_rt_sigreturn_args), (sy_call_t *)sys_linux_rt_sigreturn },	/* 173 = linux_rt_sigreturn */
	{ AS(linux_rt_sigaction_args), (sy_call_t *)sys_linux_rt_sigaction },	/* 174 = linux_rt_sigaction */
	{ AS(linux_rt_sigprocmask_args), (sy_call_t *)sys_linux_rt_sigprocmask },	/* 175 = linux_rt_sigprocmask */
	{ 0, (sy_call_t *)sys_linux_rt_sigpending },	/* 176 = linux_rt_sigpending */
	{ 0, (sy_call_t *)sys_linux_rt_sigtimedwait },	/* 177 = linux_rt_sigtimedwait */
	{ 0, (sy_call_t *)sys_linux_rt_sigqueueinfo },	/* 178 = linux_rt_sigqueueinfo */
	{ AS(linux_rt_sigsuspend_args), (sy_call_t *)sys_linux_rt_sigsuspend },	/* 179 = linux_rt_sigsuspend */
	{ AS(linux_pread_args), (sy_call_t *)sys_linux_pread },	/* 180 = linux_pread */
	{ AS(linux_pwrite_args), (sy_call_t *)sys_linux_pwrite },	/* 181 = linux_pwrite */
	{ AS(linux_chown16_args), (sy_call_t *)sys_linux_chown16 },	/* 182 = linux_chown16 */
	{ AS(linux_getcwd_args), (sy_call_t *)sys_linux_getcwd },	/* 183 = linux_getcwd */
	{ 0, (sy_call_t *)sys_linux_capget },		/* 184 = linux_capget */
	{ 0, (sy_call_t *)sys_linux_capset },		/* 185 = linux_capset */
	{ AS(linux_sigaltstack_args), (sy_call_t *)sys_linux_sigaltstack },	/* 186 = linux_sigaltstack */
	{ 0, (sy_call_t *)sys_linux_sendfile },		/* 187 = linux_sendfile */
	{ 0, (sy_call_t *)sys_nosys },			/* 188 = getpmsg */
	{ 0, (sy_call_t *)sys_nosys },			/* 189 = putpmsg */
	{ 0, (sy_call_t *)sys_linux_vfork },		/* 190 = linux_vfork */
	{ AS(linux_getrlimit_args), (sy_call_t *)sys_linux_getrlimit },	/* 191 = linux_getrlimit */
	{ AS(linux_mmap2_args), (sy_call_t *)sys_linux_mmap2 },	/* 192 = linux_mmap2 */
	{ AS(linux_truncate64_args), (sy_call_t *)sys_linux_truncate64 },	/* 193 = linux_truncate64 */
	{ AS(linux_ftruncate64_args), (sy_call_t *)sys_linux_ftruncate64 },	/* 194 = linux_ftruncate64 */
	{ AS(linux_stat64_args), (sy_call_t *)sys_linux_stat64 },	/* 195 = linux_stat64 */
	{ AS(linux_lstat64_args), (sy_call_t *)sys_linux_lstat64 },	/* 196 = linux_lstat64 */
	{ AS(linux_fstat64_args), (sy_call_t *)sys_linux_fstat64 },	/* 197 = linux_fstat64 */
	{ AS(linux_lchown_args), (sy_call_t *)sys_linux_lchown },	/* 198 = linux_lchown */
	{ 0, (sy_call_t *)sys_linux_getuid },		/* 199 = linux_getuid */
	{ 0, (sy_call_t *)sys_linux_getgid },		/* 200 = linux_getgid */
	{ 0, (sy_call_t *)sys_geteuid },		/* 201 = geteuid */
	{ 0, (sy_call_t *)sys_getegid },		/* 202 = getegid */
	{ AS(setreuid_args), (sy_call_t *)sys_setreuid },	/* 203 = setreuid */
	{ AS(setregid_args), (sy_call_t *)sys_setregid },	/* 204 = setregid */
	{ AS(linux_getgroups_args), (sy_call_t *)sys_linux_getgroups },	/* 205 = linux_getgroups */
	{ AS(linux_setgroups_args), (sy_call_t *)sys_linux_setgroups },	/* 206 = linux_setgroups */
	{ AS(fchown_args), (sy_call_t *)sys_fchown },	/* 207 = fchown */
	{ AS(setresuid_args), (sy_call_t *)sys_setresuid },	/* 208 = setresuid */
	{ AS(getresuid_args), (sy_call_t *)sys_getresuid },	/* 209 = getresuid */
	{ AS(setresgid_args), (sy_call_t *)sys_setresgid },	/* 210 = setresgid */
	{ AS(getresgid_args), (sy_call_t *)sys_getresgid },	/* 211 = getresgid */
	{ AS(linux_chown_args), (sy_call_t *)sys_linux_chown },	/* 212 = linux_chown */
	{ AS(setuid_args), (sy_call_t *)sys_setuid },	/* 213 = setuid */
	{ AS(setgid_args), (sy_call_t *)sys_setgid },	/* 214 = setgid */
	{ AS(linux_setfsuid_args), (sy_call_t *)sys_linux_setfsuid },	/* 215 = linux_setfsuid */
	{ AS(linux_setfsgid_args), (sy_call_t *)sys_linux_setfsgid },	/* 216 = linux_setfsgid */
	{ AS(linux_pivot_root_args), (sy_call_t *)sys_linux_pivot_root },	/* 217 = linux_pivot_root */
	{ AS(linux_mincore_args), (sy_call_t *)sys_linux_mincore },	/* 218 = linux_mincore */
	{ 0, (sy_call_t *)sys_linux_madvise },		/* 219 = linux_madvise */
	{ AS(linux_getdents64_args), (sy_call_t *)sys_linux_getdents64 },	/* 220 = linux_getdents64 */
	{ AS(linux_fcntl64_args), (sy_call_t *)sys_linux_fcntl64 },	/* 221 = linux_fcntl64 */
	{ 0, (sy_call_t *)sys_nosys },			/* 222 = none */
	{ 0, (sy_call_t *)sys_nosys },			/* 223 = none */
	{ 0, (sy_call_t *)sys_nosys },			/* 224 = linux_gettid */
	{ 0, (sy_call_t *)sys_nosys },			/* 225 = linux_readahead */
	{ 0, (sy_call_t *)sys_linux_setxattr },		/* 226 = linux_setxattr */
	{ 0, (sy_call_t *)sys_linux_lsetxattr },	/* 227 = linux_lsetxattr */
	{ 0, (sy_call_t *)sys_linux_fsetxattr },	/* 228 = linux_fsetxattr */
	{ 0, (sy_call_t *)sys_linux_getxattr },		/* 229 = linux_getxattr */
	{ 0, (sy_call_t *)sys_linux_lgetxattr },	/* 230 = linux_lgetxattr */
	{ 0, (sy_call_t *)sys_linux_fgetxattr },	/* 231 = linux_fgetxattr */
	{ 0, (sy_call_t *)sys_linux_listxattr },	/* 232 = linux_listxattr */
	{ 0, (sy_call_t *)sys_linux_llistxattr },	/* 233 = linux_llistxattr */
	{ 0, (sy_call_t *)sys_linux_flistxattr },	/* 234 = linux_flistxattr */
	{ 0, (sy_call_t *)sys_linux_removexattr },	/* 235 = linux_removexattr */
	{ 0, (sy_call_t *)sys_linux_lremovexattr },	/* 236 = linux_lremovexattr */
	{ 0, (sy_call_t *)sys_linux_fremovexattr },	/* 237 = linux_fremovexattr */
	{ 0, (sy_call_t *)sys_nosys },			/* 238 = linux_tkill */
	{ 0, (sy_call_t *)sys_nosys },			/* 239 = linux_sendfile64 */
	{ 0, (sy_call_t *)sys_nosys },			/* 240 = linux_futex */
	{ 0, (sy_call_t *)sys_nosys },			/* 241 = linux_sched_setaffinity */
	{ 0, (sy_call_t *)sys_nosys },			/* 242 = linux_sched_getaffinity */
	{ 0, (sy_call_t *)sys_nosys },			/* 243 = linux_set_thread_area */
	{ 0, (sy_call_t *)sys_nosys },			/* 244 = linux_get_thread_area */
	{ 0, (sy_call_t *)sys_nosys },			/* 245 = linux_io_setup */
	{ 0, (sy_call_t *)sys_nosys },			/* 246 = linux_io_destroy */
	{ 0, (sy_call_t *)sys_nosys },			/* 247 = linux_io_getevents */
	{ 0, (sy_call_t *)sys_nosys },			/* 248 = linux_io_submit */
	{ 0, (sy_call_t *)sys_nosys },			/* 249 = linux_io_cancel */
	{ 0, (sy_call_t *)sys_linux_fadvise64 },	/* 250 = linux_fadvise64 */
	{ 0, (sy_call_t *)sys_nosys },			/* 251 =  */
	{ AS(linux_exit_group_args), (sy_call_t *)sys_linux_exit_group },	/* 252 = linux_exit_group */
	{ 0, (sy_call_t *)sys_nosys },			/* 253 = linux_lookup_dcookie */
	{ 0, (sy_call_t *)sys_nosys },			/* 254 = linux_epoll_create */
	{ 0, (sy_call_t *)sys_nosys },			/* 255 = linux_epoll_ctl */
	{ 0, (sy_call_t *)sys_nosys },			/* 256 = linux_epoll_wait */
	{ 0, (sy_call_t *)sys_nosys },			/* 257 = linux_remap_file_pages */
	{ 0, (sy_call_t *)sys_nosys },			/* 258 = linux_set_tid_address */
	{ 0, (sy_call_t *)sys_nosys },			/* 259 = linux_timer_create */
	{ 0, (sy_call_t *)sys_nosys },			/* 260 = linux_timer_settime */
	{ 0, (sy_call_t *)sys_nosys },			/* 261 = linux_timer_gettime */
	{ 0, (sy_call_t *)sys_nosys },			/* 262 = linux_timer_getoverrun */
	{ 0, (sy_call_t *)sys_nosys },			/* 263 = linux_timer_delete */
	{ 0, (sy_call_t *)sys_nosys },			/* 264 = linux_clock_settime */
	{ 0, (sy_call_t *)sys_nosys },			/* 265 = linux_clock_gettime */
	{ 0, (sy_call_t *)sys_nosys },			/* 266 = linux_clock_getres */
	{ 0, (sy_call_t *)sys_nosys },			/* 267 = linux_clock_nanosleep */
};
