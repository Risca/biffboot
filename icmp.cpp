
#include "arp.h"
#include "iolib.h"
#include "stdlib.h"
#include <string.h>
#include "r6040.h"
#include "icmp.h"


typedef struct _icmp_pkt
{
  _ip_pkt_t ip;          // eth == 0x0800 (it's IP, after all)
  u8 type;		// 8 = request
  u8 code;		// 00?
  u16 checksum;		// we don't care
  u16 identifier;	// we don't care
  u16 sequence;		// don't care.
  u8 data[56];		// will be garbage?
} ETHER_PACK _icmp_pkt_t;

#define ICMP_PAYLOAD_SIZE (sizeof(_icmp_pkt_t) - sizeof(_ip_pkt_t))
#define ICMP_PROTO 1


// Check if this a request for our IP address, and if so, send response.
// length not needed as we're always inside the min. pkt size.
int icmp_check(unsigned char* pkt)  
{
  _eth_addr_t reply_eth;
  _ip_addr_t reply_ip;
  
  _icmp_pkt_t* icmp = (_icmp_pkt_t*)pkt;

  if (htons(icmp->ip.eth.proto) != 0x0800) return 0;  // not for us
  if (icmp->ip.protocol != 1) return 0;  // not ICMP, it seems.
  
  // so it's ICMP, it's handle or ditch from here.  
  if (!ip_ismyip(&icmp->ip.daddr)) return 1;  // not for this IP
  
  if (icmp->type != 8) return 1;  // not a request

  // finally, handle the response.  We need to prepare an IP packet.
  reply_eth = icmp->ip.eth.src;
  reply_ip = icmp->ip.saddr;

  arp_add(&reply_ip, &reply_eth);  
  
  // len is length of payload only
  ip_prepsend(&icmp->ip, &reply_ip, ICMP_PAYLOAD_SIZE, ICMP_PROTO);
  
  
  icmp->ip.eth.dst = reply_eth;  // return to sender
  icmp->type = 0;		// it's now a reply
  
  // Calc checksum
  icmp->checksum = 0;
  
  // Checksum the packet minus ether and IP headers.
  icmp->checksum = htons(ip_buf_chksum((u8*)&(icmp->type), 
            ICMP_PAYLOAD_SIZE, 0));

  r6040_tx((unsigned char*)icmp, sizeof(_icmp_pkt_t));
  return 1; // processed
}

