
/* 
  Implement loading of an image in coreboot payload format
  Many features unsupported
  */

#include "bootcoreboot.h"
#include "config.h"
#include "iolib.h"
#include <string.h>
#include <stdio.h>
#include "assem.h"
#include "elfutils.h"



#define BIT0 (1<<0)
#define BIT1 (1<<1)
#define BIT2 (1<<2)
#define BIT9 (1<<9)
#define BIT16 (1<<16)



void bootcoreboot_go(void)
{ 
  u32 addr = elfutils_relocate();
  if (!addr) return;
  printf("Coreboot entry point: %x\n", addr);
  assem_core_boot((u32)addr);
}


