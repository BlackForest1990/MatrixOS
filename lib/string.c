// kernel/lib/string.c
#include "string.h"

void* memset(void* ptr, int value, size_t num) {
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (uint8_t)value;
    }
    return ptr;
}

void* memcpy(void* dest, const void* src, size_t num) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < num; i++) {
        d[i] = s[i];
    }
    return dest;
}

void* memmove(void* dest, const void* src, size_t num) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    
    if (d < s) {
        for (size_t i = 0; i < num; i++) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = num; i > 0; i--) {
            d[i-1] = s[i-1];
        }
    }
    return dest;
}

int memcmp(const void* ptr1, const void* ptr2, size_t num) {
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;
    for (size_t i = 0; i < num; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while (*src) {
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}

char* strncpy(char* dest, const char* src, size_t num) {
    char* d = dest;
    size_t i;
    for (i = 0; i < num && src[i]; i++) {
        d[i] = src[i];
    }
    for (; i < num; i++) {
        d[i] = '\0';
    }
    return dest;
}

char* strcat(char* dest, const char* src) {
    char* d = dest;
    while (*d) {
        d++;
    }
    while (*src) {
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}

int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

int strncmp(const char* str1, const char* str2, size_t num) {
    for (size_t i = 0; i < num; i++) {
        if (str1[i] != str2[i]) {
            return str1[i] - str2[i];
        }
        if (str1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}
