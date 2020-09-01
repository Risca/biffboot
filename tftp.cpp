
#include <string.h>
#include <stdio.h>
#include "udp.h"
#include "tftp.h"
#include "r6040.h"
#include "arp.h"
#include "timer.h"
#include "iolib.h"


typedef struct _rrq
{
  unsigned short op;
  char data[512];      // make sure it deals with the largest 
                       // requested packets
} ETHER_PACK _rrq_t;

typedef struct _ack
{
  unsigned short op;
  unsigned short block;
} ETHER_PACK _ack_t;


typedef struct _data
{
  unsigned short op;
  unsigned short block;
  char data[512];      // also mode
} ETHER_PACK _data_t;


typedef struct _rrq_pkt 
{
  _udp_pkt_t udp;
  _rrq_t rrq;
} ETHER_PACK _rrq_pkt_t;

typedef struct _ack_pkt 
{
  _udp_pkt_t udp;
  _ack_t ack;
} ETHER_PACK _ack_pkt_t;


typedef struct _data_pkt 
{
  _udp_pkt_t udp;
  _data_t data;
} ETHER_PACK _data_pkt_t;



static unsigned short g_random;  // randomly allocated src port
static _rrq_pkt_t* g_rrq;   // packet for request transmission
static _ack_pkt_t* g_ack;   // packet for ack transmission
static unsigned char* g_pkt;   // general rx packet
static _ip_addr_t g_server_ip;   // the server IP address

void tftp_setrandom(unsigned short port)
{
  if (!port) port ++;
  if (port == 0xffff) port--;
  g_random = port;
}



void tftp_init()
{
  g_rrq = (_rrq_pkt_t*)malloc(sizeof(_rrq_pkt_t));
  g_ack = (_ack_pkt_t*)malloc(sizeof(_ack_pkt_t));
  g_pkt = (unsigned char*)malloc(600);
}


void tftp_rrq(_ip_addr_t* saddr, char* bootpath)
{
  char* ptr;
  unsigned short length;
  
  // Fill in the tftp request
  g_rrq->rrq.op = htons(1);  // read request
  
  // assume bootpath less than data
  strcpy(g_rrq->rrq.data, bootpath);
  ptr = g_rrq->rrq.data;
  ptr += strlen(bootpath);
  ptr ++;  // skip over the null
  strcpy(ptr, "octet");
  ptr += strlen("octet");
  ptr ++;  // skip the null
  
  length = (ptr - ((char*)&g_rrq->rrq));
  
  // Calculate the effective payload size.
  // Fill in the udp parts of the read request.
  // length here, is the payload length
  udp_prepsend(&g_rrq->udp, saddr, 69, g_random, length);

  r6040_tx((unsigned char*)g_rrq, sizeof(g_rrq->udp)+length);
}


// Send an ACK to the given port, for the given block number
// Fills in the remaining parts of the UDP pkt, which has already been
// initialised.
void tftp_ack(unsigned short dport, unsigned short block)
{
  udp_prepsend(&g_ack->udp, &g_server_ip, dport, g_random, sizeof(g_ack->ack));
  g_ack->udp.dport = htons(dport);  // now we have it
  g_ack->ack.op = htons(4);
  g_ack->ack.block = htons(block);
  r6040_tx((unsigned char*)g_ack, sizeof(_ack_pkt_t));  
}


// dest is the place to copy the (single) boot file to
// saddr is the server addres bootpath is the bootpath
unsigned long tftp_getfile(char* dest, _ip_addr_t* saddr, char* bootpath)
{
  unsigned short expected_block;
  _data_pkt_t* data_pkt;
  int first; // is this the first pkt?
  unsigned short server_port; // the port the server uses for the connection
  unsigned long total;

  g_server_ip = *saddr;
  tftp_init();
  r6040_rx_enable();	// start reception of packets

  // Send initial transfer request
  tftp_rrq(saddr, bootpath);    
  
  // Start of sequence.
  expected_block = 1;
  first = 1;
  total = 0;

  while (1)  // wait for transfer to complete.
  {
    int rx_len;
    rx_len = r6040_rx(g_pkt, 600);  // big enough for data packets
    if (rx_len)
    {
      // check if it's ARP and process if needed
      if (arp_check(g_pkt)) continue;

      // Make sure it's really UDP protocol bound for this address/port
      if (!udp_isforme(g_pkt)) continue;
      
      // It's for us, act on it.
      data_pkt = (_data_pkt_t*)g_pkt;
      if (data_pkt->data.op == htons(3))  // 3==data
      {
        unsigned short block;
        block = htons(data_pkt->data.block);
        if (first)
        {
          server_port = htons(data_pkt->udp.sport);
          first = 0;
        }
        if (block == expected_block)
        {
          unsigned short length;
          // How much data?
          length = htons(data_pkt->udp.len)-12;
          // copy bytes to memory out of the packet
          memcpy(dest, data_pkt->data.data, length);
          // advance the pointer for the next block
          dest += length;
          total += length;
          expected_block++;   // expect the next
        
          // Put together an ack for this packet
          tftp_ack(server_port, block);
          if (length != 512) break;   // last pkt.
        } 
        else if (block < expected_block)
        {
          // Already got this block but ACK a 2nd time
          tftp_ack(server_port, block);
          printf("TFTP Error, pkt received out of order (block<expected)\n");
        }
        else
        {
          // Just start again in this case
          printf("TFTP Error, pkt received out of order (block>=expected)\n");
          total = 0;
          break;
        }
      } else 
      {
        printf("TFTP: Received invalid response from server\n");
        total = 0;
        break;
      }
    } 
  }    
  
  r6040_rx_disable();    // stop the receiver while we continue with boot
  return total;
}

