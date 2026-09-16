#pragma once

#include <sys/type.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
    PROCESS
*/

/**
 * @brief 
 * 
 * @return pid_t 
 */
pid_t getpid();

/**
 * @brief 
 * 
 * @param name 
 * @param argv 
 * @return int 
 */
int execv(const char *name, char *const argv[]);

/**
 * @brief 
 * 
 * @return pid_t 
 */
pid_t fork();

/**
 * @brief 
 * 
 * @param pid 
 * @param status 
 * @param option 
 * @return pid_t 
 */
pid_t waitpid(pid_t pid, int* status, int option);

/**
 * @brief 
 * 
 * @param code 
 */
void exit(int code);

/**
 * @brief
 *
 * @param pid
 * @param sig
 * @return int
 */
int kill(pid_t pid, int sig);

/**
 * @brief 
 * 
 * @param fd 
 * @param buf 
 * @param count 
 * @return int
 */
int read(int fd, void* buf, size_t count);

/**
 * @brief 
 * 
 * @param fd 
 * @param buf 
 * @param count 
 * @return int
 */
int write(int fd, void* buf, size_t count);

/**
 * @brief 
 * 
 * @param fd 
 * @return int 
 */
int close(int fd);

/**
 * @brief create an empty file
 * 
 * @param pathname 
 * @param mode 
 * @return int 
 */
int creat(const char* pathname, int mode);

/**
 * @brief create an empty directory
 * 
 * @param pathname 
 * @param mode 
 * @return int 
 */
int mkdir(const char* pathname, int mode);

/**
 * @brief remove a file
 * 
 * @param pathname 
 * @return int 
 */
int unlink(const char* pathname);

/**
 * @brief remove an empty directory
 * 
 * @param pathname 
 * @return int 
 */
int rmdir(const char* pathname);

/**
 * @brief brk() sets the end of the data segment to the value specified by
    addr, when that value is reasonable, the system has enough
    memory.
 * 
 * @param addr the new program break address.
 * @return int 
 */
int brk(void* addr);

/**
 * @brief
 *
 * @param uid
 */
void setuid(int uid);

/**
 * @brief
 *
 * @return
 */
int getuid();

/**
 * @brief
 *
 * @param buf
 * @param size
 * @return
 */
char* getcwd(char* buf, int size);

/**
 * @brief
 *
 * @param dir
 * @return
 */
int chdir(char* dir);

/**
 * @brief
 * @param ms
 * @return
 */
int sleep(size_t ms);


/**
 * @brief Reboot or shutdown system
 * @param option BALROG_REBOOT_POWER_OFF | BALROG_REBOOT_RESTART
 * @return
 */
int reboot(int option);

#ifdef __cplusplus
}
#endif