#pragma once
#include <stdint.h>

#define NULL ((void *)0)

/*
We use the builtin and not the old trick with a null pointer. The compiler
knows where a member is, we don't, and the trick is undefined behaviour.
*/
#define offsetof(type, member) __builtin_offsetof(type, member)

typedef unsigned long size_t;
typedef long ptrdiff_t;

#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2