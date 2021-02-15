#pragma once

/*
%rax	System call	        %rdi	            %rsi	                %rdx	            %r10	            %r8	            %r9
0	    sys_read	        unsigned int fd	    char *buf	            size_t count
1	    sys_write	        unsigned int fd	    const char *buf	        size_t count
60	    sys_exit	        int error_code
*/

/**
 * @brief Initialize system calls
 * 
 */
void init_syscalls();