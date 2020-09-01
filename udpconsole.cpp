
#include <string.h>
#include <stdio.h>
#include "r6040.h"
#include "timer.h"
#include "netconsole.h"
#include "stdlib.h"
#include "iolib.h"
#include "flash.h"
#include "flashmap.h"
#include "transit.h"
#include "gpio.h"
#include "ether.h"
#include "config.h"

#include "idt.h"
#include "gdt.h"
#include "bootp.h"
#include "arp.h"
#include "icmp.h"
#include "udp.h"
#include "udpconsole.h"


typedef struct _udp_cons_pkt
{
  _udp_pkt_t udp; 	// UDP headers
  
  u16 type;     	// type of packet (determines direction)
  u32 arg;      	// length of data block
  u8 data;		// data block
} ETHER_PACK _udp_cons_pkt_t;


#define TYPE_ATTN            0  // console break-in packet
#define TYPE_RELEASE         2  // allow execution to continue
#define TYPE_DATA_RESET      4  // reset data receive buffer (no ack)
#define TYPE_DATA_ADD        6  // send data (no ack).
#define TYPE_DATA_DIGEST     8  // return the digest of the sent data
#define TYPE_DATA_WRITE    0xa  // write block to flash
#define TYPE_ERASE_SECTOR  0xc  // write block to flash

#define TYPE_XILINX	  0x80  // xilinx redirector

#define ERROR_NONE   0x0
#define ERROR_VERIFY 0x1
#define ERROR_FLASH  0x2


static struct _udp_cons_pkt* g_pkt = (_udp_cons_pkt*)NULL;
static _ip_addr_t myaddr;


// len is the UDP payload size, excluding the 6-byte 'header'
static void send(u32 tosend)
{
  _ip_addr_t daddr;
  g_pkt->type++;
  // len is UDP len, i.e. hdr + 6 + tosend
  u16 len = 6 + tosend;
  // prepare response using incoming packet.
  daddr = g_pkt->udp.ip.saddr;
  // Ensure the addr in the ARP cache
  arp_add(&daddr, &g_pkt->udp.ip.eth.src);
  udp_prepsend(&g_pkt->udp, &daddr, 0xb1fe, 0xb1ff, len);
  g_pkt->arg = tosend;
  // len is eth+ip+udp+payload
  r6040_tx((unsigned char*)g_pkt, sizeof(_udp_pkt_t) + 6 + tosend);
}


void udpconsole_run()
{
  u32 ticks;  // erase time
  u32 release = 0;
  char hostname[16];
  char mac_str[10];
  
  strcpy(hostname, "biff");
  r6040_get_mac_suffix(mac_str, sizeof(mac_str));
  strcat(hostname, mac_str);

//  gdt_init();  // needed for idt
//  idt_init();  // needed for timer interrupt

//  timer_init();   // start the timer tick
  printf("Requesting hostname: ");
  printf(hostname); printf("\n");

  printf("Getting IP");

  // dhcp discover our IP address, request specific hostname.
  bootp_getaddr(&myaddr, hostname, (char*)NULL);  // no name needed, just the IP

  g_pkt = (_udp_cons_pkt_t*)malloc(1500);

  transit_init();  // prepare flash buffers
  
  printf("Started UDP flashing server.  Power off then on again to quit.\n");

  while (!release)  // serve until release packet
  {
    int rx_len;
    rx_len = r6040_rx((u8*)g_pkt, 1500);  // big enough for data packets
    if (!rx_len) continue;  // nothing received
    
    // check if it's ARP and process if needed
    if (arp_check((u8*)g_pkt)) continue;

    // reply to pings
    if (icmp_check((u8*)g_pkt)) continue;

    // Make sure it's really UDP protocol
    if (g_pkt->udp.ip.protocol != 17) continue;

    // ensure it's the right port
    if (g_pkt->udp.dport!=htons(0xb1ff)) continue;
      
    // check what it's about    
    switch (g_pkt->type)
    {
      case TYPE_ATTN:
        // ACK that we're in the console
        send(0);
        break;
      case TYPE_RELEASE:
        send(0);
        release = 1;
        break;
      case TYPE_DATA_RESET:  // no ACK
        transit_reset();
        break;
      case TYPE_DATA_ADD:   // no ACK
        transit_add(&g_pkt->data, g_pkt->arg);
        break;
      case TYPE_DATA_DIGEST:   // ACK
        transit_digest(&g_pkt->data);
        send(16);		// length of digest, always 16
        break;
      case TYPE_DATA_WRITE:   // write the block to given offset
        {
          r6040_rx_disable();  // switch off packet reception for flash writing
          g_pkt->data = flash_write_chunk((u16*)transit_payload_ptr(),
                   EON_ChunkToSector(g_pkt->arg)*transit_payload_length(), 
                            transit_payload_length()) & 0xff;
          r6040_rx_enable();  // switch on packet reception after flash write
          send(1);		// only one byte returned (flash error code).
          break;
        }
      case TYPE_ERASE_SECTOR:
        ticks = 1;  // indicate erase has been skipped.  This becomes NOP
                    // in this version.
        memcpy(&g_pkt->data, &ticks, sizeof(ticks));
        send(4);		// sizeof ticks
        break;
        
      default:
        printf("Unknown packet type received\n");
        break;
    }

  }

}

