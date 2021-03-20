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
 * @param fd 
 * @param buf 
 * @param count 
 * @return size_t 
 */
size_t read(int fd, void* buf, size_t count);

/**
 * @brief 
 * 
 * @param fd 
 * @param buf 
 * @param count 
 * @return size_t 
 */
size_t write(int fd, void* buf, size_t count);

/**
 * @brief 
 * 
 * @param fd 
 * @return int 
 */
int close(int fd);

#ifdef __cplusplus
}
#endif