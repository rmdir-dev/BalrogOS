#pragma once

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned uint32_t;
typedef unsigned long uint64_t;
typedef unsigned long uintptr_t;
typedef unsigned long size_t;

typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long int64_t;
typedef long intptr_t;
typedef long ssize_t;

#define INT8_C(c)    c
#define INT16_C(c)   c
#define INT32_C(c)   c
#define INT64_C(c)   c ## L
#define UINT8_C(c)   c
#define UINT16_C(c)  c
#define UINT32_C(c)  c ## U
#define UINT64_C(c)  c ## UL

#define INT8_MAX     0x7f
#define INT16_MAX    0x7fff
#define INT32_MAX    0x7fffffff
#define INT64_MAX    0x7fffffffffffffffL
#define INT8_MIN     (-INT8_MAX - 1)
#define INT16_MIN    (-INT16_MAX - 1)
#define INT32_MIN    (-INT32_MAX - 1)
#define INT64_MIN    (-INT64_MAX - 1)

#define UINT8_MAX    0xff
#define UINT16_MAX   0xffff
#define UINT32_MAX   0xffffffffU
#define UINT64_MAX   0xffffffffffffffffUL
