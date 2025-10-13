#ifndef TYPES_H
#define TYPES_H

/* 标准整数类型定义 */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;

/* 布尔类型 */
typedef uint8_t bool;
#define true 1
#define false 0

/* 大小类型 */
typedef uint32_t size_t;

/* 空指针类型 */
typedef void* ptr_t;

#define NULL ((void*)0)
#define PAGE_SIZE 4096

#endif /* TYPES_H */
