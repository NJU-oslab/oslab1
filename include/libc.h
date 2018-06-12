#ifndef _LIBC_H
#define _LIBC_H

#include <stdint.h>
#include <am.h>
// string.h
void *memset(void *b, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
size_t strlen(const char* s);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strcat(char *s1, char *s2);

// stdlib.h
void srand(unsigned int seed);
int rand();

// stdio.h
int printf(const char* fmt, ...);
int sprintf(char* out, const char* fmt, ...);
int snprintf(char* s, size_t n, const char* fmt, ...);

#endif