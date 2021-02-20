#pragma once

/*
    ALL SYSTEM CALLS

%rax	System call	        %rdi	            %rsi	                %rdx	            %r10	            %r8	            %r9
0	    sys_read	        unsigned int fd	    char *buf	            size_t count
    read input from FD (file descriptor)
1	    sys_write	        unsigned int fd	    const char *buf	        size_t count
    write data into FD (file descriptor)
39      sys_getpid
    get the currently running process ID.
60	    sys_exit	        int error_code
    exit the currently running program with an error code (0 for no error)
*/

#define SYS_READ        0
#define SYS_WRITE       1
#define SYS_GETPID      39
#define SYS_EXIT        60
#define SYS_FUTEX       202
#define SYS_PARK        202 // TODO switch to futex

/**
 * @brief Initialize system calls
 * 
 */
void init_syscalls();