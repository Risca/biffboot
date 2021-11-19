

// Definitions of ethernet packets.

#include <string.h>
#include <stdio.h>
#include "ether.h"
#include "udp.h"
#include "iolib.h"
#include "arp.h"


static _ip_addr_t g_my_ip;    // our IP, once we have it
static unsigned short g_my_port;   // our port, from the pkt prep.


void ip_init(void)
{
  ip_get_zero(&g_my_ip);
}

// Return 255.255.255.255
void ip_get_bcast(_ip_addr_t* ip)
{
  memset(ip,0xff, sizeof(_ip_addr_t));   // all zeros (don't know)
}

// Return a 'null' ip address
void ip_get_zero(_ip_addr_t* ip)
{
  memset(ip,0x00, sizeof(_ip_addr_t));   // all 0xff (don't know).
}


u16 ip_buf_chksum(u8 *buf, u16 len, u32 sum)
{
  u16 out;
    
	// build the sum of 16bit words
	while(len>1)
	{
		sum += 0xFFFF & (*buf<<8|*(buf+1));
		buf+=2;
		len-=2;
	}
	// if there is a byte left then add it (padded with zero)
	if (len)
		sum += (0xFF & *buf)<<8;
		
	// now calculate the sum over the bytes in the sum
	// until the result is only 16bit long
	while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);

	// build 1's complement:
	out = ( (u16) sum ^ 0xFFFF);
	return out;
}


unsigned short ip_chksum(_ip_pkt_t *hdr)
{
   return htons(ip_buf_chksum((u8*)&hdr->verihl, 20, 0));
}


// Setup a basic ip packet header for broadcast
void ip_prep_broad(_ip_pkt_t *hdr, unsigned short len)
{
  unsigned short csum;
  ether_prep_broad(&hdr->eth);

  hdr->verihl = 0x45;
  hdr->tos = 0x00;
  hdr->len = htons(len+20);  // IP header length + payload
  hdr->id = 0;
  hdr->frag = htons(0x4000);  // fragment not allowed
  hdr->time = 0x40;
  hdr->protocol = 17;
  hdr->chksum = 0x0000;       // zero for csum calc.
  hdr->saddr = g_my_ip;
  ip_get_bcast(&hdr->daddr);
  
  csum = ip_chksum(hdr);
  hdr->chksum = csum;  // for some reason it comes out bigendian
}


void udp_prep_broad(_udp_pkt_t *hdr, 
                  unsigned short dport, unsigned short sport,
                  unsigned short len)
{
  u16 csum;
  hdr->sport = htons(sport);
  hdr->dport = htons(dport);
  len += 8;
  hdr->len = htons(len);   // size of udp hdr + payload
  hdr->cksum = 0;

  ip_prep_broad(&hdr->ip, len);  // payload + udphdr.

  // When calculating the UDP header, the IP header is included
  csum = ip_buf_chksum((u8*)&(hdr->ip.verihl), len + 20, 0);
//  hdr->cksum = htons(csum);

}


// Setup a basic ip packet header.  len is IP payload
void ip_prepsend(_ip_pkt_t *hdr, _ip_addr_t* dest, unsigned short len,
                                              u8 protocol)
{
  unsigned short csum;

  ether_getmymac(&hdr->eth.src);  // src no problem.
  // either get dest from arp cache, or get it from the network
  // This may take some time!
  arp_get(dest, &hdr->eth.dst);
  hdr->eth.proto = htons(0x0800);

  // Standard IP stuff follows
  hdr->verihl = 0x45;
  hdr->tos = 0x00;
  hdr->len = htons(len+20);  // IP header length + payload
  hdr->id = 0;
  hdr->frag = htons(0x4000);  // fragment not allowed
  hdr->time = 0x40;
  hdr->protocol = protocol;	// 17=UDP, 1= ICMP
  hdr->chksum = 0x0000;       // zero for csum calc.
  hdr->saddr = g_my_ip;
  hdr->daddr = *dest;
  
  csum = ip_chksum(hdr);
  hdr->chksum = csum;  // for some reason it comes out bigendian from
                      // the Steven's routine.
}


// Fill in header to specific destination
// sport, starts a kind of connection with an endpoint and is stored to
// determine if a pkt is for us later.
void udp_prepsend(_udp_pkt_t* hdr,_ip_addr_t* daddr, 
                  unsigned short dport, unsigned short sport,
                  unsigned short len)
{
  g_my_port = sport;
  hdr->sport = htons(sport);
  hdr->dport = htons(dport);
  len += 8;
  hdr->len = htons(len);   // size of udp hdr + payload
  hdr->cksum = 0;	// zero for csum calculation
  
//  csum = ip_buf_chksum((u8*)&hdr->sport, len, 0);
//  hdr->cksum = htons(csum);

  ip_prepsend(&hdr->ip, daddr, len, 17);  // payload + udphdr.
}


int ip_isforme(unsigned char* pkt)
{
  _ip_pkt_t* ip = (_ip_pkt_t*)pkt;
  if (ether_isbcast(&ip->eth.dst)) return 0;  // ignore
  if (ip->eth.proto != htons(0x0800)) return 0;  // ignore
  if (!ip_ismyip(&ip->daddr)) return 0;
  return 1;
}


// Don't bother calculating the checksum
int udp_isforme(unsigned char* pkt)
{
  _udp_pkt_t* udp = (_udp_pkt_t*)pkt;
  if (!ip_isforme(pkt)) return 0;
  if (udp->dport != htons(g_my_port)) return 0;
  return 1;  
}


// diagnostic
void ip_print(_ip_addr_t* ip)
{
  printf("%d.%d.%d.%d",
    unsigned(ip->a1 & 0xff),
    unsigned(ip->a2 & 0xff),
    unsigned(ip->a3 & 0xff),
    unsigned(ip->a4 & 0xff)
  );
}


// return true if the addresses are the same.
int ip_isequal(_ip_addr_t* a1, _ip_addr_t* a2)
{
  return ((*((unsigned long*)a1)) == (*((unsigned long*)a2))) ? 1 : 0;
}


// if an ip address is 'null'
int ip_iszero(_ip_addr_t* addr)
{
  return (*((unsigned long*)addr)) ? 0 : 1;
}


// If addr has been set, and this is matching, return true
int ip_ismyip(_ip_addr_t* addr)
{
  if (ip_iszero(&g_my_ip)) return 0;  // never matches if unset.
  return ip_isequal(&g_my_ip, addr) ? 1 : 0;
}


void ip_set(_ip_addr_t* ip)
{
  printf("My IP: ");  ip_print(ip);  printf("\n");
  g_my_ip = *ip;
}


void ip_getmyip(_ip_addr_t* ip)
{
  *ip = g_my_ip;
}

