//
// Low level ring-buffer manipulation for r6040 ethernet.
//


#include "io.h"
#include <string.h>
#include <stdio.h>
#include "setup.h"
#include "r6040.h"
#include "iolib.h"
#include "stdlib.h"
#include "timer.h"
#include "ether.h"
#include "config.h"

// definitions for 
#define IOADDR 0xe800

#define MCR0 (IOADDR + 0x00)
#define TX_START_LOW (IOADDR + 0x2c)
#define TX_START_HIGH (IOADDR + 0x30)
#define RX_START_LOW (IOADDR + 0x34)
#define RX_START_HIGH (IOADDR + 0x38)
#define INT_STATUS (IOADDR + 0x3c)


#define MMDIO           (IOADDR + 0x20)    /* MDIO control register */
#define MDIO_WRITE     0x4000  /* MDIO write */
#define MDIO_READ      0x2000  /* MDIO read */
#define MMRD            (IOADDR + 0x24)    /* MDIO read data register */
#define MMWD            (IOADDR + 0x28)    /* MDIO write data register */

#define R6040_PKT_MAX_LEN    1536






// Descriptor definition and manip functions.
// doubles as the TX and RX descriptor
struct _eth_desc {
  unsigned short DST;
  unsigned short DLEN;
  unsigned char* DBP;
  struct _eth_desc* DNX;
  unsigned short HIDX;
  unsigned short Reserved1;
  unsigned short Reserved2;
  // Extra info, used by driver
  unsigned short DLEN_orig;
};


// Diagnostic functions.

void addr_dump16(const char* name, unsigned short val)
{
  printf("%s: 0x%04x  ", name, val);
}

void addr_dump32(const char* name, unsigned long val)
{
  printf("%s: 0x%x  ", name, unsigned(val));
}


// print the descriptor
void desc_dump(struct _eth_desc* d)
{
  printf("%x\n", unsigned(d));
  addr_dump16("DST", d->DST);
  addr_dump16("DLEN", d->DLEN);
  addr_dump32("DBP", (unsigned long)d->DBP);
  addr_dump32("DNX", (unsigned long)d->DNX);
  addr_dump16("HIDX", d->HIDX); 
  printf("\n");
  // get the address of the buffer.
  hex_dump(d->DBP, 32);
  printf("\n");
}


void io_dump(const char* name, unsigned offset)
{
  unsigned short tmp;
  tmp = inw(IOADDR + offset);
  printf("%s: 0x%04x\n", name, tmp);
}


// Pass in the next pointer
struct _eth_desc* rxd_init(size_t pkt_size)
{
  // allocate the descriptor memory
  struct _eth_desc* rxd = (struct _eth_desc*)dma_malloc(pkt_size);
  
  // Clear it
  memset(rxd,0,sizeof(struct _eth_desc));
  
  // allocate some space for a packet
  unsigned char* pkt = (unsigned char*)dma_malloc(pkt_size);

  // clear pkt area
  memset(pkt,0,pkt_size);

  // Make this one owned by the MAC
  rxd->DST = 0x8000;
  
  // Set the buffer pointer
  rxd->DBP = pkt;
  rxd->DLEN = pkt_size;
  
  return rxd;
}


void r6040_tx_enable()
{
  unsigned short tmp = inw(MCR0);
  outw(tmp | (1 << 12), MCR0);  
}

void r6040_tx_disable()
{
  unsigned short tmp = inw(MCR0);
  outw(tmp & ~(1 << 12), MCR0);    
}


void r6040_rx_enable()
{
  unsigned short tmp = inw(MCR0);
  outw(tmp | (1 << 1), MCR0);  
//  outw(2, MCR0);
}

void r6040_rx_disable()
{
  outw(0, MCR0);  
}


// Dump the interesting registers
void r6040_dump()
{
  printf("\r\n== DUMP ==\n");
  io_dump("MAC0", 0x00);
  io_dump("MAC1", 0x04);
  io_dump("MAC Bus control", 0x08);
  io_dump("TX start0", 0x2c);
  io_dump("TX start1", 0x30);
  io_dump("RX start0", 0x34);
  io_dump("RX start1", 0x38);
  io_dump("INT Stat", 0x3c);
}


void r6040_set_tx_start(struct _eth_desc* desc)
{
  unsigned long tmp = (unsigned long)desc;
  outw((tmp & 0xffff), TX_START_LOW);
  tmp >>= 16;
  outw((tmp & 0xffff), TX_START_HIGH);
}

