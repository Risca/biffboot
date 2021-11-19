#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

#include "udp.h"

unsigned long tftp_getfile(char* dest, _ip_addr_t* saddr, char* bootpath);

      // set a random number for later
void tftp_setrandom(unsigned short random);      

#ifdef __cplusplus
}
#endif
