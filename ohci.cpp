
#include "ohci.h"
#include "iolib.h"
#include "stdlib.h"

//static _ohci_regs_t* g_reg;

void print_val(char* desc, unsigned long val)
{
//  printf(desc);  print_hex32(val);  printf("\n");
}



void ohci_init()
{
  

//  MyClass* mine = new MyClass();
  
  

/*  char* ptr;
  // Register address location set in head.S
  g_reg = (_ohci_regs_t*)0xd5900000;

  // Allocate hcca, must be on 256-byte boundary.
  ptr = (char*)dma_malloc(sizeof(_hcca_t) + 256);
  while (((unsigned long)ptr) % 256) ptr++;
  g_reg->hcca = (_hcca_t*)ptr;

  g_reg->control = 0x00000080;
  print_val("control:",g_reg->control);
  print_val("interrupt_status:",g_reg->interrupt_status);

  
  print_val("rh_descriptor_a:",g_reg->rh_descriptor_a);
  print_val("rh_descriptor_b:",g_reg->rh_descriptor_b);
  print_val("rh_status:",g_reg->rh_status);
  print_val("rh_port_status_1:",g_reg->rh_port_status_1);
  print_val("rh_port_status_2:",g_reg->rh_port_status_2);

  // print_hex32((unsigned))

  // Allocate some EDs and TDs
  */
}
