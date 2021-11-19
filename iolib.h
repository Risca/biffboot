#pragma once
#ifdef __cplusplus
extern "C" {
#endif  

#include "types.h"

#define IOLIB_CURSOR_UP 202
#define IOLIB_CURSOR_DOWN 203
#define IOLIB_CURSOR_LEFT 204
#define IOLIB_CURSOR_RIGHT 205

int print_biffboot(const char* p);
int getkey(int timeout);
// tells if key was pressed
int keyready();
u32 getint(int digits);
void hex_dump(void* buffer, int length);
u32 hex_to_u32(char* buffer, u32 length);
void get_serial_data(u8* pkt, u32 length);

typedef const char* (*match_fn)(const char*);

char get_ascii_data(char* pkt, u32 max, char first_key, int hexonly, int history, match_fn callback);


void fatal(const char* msg);

void prompt();
int are_you_sure();
void print_erase(int count);

const char* eat_token(const char* str, const char *tok);

void stream_write(char** ptr, const char* val);
void stream_write_char(char** ptr, const char val);


#ifdef __cplusplus
}
#endif

