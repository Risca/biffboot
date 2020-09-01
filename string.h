#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

#include "types.h"

void* memcpy(void* dest, const void *src, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);
void* memset(void *s, int c, size_t n);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
size_t strlen(const char* txt);
int strcmp(const char *s1, const char *s2);
int strncmp(const char* s1, const char* s2, size_t len);

#ifdef __cplusplus
}
#endif