void r6040_set_rx_start(struct _eth_desc* desc)
{
  unsigned long tmp = (unsigned long)desc;
  outw((tmp & 0xffff), RX_START_LOW);
  tmp >>= 16;
  outw((tmp & 0xffff), RX_START_HIGH);
}



////////////////////////////////////////////////////////////////
//  Interface

// Keep track of what we've allocated, so we can easily walk it.
struct _eth_desc* g_rx_descriptor_list[R6040_RX_DESCRIPTORS];
struct _eth_desc* g_rx_descriptor_next;

struct _eth_desc* g_tx_descriptor_next;


// setup descriptors, start packet reception
void r6040_init(void)
{
  int i;
  r6040_rx_disable();
  r6040_tx_disable();
  for (i=0;i<R6040_RX_DESCRIPTORS;i++)
  {
    g_rx_descriptor_list[i] = rxd_init(R6040_PKT_MAX_LEN);   //  most packets will be no larger than this
    if (i) g_rx_descriptor_list[i-1]->DNX = g_rx_descriptor_list[i];
  }
  // Make ring buffer.
  g_rx_descriptor_list[R6040_RX_DESCRIPTORS-1]->DNX = g_rx_descriptor_list[0];
  r6040_set_rx_start(g_rx_descriptor_list[0]);
  g_rx_descriptor_next = g_rx_descriptor_list[0];

  for (i=0;i<R6040_RX_DESCRIPTORS;i++)
  {
//    desc_dump(g_rx_descriptor_list[i]);
  }
//  printf("r6040 init\n");  
  g_tx_descriptor_next = rxd_init(R6040_PKT_MAX_LEN);
}




void DiscardDescriptor()
{
  // reset the length field to original value.
  g_rx_descriptor_next->DLEN = g_rx_descriptor_next->DLEN_orig;
  g_rx_descriptor_next->DST = 0x8000;  // give back to the MAC.
  g_rx_descriptor_next = g_rx_descriptor_next->DNX;  // Move along
}


// Returns size of pkt, or zero if none received.
size_t r6040_rx(unsigned char* pkt, size_t max_len)
{
  size_t ret=0;
  if (g_rx_descriptor_next->DST & 0x8000)
  {
    // Still owned by the MAC, nothing received
    return ret;
  }
  
  if (!(g_rx_descriptor_next->DST & (1 << 14)))
  {
    printf("Descriptor discarded with error: %04x\n", g_rx_descriptor_next->DST);
    DiscardDescriptor();
    return ret;
  }  

  // If the buffer isn't long enough discard this packet.
  if (g_rx_descriptor_next->DLEN > max_len)
  {
    printf("Descriptor descarded (buffer too short)\n"
	      "Packet length: %04x\n", g_rx_descriptor_next->DLEN );
    printf("Buffer length: %x\n", unsigned(max_len));

    DiscardDescriptor();
    return ret;
  }
  
  // Otherwise copy out and advance buffer pointer
  //printf("Received packet\n");
  memcpy(pkt, g_rx_descriptor_next->DBP, g_rx_descriptor_next->DLEN);
  ret = g_rx_descriptor_next->DLEN;
  ret -= 4;   // chop the checksum, we don't need it.
  DiscardDescriptor();
  return ret;  
}


// queue packet for transmission
void r6040_tx(unsigned char* pkt, size_t length)
{
  r6040_tx_disable();
  
  if (length>R6040_PKT_MAX_LEN)
  {
    printf("TX packet exceeds maximum length, skipping\n");
    return;
  }

  // Zero the packet if it's a runt
  if (length<60) memset(g_tx_descriptor_next->DBP, 0, 60);

  memcpy(g_tx_descriptor_next->DBP, pkt, length);
  g_tx_descriptor_next->DLEN = (length<60)? 60:length;
  
  // Copy the descriptor address (will have been set to zero by last op).
  r6040_set_tx_start(g_tx_descriptor_next);
  
  // Make the mac own it.
  g_tx_descriptor_next->DST = 0x8000;


  //desc_dump(g_tx_descriptor_next);
  
  // Start xmit.
  r6040_tx_enable();

  //printf("Waiting to get ownership\n");  
  // poll for mac to no longer own it.
  while (g_tx_descriptor_next->DST & 0x8000)
  {
  }
  // Stop any other activity.
  r6040_tx_disable();
  
  //desc_dump(g_tx_descriptor_next);  
}


unsigned short r6040_mdio_read(int reg, int phy)
{            
  outw(MDIO_READ + reg + (phy << 8), MMDIO);
  /* Wait for the read bit to be cleared */
  while (inw(MMDIO) & MDIO_READ);
  return inw(MMRD);
}


