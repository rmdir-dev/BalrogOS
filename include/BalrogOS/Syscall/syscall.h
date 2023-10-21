#pragma once

#include "balrog/debug/debug.h"

/*
    ALL SYSTEM CALLS

%rax	System call	        %rdi	            %rsi	                %rdx	            %r10	            %r8	            %r9
0	    sys_read	        unsigned int fd	    char *buf	            size_t count
    read input from FD (file descriptor)
1	    sys_write	        unsigned int fd	    const char *buf	        size_t count
    write data into FD (file descriptor)
2	    sys_open	        const char *fname	int flags	            int mode
    open a file/pipe
3	    sys_close	        unsigned int fd
    close a file/pipe
4	    sys_stat	        const char *fname   struct stat *statbuf
    get file inode using file name
5	    sys_fstat	        unsigned int fd	    struct stat *statbuf
    get file inode using file descriptor
12	    sys_brk	            unsigned long brk
    change the location of the program break (the program break is the first location after the end of the uninitialized
       data segment)
39      sys_getpid
    get the currently running process ID.
57	    sys_fork
    fork current process (create a child process that is a copy of the caller).
    return the child PID
59	    sys_execve	        const char *fname	const char *const argv[] const char *const envp[]
    exec a new process
60	    sys_exit	        int error_code
    exit the currently running program with an error code (0 for no error)
61	    sys_wait4	        pid_t upid	        int *stat_addr	        int options	        struct rusage *ru
    wait for a process with pid to exit.
62	    sys_kill	        pid_t pid	        int sig
    send a signal to a process
169      sys_reboot        	int magic1          int magic2              unsigned int cmd    void *arg
    shutdown the system.
202     sys_park            uint64 pid
    park the current process if pid == 0 else unpark the process with that pid
203     sys_setpark         
    prepare to park the current process, if the current process is unparked in between the setpark and park call, 
    then it won't be parked at all.
*/

#define SYS_READ        0
#define SYS_WRITE       1
#define SYS_OPEN        2
#define SYS_CLOSE       3
#define SYS_STAT        4
#define SYS_FSTAT       5
#define SYS_BRK         12
#define SYS_NANOSLEEP   35
#define SYS_GETPID      39
#define SYS_FORK        57
#define SYS_EXECVE      59
#define SYS_EXIT        60
#define SYS_WAIT        61
#define SYS_KILL        62
#define SYS_GETCWD      79
#define SYS_CHDIR       80
#define SYS_GETUID      102
#define SYS_SETUID      105
#define SYS_GETPPID     110
#define SYS_REBOOT      169
#define SYS_FUTEX       202
#define SYS_PARK        202 // TODO switch to futex
#define SYS_SETPARK     203 // TODO switch to futex
#define SYS_DEBUG       KDB_CALL // TODO currently at 254 change to 300 + if we configure sys_inotify_add_watch

/**
 * @brief Initialize system calls
 * 
 */
void init_syscalls();