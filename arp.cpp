
#include "arp.h"
#include "iolib.h"
#include "stdlib.h"
#include <string.h>
#include <stdio.h>
#include "r6040.h"
#include "timer.h"

// Really simple table consisting of 1 entry.
static _ip_addr_t g_ip;
static _eth_addr_t g_mac;


typedef struct _arp_pkt
{
  _eth_pkt_t eth;          // 0x0806
  u16 hwtype;   // 0x0001
  u16 ptype;    // 0x0800
  unsigned char  hwsize;   // 6
  unsigned char  psize;    // 4
  u16 op;       // 1 = request, 2= reply
  _eth_addr_t hwsender;
  _ip_addr_t  ipsender;
  _eth_addr_t hwtarget;
  _ip_addr_t  iptarget;
  char padding[30];
} ETHER_PACK _arp_pkt_t;


// Space in which to compose any reply.
static _arp_pkt_t* g_arp_reply;
static _arp_pkt_t* g_arp_request;


void arp_add(_ip_addr_t* ip, _eth_addr_t* mac)
{
  memcpy(&g_ip,  ip,  sizeof(_ip_addr_t));
  memcpy(&g_mac, mac, sizeof(_eth_addr_t));

  // printf("ARP: "); ip_print(ip); printf(" --> "); ether_printf(mac); 
  // printf("\n");
  
  return;
}



// Check if this a request for our IP address, and if so, send response.
// length not needed as we're always inside the min. pkt size.
int arp_check(unsigned char* pkt)  
{
  _arp_pkt_t* arp = (_arp_pkt_t*)pkt;

  // Not arp, ignore
  if (htons(arp->eth.proto) != 0x0806) return 0;
  // Check validity of arp header
  if (htons(arp->hwtype) != 0x0001) return 1;
  if (htons(arp->ptype) != 0x0800) return 1;
  if (arp->hwsize != 6) return 1;
  if (arp->psize != 4) return 1;
  
  // Check the operation
  if (htons(arp->op) == 1)  // request, we have to send a response if it's for us
  {
    if (!ip_ismyip(&arp->iptarget)) return 1;  // processed, not for us
    // it's for us: we need to formulate a response.
    // printf("ARP incoming from "); ip_print(&arp->ipsender); printf("\n");
    g_arp_reply->eth.dst = arp->eth.src;  // return to sender
    ether_getmymac(&g_arp_reply->eth.src);
    g_arp_reply->eth.proto = htons(0x0806);
    g_arp_reply->hwtype = htons(0x0001);
    g_arp_reply->ptype = htons(0x0800);
    g_arp_reply->hwsize = 6;
    g_arp_reply->psize = 4;
    g_arp_reply->op = htons(2);  // reply
    ether_getmymac(&g_arp_reply->hwsender);
    ip_getmyip(&g_arp_reply->ipsender);
    g_arp_reply->hwtarget = arp->eth.src;
    g_arp_reply->iptarget = arp->ipsender;
    r6040_tx((unsigned char*) g_arp_reply, sizeof(_arp_pkt_t));
    return 1; // processed
  }
  if (htons(arp->op) == 2)  // the arp packet we requested?
  {
    if (!ether_ismymac(&arp->hwtarget)) return 1; // not for me.
    // add to cache, we must have requested this one.
    arp_add(&arp->ipsender, &arp->hwsender);
  }
  return 1;  // processed, drop it.
}


// Arp entries have expired or this is init
void arp_reset()
{
  memset(&g_ip, 0, sizeof(_ip_addr_t));
  memset(&g_mac, 0, sizeof(_eth_addr_t));
}


void arp_init(void)
{
  arp_reset();
  // Clear the table.
  // Allocate space for protocol packets.
  g_arp_reply = (_arp_pkt_t*)malloc(sizeof(_arp_pkt_t));
  g_arp_request = (_arp_pkt_t*)malloc(sizeof(_arp_pkt_t));
}




// Get the MAC for given IP address.
void arp_get(_ip_addr_t* ip, _eth_addr_t* mac)
{
  int count;
  int rx_len;
  if (ip_isequal(ip, &g_ip))
  {
    memcpy(mac, &g_mac, sizeof(_eth_addr_t));
    return;
  }  
  // TODO: Make the ARP request for the new host here.
  ether_getbcast(&g_arp_request->eth.dst);
  ether_getmymac(&g_arp_reply->eth.src);
  g_arp_request->eth.proto = htons(0x0806);
  g_arp_request->hwtype = htons(0x0001);
  g_arp_request->ptype = htons(0x0800);
  g_arp_request->hwsize = 6;
  g_arp_request->psize = 4;
  g_arp_request->op = htons(2);  // reply
  ether_getmymac(&g_arp_request->hwsender);
  ip_getmyip(&g_arp_request->ipsender);
  ether_getzero(&g_arp_request->hwtarget);
  g_arp_request->iptarget = *ip;
  
  count = 100;
  while (1)
  {  
    if (!(count++ % 100))  // send a packet every second
    {
      r6040_tx((unsigned char*)g_arp_request, sizeof(_arp_pkt_t));
      printf(".");
    }
  
    timer_mdelay10();   // wait 10mS for the action
    // check the rx buffer
    rx_len = r6040_rx((unsigned char*)g_arp_reply, sizeof(_arp_pkt_t));
    if (rx_len)
    {
      arp_check((unsigned char*)g_arp_reply);   // ignore return, just check cache
      if (ip_isequal(ip, &g_ip))
      {
        memcpy(mac, &g_mac, sizeof(_eth_addr_t));
        break;
      }
    }
  }
}

