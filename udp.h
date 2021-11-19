#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

//
// Definitions of ethernet packets.

#include "ether.h"


typedef struct _ip_addr
{
  unsigned char a1;
  unsigned char a2;
  unsigned char a3;
  unsigned char a4;
} ETHER_PACK _ip_addr_t;


typedef struct _ip_pkt
{
  _eth_pkt_t eth;
  unsigned char verihl;   // normally 0x45
  unsigned char tos;      // normally 0x00
  unsigned short len;     // big endian - length of pkt.
  unsigned short id;      // can be zero
  unsigned short frag;    // flg+fragment
  unsigned char time;     // 123
  unsigned char protocol; // 1
  unsigned short chksum;  // all zero
  _ip_addr_t saddr;
  _ip_addr_t daddr;
} ETHER_PACK _ip_pkt_t;


typedef struct _udp_pkt
{
  _ip_pkt_t ip;
  unsigned short sport;
  unsigned short dport;
  unsigned short len;
  unsigned short cksum;
} ETHER_PACK _udp_pkt_t;


void ip_init(void);

void ip_prepsend(_ip_pkt_t *hdr, _ip_addr_t* dest, unsigned short len,
                                                      u8 protocol);

void ip_prep_broad(_ip_pkt_t *hdr, unsigned short len);

void udp_prep_broad(_udp_pkt_t *hdr,
                  unsigned short dport, unsigned short sport,
                  unsigned short len);


// Sends to specific address
void udp_prepsend(_udp_pkt_t *hdr,_ip_addr_t* daddr, 
                  unsigned short dport, unsigned short sport,
                  unsigned short len);


void ip_get_bcast(_ip_addr_t* ip);
void ip_get_zero(_ip_addr_t* ip);


u16 ip_buf_chksum(u8 *buf, u16 len, u32 sum);
unsigned short ip_chksum(_ip_pkt_t *hdr);

void ip_print(_ip_addr_t* ip);

int ip_isequal(_ip_addr_t* a1, _ip_addr_t* a2);

int ip_ismyip(_ip_addr_t* addr);


// Set the ip address of this client, when we have it.
void ip_set(_ip_addr_t* ip);

void ip_getmyip(_ip_addr_t* ip);

int ip_isforme(unsigned char* pkt);

int udp_isforme(unsigned char* pkt);

#ifdef __cplusplus
}
#endif

