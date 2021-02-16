#pragma once

/*
    ALL SYSTEM CALLS

%rax	System call	        %rdi	            %rsi	                %rdx	            %r10	            %r8	            %r9
0	    sys_read	        unsigned int fd	    char *buf	            size_t count
    read input from FD (file descriptor)
1	    sys_write	        unsigned int fd	    const char *buf	        size_t count
    write data into FD (file descriptor)
60	    sys_exit	        int error_code
    exit the currently running program with an error code (0 for no error)
*/

/**
 * @brief Initialize system calls
 * 
 */
void init_syscalls();