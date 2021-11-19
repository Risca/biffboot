
#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include "stddef.h"


extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int putchar(int c);
int puts(const char *s);
int printf(const char *format, ...);


int get_hex8(char* buffer, u32 len, u8 a);


// ignores stream arg, just sends to output
int fprintf(FILE *stream, const char *format, ...);

void serial_enable(int enable);


#ifdef __cplusplus
}
#endif

