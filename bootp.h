#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

#include "udp.h"

#define BOOTP_SZFNAME		128   // size of file name field

// Copy ip address and name into supplied buffer.
unsigned short bootp_getaddr(_ip_addr_t* saddr, const char* hostname, char* fname);

#ifdef __cplusplus
}
#endif
