

#include <string.h>
#include <stdio.h>
#include "stdlib.h"
#include "udp.h"
#include "iolib.h"
#include "r6040.h"
#include "ether.h"
#include "timer.h"
#include "bootp.h"
#include "arp.h"


#define BOOTP_NBOOTPS	67
#define BOOTP_NBOOTPC	68

#define BOOTP_SZCHADDR	16    		//* size of client haddr field	
#define BOOTP_SZSNAME	64    		//* size of server name field
#define BOOTP_SZVENDOR	64    		//* size of vendor spec. field	

#define BOOTP_REQUEST	1
#define BOOTP_REPLY	2

#define BOOTP_ETHERNET	1
#define BOOTP_ELEN	6

#define DHCP_RX_MAX    576


/* structure of a BOOTP message */
typedef struct bootp_msg {
    char op;				// request or reply 		
    char htype;				// hardware type		
    char hlen;				// hardware address length	
    char hops;				// set to zero
    long xid;				// transaction id		
    unsigned short secs;		// time client has been trying	
    short unused;			// unused			
    _ip_addr_t ciaddr;			// client IP address		
    _ip_addr_t yiaddr;			// your (client) IP address	
    _ip_addr_t siaddr;			// server IP address		
    _ip_addr_t giaddr;			// gateway IP address		
    _eth_addr_t chaddr;			// client hardware address	
    char chaddr_pad[10];		// not used.
    char sname[BOOTP_SZSNAME];		// server host name
    char file[BOOTP_SZFNAME];		// boot file name
    u8 vend[BOOTP_SZVENDOR];		// options
} ETHER_PACK bootp_msg_t;




typedef struct _pkt_buffer 
{
  _udp_pkt_t udp;
  bootp_msg_t msg;
} ETHER_PACK _pkt_buffer_t;


// Whether or not use use the 'DHCP' version of the protocol, otherwise
// just plain old bootp.
static int g_DHCP = 0;


static u8* g_dhcp_option_ptr_start;
static u8* g_dhcp_option_ptr;

static inline void dhcp_option_start(u8* ptr)
{
  g_dhcp_option_ptr_start = ptr;
  *ptr = 0x63;  ptr++;
  *ptr = 0x82;  ptr++;
  *ptr = 0x53;  ptr++;
  *ptr = 0x63;  ptr++;
  g_dhcp_option_ptr = ptr;
}

static void dhcp_option_set(u8 option, u8 len, u8* val)
{
  *g_dhcp_option_ptr = option;
  g_dhcp_option_ptr++;
  *g_dhcp_option_ptr = len;
  g_dhcp_option_ptr++;
  memcpy(g_dhcp_option_ptr, val, len);
  g_dhcp_option_ptr += len;
}

static inline void dhcp_option_end()
{
  int pad;
  *g_dhcp_option_ptr = 0xff;   // terminator, end of options
  g_dhcp_option_ptr++;
  
  pad = (BOOTP_SZVENDOR - (g_dhcp_option_ptr - g_dhcp_option_ptr_start));
  if (pad<0)
  {
    // only for debugging, remove for release.
    printf("Warning: vendor option space exceeded\n");
  } 
  else
  {
    memset(g_dhcp_option_ptr, 0, pad);
  }
}


typedef struct client_id {
  u8	magic;
  char  IAID[4];
  u16	DUID;
  u16	hwType;
  u32	time;
  _eth_addr_t llAddr;  
} ETHER_PACK client_id_t;

static void bootp_prep_client_id(client_id_t* id)
{
    // client identifier
    id->magic = 0xff;
    id->IAID[0]='e'; id->IAID[1]='t'; id->IAID[2]='h'; id->IAID[3]='0';
    id->DUID = htons(1);
    id->hwType = htons(1);
    id->time = htonl(12345);
    ether_getmymac(&id->llAddr);
}


