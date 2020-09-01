#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

#include "setup.h"
#include "iolib.h"

#define ETHER_PACK __attribute__((__packed__))

typedef struct _eth_addr
{
  unsigned short addr0;
  unsigned short addr1;
  unsigned short addr2;
} ETHER_PACK _eth_addr_t;


typedef struct _eth_pkt
{
  _eth_addr_t dst;
  _eth_addr_t src;
  unsigned short proto;
} ETHER_PACK _eth_pkt_t;



// Fill in the mac address
void ether_getmymac(_eth_addr_t* addr);
// Fill out a broadcast
void ether_getbcast(_eth_addr_t* addr);

void ether_getzero(_eth_addr_t* addr);


// Prepare a complete broadcast pkt
void ether_prep_broad(_eth_pkt_t* hdr);


u16 htons(u16);
u32 htonl(u32);

int ether_isequal(_eth_addr_t* a1, _eth_addr_t* a2);
int ether_ismymac(_eth_addr_t* addr);
int ether_isbcast(_eth_addr_t* addr);


void ether_print(_eth_addr_t* addr);

int ether_mac_unset();
void ether_entermac(const char* args);


//#ifdef COMMUNITY_EDITION
#define ETHER_MAC ((_eth_addr_t*)0xfffffff8)
//#else
//#define ETHER_MAC ((_eth_addr_t*)(BIOS_RUN_ADDR-BIOS_HDR_LEN))
//#endif


#ifdef __cplusplus
}
#endif