void r6040_phy_printf()
{
  unsigned short tmp;
  tmp = r6040_mdio_read(0,1);
  printf("MII Control: %04x\n", unsigned(tmp));
  tmp = r6040_mdio_read(1,1);
  printf("MII Status: %04x\n", unsigned(tmp));
  tmp = r6040_mdio_read(4,1);
  printf("PHY4: %04x\n", unsigned(tmp));
  tmp = r6040_mdio_read(5,1);
  printf("PHY5: %04x\n", unsigned(tmp));
  tmp = r6040_mdio_read(6,1);
  printf("PHY6: %04x\n", unsigned(tmp));

  tmp = r6040_mdio_read(16,1);
  printf("PHY16: %04x\n", unsigned(tmp));
  tmp = r6040_mdio_read(17,1);
  printf("PHY17: %04x\n", unsigned(tmp));
}


// Wait for linkup, or timeout, return the time taken for the link to come up.
int r6040_wait_linkup(int delay, int* uptime)
{
  u16 established = 0;
  u16 i;
  // read the vendor information
  //tmp = r6040_mdio_read(2,1);
  //print_hex16(tmp); printf("\n");
  //tmp = r6040_mdio_read(3,1);
  //print_hex16(tmp); printf("\n");
  *uptime = 0;
  
  for (i=0;i<delay;i++)
  {
    // Check if link up.
    established = r6040_mdio_read(1,1) & (1<<2);
    if (established) break;

    // If user pressed a key skip this wait.
    if (keyready()) return 0;

    timer_mdelay10();
    (*uptime)++;   // how long we waited
  }
  if (established)
  {
    printf("NIC up in: %d0ms\n", i);
  }
  else
  {
    printf("No network link established - cable disconnected?\n");
  }
  return established;
}





#define REG_MAC1 0xe868
#define REG_MAC2 0xe86a
#define REG_MAC3 0xe86c


static void r6040_set_common_mac()
{
  // mac addresses for the MAC E800
  // MAC 1 multicast addresses
  outw(0xffff, 0xe870);
  outw(0xffff, 0xe872);
  outw(0xffff, 0xe874);

  outw(0xffff, 0xe878);
  outw(0xffff, 0xe87a);
  outw(0xffff, 0xe87c);

  outw(0xffff, 0xe880);
  outw(0xffff, 0xe882);
  outw(0xffff, 0xe884);

  outw(0x9f07, 0xe888);   // MAC status change register
}


void r6040_print_common_mac()
{
  // Now read them back, to check they got set OK.
  printf("%02x", unsigned(inw(REG_MAC1) & 0xff));
  printf("%02x", unsigned((inw(REG_MAC1) >> 8) & 0xff));
  printf("%02x", unsigned(inw(REG_MAC2) & 0xff));
  printf("%02x", unsigned((inw(REG_MAC2) >> 8) & 0xff));
  printf("%02x", unsigned(inw(REG_MAC3) & 0xff));
  printf("%02x", unsigned((inw(REG_MAC3) >> 8) & 0xff));
}


int r6040_get_mac_suffix(char* buffer, u32 len)
{
  // copy unique mac into buffer
  char tmp[3];
  get_hex8(tmp, sizeof(tmp), unsigned((inw(REG_MAC2) >> 8) & 0xff));
  strcat(buffer, tmp);
  get_hex8(tmp, sizeof(tmp), unsigned(inw(REG_MAC3) & 0xff));
  strcat(buffer, tmp);
  get_hex8(tmp, sizeof(tmp), unsigned((inw(REG_MAC3) >> 8) & 0xff)); 
  strcat(buffer, tmp);
  return 6;
}


// Just setup the addresses, don't start packet reception.
void r6040_set_mac()
{
  r6040_set_common_mac();

  // MAC address for eth0
  if (ether_mac_unset())
  {
    // We always want the MAC to be something sensible, otherwise the board
    // won't be ethernet programmable.
    outw(0x0100, REG_MAC1);
    outw(0x0302, REG_MAC2);
    outw(0x0504, REG_MAC3);  
  }
  else
  {
    outw(ETHER_MAC->addr0, REG_MAC1);
    outw(ETHER_MAC->addr1, REG_MAC2);
    outw(ETHER_MAC->addr2, REG_MAC3);
  }
  // MAC address for eth1, never used.
  outw(0x0000, 0xe968);
  outw(0x0100, 0xe96a);
  outw(0x0300, 0xe96c);

}



