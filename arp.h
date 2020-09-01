#pragma once
#ifdef __cplusplus
extern "C" {
#endif 


// Really simply arp implementation, only has to remember one entry
// but we'll hide that behind the interface

#include "udp.h"


// Initialise the cache
void arp_init(void);

// Set the one-and-only entry.
void arp_add(_ip_addr_t* ip, _eth_addr_t* mac);

// get the mac for given IP address
void arp_get(_ip_addr_t* ip, _eth_addr_t* mac);

// Process this arp packet and reply if needed
int arp_check(unsigned char* pkt);


#ifdef __cplusplus
}
#endif