static void bootp_prep_request(bootp_msg_t* m, const char* hostname)
{
  m->op = BOOTP_REQUEST;     // request
  m->htype = 1;  // hw is 10mb ethernet
  m->hlen = 6;   // hw addr length
  m->hops = 0;   // always zero
  m->xid = 0xb1ff;   // transaction ID (random)
  m->secs = htons(3);  // secs since I booted.
  m->unused = 0;       // also used as flags

  ip_get_zero(&m->ciaddr);  // not known, so zero
  ip_get_zero(&m->yiaddr);  // not known, so zero
  ip_get_zero(&m->siaddr);  // not known, so zero
  ip_get_zero(&m->giaddr);  // not known, so zero
  
  ether_getmymac(&m->chaddr);  // my mac address

  memset(m->chaddr_pad, 0, 10);		// pad client hardware address
  memset(m->sname, 0, BOOTP_SZSNAME);	// server host name
  memset(m->file, 0, BOOTP_SZFNAME);	// boot file name
  
  if (g_DHCP)
  {
    //const char* params = "\x01\x03\x06";
    const char* vendor_class = "dhcpcd 3.2.3";
    client_id_t	id;
    u8 tmp_b;
  
    // fill in 64-byte vendor extensions
    dhcp_option_start(m->vend);  // sets cookie
    
    tmp_b = 1;
    dhcp_option_set(53, 1, &tmp_b);   // Discover
    
    // Maximum message size
    //tmp_s = htons(sizeof(_pkt_buffer_t));  // should be 576
    //dhcp_option_set(57, 2, &tmp_s);  // max response size
    
    //tmp_l = htonl(0x100);
    //dhcp_option_set(51, 4, &tmp_l);  // 5 minute lease

    // Client ID
    bootp_prep_client_id(&id);
    dhcp_option_set(61, sizeof(id), (u8*)&id);  // Slackware 13 DHCP gives this.
    
    // Vendor class ID.  We'll pretend we're linux dhcpcd.
    dhcp_option_set(60, strlen(vendor_class), (u8*)vendor_class);
    
    // Go for 'biffer'
    dhcp_option_set(12, strlen(hostname), (u8*)hostname);
    
    // Parameter request list, ensure to request DNS server, so the
    // supplied host gets acted on.
    tmp_b = 6;
    dhcp_option_set(55, 1, &tmp_b);
   
    dhcp_option_end();   // check range and terminate
  }
  else
  {
    memset(m->vend, 0, BOOTP_SZFNAME);	// no vendor extensions
  }
  
}


static void bootp_req_setup(_pkt_buffer_t* pkt, const char* hostname)
{
  memset(pkt, 0, sizeof(_pkt_buffer_t));
  
  // Fill in the rest, from above
  bootp_prep_request(&pkt->msg, hostname);
 
  // Fill in udp details passing ports and payload size
  udp_prep_broad(&pkt->udp, 67, 68, sizeof(bootp_msg_t));
}


// Copy ip address and name into supplied buffer.
// wait forever to get an address.
unsigned short bootp_getaddr(_ip_addr_t* saddr, const char* hostname, char* fname)
{
  unsigned count=0;
  size_t rx_len;
  _pkt_buffer_t* tx_pkt=(_pkt_buffer_t*)NULL;
  _pkt_buffer_t* rx_pkt=tx_pkt;

  g_DHCP = hostname ? 1 : 0;

  ip_init();
  arp_init();  // we'll need ARP later

  // Allocate our sent pkt.
  tx_pkt = (_pkt_buffer_t*)malloc(sizeof(_pkt_buffer_t));
  
  bootp_req_setup(tx_pkt, hostname);

  // Allocate for our rx pkt, the maximum DHCP pkt size.
  rx_pkt = (_pkt_buffer_t*)malloc(DHCP_RX_MAX);
  memset(rx_pkt, 0, sizeof(_pkt_buffer_t));

  r6040_rx_enable();	// start reception of packets

  // First request.
  r6040_tx((unsigned char*)tx_pkt, sizeof(_pkt_buffer_t));
  // Write something to indicate we've sent a packet.
  printf(".");
 
  while (1)
  {  
    // check the rx buffer
    rx_len = r6040_rx((unsigned char*)rx_pkt, sizeof(_pkt_buffer_t)+100);
    if (rx_len)
    {
      // does the magic number match?
      if (rx_pkt->msg.xid != 0xb1ff) continue;
      // does the hw addr match?
      if (!ether_isequal(&rx_pkt->msg.chaddr, &tx_pkt->udp.ip.eth.src)) continue;
      // is it a response?
      if (rx_pkt->msg.op != BOOTP_REPLY) continue;

      // assign the server's address
      *saddr = rx_pkt->msg.siaddr;
            
      if ((!hostname) && fname)
      {
        // Copy the filename
        strncpy(fname, rx_pkt->msg.file, BOOTP_SZFNAME);
      } 
      
      printf("\n");

      // Look at the server address, if it matches the IP header source
      // address the server is saying he is also the tftp server so we
      // can add the ARP entry now
      if (ip_isequal(saddr,&rx_pkt->udp.ip.saddr))
      {
        arp_add(saddr, &rx_pkt->udp.ip.eth.src);
      }
      
      // set our address, for future ip transactions
      ip_set(&rx_pkt->msg.yiaddr);

      break;
    }
    
    timer_mdelay10();   // wait 10mS for more packets.

    if (!(count++ % 100))  // send new packet every second approx
    {
      r6040_tx((unsigned char*)tx_pkt, sizeof(_pkt_buffer_t));
      printf(".");
    }
    
  }
  
  if (!hostname)
    r6040_rx_disable();       // stop the receiver, continue with boot
  
  return timer_read();	// return something random to use as tftp cookie
}

