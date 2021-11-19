
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



typedef struct _eth_cons_pkt
{
  _eth_addr_t dst;
  _eth_addr_t src;
  unsigned short proto;
  unsigned short type;     // type of packet (determines direction)
  unsigned long arg;       // length of data block
  unsigned char data;	   // data block
} _eth_cons_pkt_t;



#define TYPE_ATTN            0  // console break-in packet
#define TYPE_RELEASE         2  // allow execution to continue
#define TYPE_DATA_RESET      4  // reset data receive buffer (no ack)
#define TYPE_DATA_ADD        6  // send data (no ack).
#define TYPE_DATA_DIGEST     8  // return the digest of the sent data
#define TYPE_DATA_WRITE    0xa  // write block to flash
#define TYPE_ERASE_SECTOR  0xc  // write block to flash

#define TYPE_GPIO          0xe  // switch pin high


#define TYPE_XILINX	  0x80  // xilinx redirector

#define ERROR_NONE   0x0
#define ERROR_VERIFY 0x1
#define ERROR_FLASH  0x2


static struct _eth_cons_pkt* g_pkt = (_eth_cons_pkt*)NULL;
static size_t r6040_len;


int netconsole_check_pkt()
{
  r6040_len = r6040_rx((unsigned char*)g_pkt, 1500);
  if (!r6040_len) return 0;
  if (g_pkt->proto != 0xffb1) return 0;
  
  if (config_nic_get() != CONFIG_NIC_PROMISCUOUS)
  {
    if (!ether_ismymac(&g_pkt->dst)) return 0;
  }
  return 1;
}

static void netconsole_reverse_pkt()
{
  g_pkt->dst = g_pkt->src;
  ether_getmymac(&g_pkt->src);
}





static int netconsole_status = 0;

int netconsole_ran()
{
    return netconsole_status;
}

static void netconsole_run()
{
  int release=0;
  unsigned short tmp_type;
  unsigned long ticks, reply_length;
  netconsole_status = 1;
  transit_init();
  
  while (!release)
  {
    if (!netconsole_check_pkt()) continue;

    // get ready to send response.
    netconsole_reverse_pkt();

    tmp_type = g_pkt->type;
    g_pkt->type |= 1;
    
    switch (tmp_type)
    {
      case TYPE_ATTN:
        // ACK that we've entered the console.
        g_pkt->arg = 0;
        r6040_tx((unsigned char*)g_pkt, 64);
        break;
      case TYPE_RELEASE:
        g_pkt->arg = 0;
        r6040_tx((unsigned char*)g_pkt, 64);
        release=1;
        break;
      case TYPE_DATA_RESET:  // no ACK
        transit_reset();
        break;
      case TYPE_DATA_ADD:   // no ACK
        transit_add(&g_pkt->data, g_pkt->arg);
        break;
      case TYPE_DATA_DIGEST:   // ACK
        g_pkt->arg = 16;  // length of digest, always 16
        transit_digest(&g_pkt->data);
        r6040_tx((unsigned char*)g_pkt, 64);
        break;
      case TYPE_DATA_WRITE:   // write the block to given offset
        {
          r6040_rx_disable();  // switch off packet reception for flash writing
          g_pkt->data = flash_write_chunk((u16*)transit_payload_ptr(),
                   EON_ChunkToSector(g_pkt->arg)*transit_payload_length(), 
                            transit_payload_length()) & 0xff;
          r6040_rx_enable();  // switch on packet reception after flash write
          g_pkt->arg = 1;    // only one byte returned (flash error code).
          r6040_tx((unsigned char*)g_pkt, 64);
          break;
        }
      case TYPE_ERASE_SECTOR:
        ticks = 1;  // indicate erase has been skipped.  This becomes NOP
                    // in this version.
        g_pkt->arg = 4;  // length of error returned
        memcpy(&g_pkt->data, &ticks, sizeof(ticks));
        r6040_tx((unsigned char*)g_pkt, 64);
        break;
      
      case TYPE_GPIO:
        reply_length = gpio_proxy(&g_pkt->data, g_pkt->arg);
        g_pkt->arg = reply_length;
        r6040_tx((unsigned char*)g_pkt, sizeof(struct _eth_cons_pkt)-1+reply_length);
        break;
        
      default:
        printf("Unknown packet type received\n");
        break;
    }
  }  
}

// Assume PHY already got the link up by the time we get here.
// returns true if the console ran.
int netconsole_check(void)
{
  int ret=0;
  g_pkt = (_eth_cons_pkt*)malloc(1500);
  r6040_rx_enable();	// start reception.
  
  timer_set();
 
  while (!timer_expired(TIMER_10MS))  // 10mS to comply
  {
    if (!netconsole_check_pkt()) continue;
    if (g_pkt->type == TYPE_ATTN)
    {
      ret = 1;
      printf("Started net console\n");
      netconsole_run();
      break;
    }
  }
  r6040_rx_disable();       // stop the receiver, continue with boot.
  return ret;
}



