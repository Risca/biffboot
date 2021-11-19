
#include "idt.h"
#include "gdt.h"
#include "timer.h"
#include "bootp.h"
#include "arp.h"
#include "r6040.h"
#include "icmp.h"

static unsigned char* g_pkt;   // general rx packet

void telnet_run()
{
  _ip_addr_t myaddr;

  gdt_init();  // needed for idt
  idt_init();  // needed for timer interrupt
    
  timer_init();   // start the timer tick

  // dhcp discover our IP address.
  bootp_getaddr(&myaddr, 0, NULL);  // no name needed, just the IP

  g_pkt = (unsigned char*)malloc(600);
  
  print("Started telnet server\r\n");

  while (1)  // serve forever
  {
    int rx_len;
    rx_len = r6040_rx(g_pkt, 600);  // big enough for data packets
    if (rx_len)
    {
      // check if it's ARP and process if needed
      if (arp_check(g_pkt)) continue;

      if (icmp_check(g_pkt)) continue;

      // Make sure it's really UDP protocol bound for this address/port
      //if (!udp_isforme(g_pkt)) continue;
      
      // It's for us, act on it.
    } 
  }    

}
