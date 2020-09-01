
#include "setup.h"
#include "ether.h"
#include "flash.h"
#include "iolib.h"
#include "config.h"
#include <string.h>
#include <stdio.h>

void ether_getmymac(_eth_addr_t* addr)
{
  addr->addr0 = ETHER_MAC->addr0;
  addr->addr1 = ETHER_MAC->addr1;
  addr->addr2 = ETHER_MAC->addr2;
}


void ether_getzero(_eth_addr_t* addr)
{
  addr->addr0 = 0;
  addr->addr1 = 0;
  addr->addr2 = 0;
}


int ether_ismymac(_eth_addr_t* addr)
{
  return ((addr->addr0 == ETHER_MAC->addr0) &&
          (addr->addr1 == ETHER_MAC->addr1) &&
          (addr->addr2 == ETHER_MAC->addr2)) ? 1 : 0;
}


int ether_isbcast(_eth_addr_t* addr)
{
  return ((addr->addr0 == 0xffff) &&
          (addr->addr1 == 0xffff) &&
          (addr->addr2 == 0xffff)) ? 1 : 0;
}


void ether_getbcast(_eth_addr_t* addr)
{
  addr->addr0 = 0xffff;
  addr->addr1 = 0xffff;
  addr->addr2 = 0xffff;
}


void ether_prep_broad(_eth_pkt_t* hdr)
{
  ether_getmymac(&hdr->src);
  ether_getbcast(&hdr->dst);
  hdr->proto = htons(0x0800);
}


u16 htons(u16 val)
{
  u16 tmp = (val & 0xff);
  val >>= 8;
  tmp <<= 8;
  val |= tmp;
  return val;
}


u32 htonl(u32 val)
{
  u32 out = (val & 0xff) << 24;
  out |= (val & 0xff00) << 8;
  out |= (val & 0xff0000) >> 8;
  out |= (val & 0xff000000) >> 24;
  return out;
}


// True if equal
int ether_isequal(_eth_addr_t* a1, _eth_addr_t* a2)
{
  return ((a1->addr0 == a2->addr0) &&
          (a1->addr1 == a2->addr1) &&
          (a1->addr2 == a2->addr2)) ? 1 : 0;
}


void ether_printf(_eth_addr_t* addr)
{
  printf("%02x:%02x:%02x:%02x:%02x:%02x",
    unsigned(addr->addr0 & 0xff),
    unsigned((addr->addr0>>8) & 0xff),
    unsigned(addr->addr1 & 0xff),
    unsigned((addr->addr1>>8) & 0xff),
    unsigned(addr->addr2 & 0xff),
    unsigned((addr->addr2>>8) & 0xff)
  );
}


// Return true if the MAC is ff:ff:ff:ff:ff:ff
int ether_mac_unset()
{
  if ((ETHER_MAC->addr0 == 0xffff) &&
      (ETHER_MAC->addr1 == 0xffff) && 
      (ETHER_MAC->addr2 == 0xffff)    ) return 1;
      
  return 0;
}


static const char* complete_mac(const char* partial)
{
  return "00b3f7";
}


static bool input_hex_mac(char* hexbuffer)
{
  int out;
  printf("Enter MAC value (e.g. 000102030405), <ENTER> when done, <ESC> to abort\n");
  out = get_ascii_data(hexbuffer, 12 + 1, 0, 1, 0, complete_mac);
  if (out == 27) {
    printf("\r\nAborted!");
    return false;
  }
  printf("\n");
  return true;
}


void ether_entermac(const char* args)
{
  if (!ether_mac_unset())
  {
    printf("MAC has already been set\n");
    return;
  }
  
  char string_buff[13];
  u8 hex_buff[6];
  
  if (!input_hex_mac(string_buff))
  {
    printf("\n");
    return;
  }
  
  if (strlen(string_buff) < 12)
  {
    printf("Invalid MAC: too short\n");
    return;
  }
  
  hex_buff[0] = hex_to_u32(&string_buff[0], 2) & 0xff;
  hex_buff[1] = hex_to_u32(&string_buff[2], 2) & 0xff;
  hex_buff[2] = hex_to_u32(&string_buff[4], 2) & 0xff;
  hex_buff[3] = hex_to_u32(&string_buff[6], 2) & 0xff;
  hex_buff[4] = hex_to_u32(&string_buff[8], 2) & 0xff;
  hex_buff[5] = hex_to_u32(&string_buff[10], 2) & 0xff;
  
  printf("About to permanently set MAC to %02x:%02x:%02x:%02x:%02x:%02x", hex_buff[0],
    hex_buff[1], hex_buff[2], hex_buff[3], hex_buff[4], hex_buff[5]);
  if (!are_you_sure())
  {
    return;
  }
  
  // program the MAC
  printf("Programming... ");
  flash_setmac(hex_buff);
  printf("done\n");
}

