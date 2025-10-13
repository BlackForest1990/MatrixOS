// include/string.h
#ifndef STRING_H
#define STRING_H

#include "types.h"

// 内存操作函数
void* memset(void* ptr, int value, size_t num);
void* memcpy(void* dest, const void* src, size_t num);
void* memmove(void* dest, const void* src, size_t num);
int memcmp(const void* ptr1, const void* ptr2, size_t num);

// 字符串操作函数
size_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t num);
char* strcat(char* dest, const char* src);
int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, size_t num);

#endif
