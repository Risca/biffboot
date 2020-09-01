#include "iolib.h"
#include "flashmap.h"
#include "flash.h"
#include <stdio.h>



u32 FLASHMAP_DEVICE_SIZE = 0;
u32 FLASHMAP_BASE = 0;
u32 FLASHMAP_SIZE = 0;
u32 FLASHMAP_KERNEL_SIZE = 0;
u32 FLASHMAP_CHUNK_COUNT = 0;


static u32 g_detected = 0;

u32 flashmap_get_detected()
{
  return g_detected;
}


void flashmap_init()
{
  u32 val;
  // Need to set the base address for detection to work.
  g_detected = flash_detect(0xff800000);
  if (!g_detected) g_detected = flash_detect(0xffc00000);
  if (!g_detected) g_detected = flash_detect(0xfff00000);
  
  switch (g_detected)
  {
    case FLASH_CODE_EN800:  // 1MB
      FLASHMAP_DEVICE_SIZE = 1;
      FLASHMAP_BASE = 0xfff00000; // start address.
      FLASHMAP_SIZE = 0x100000;   // size in bytes
      FLASHMAP_KERNEL_SIZE = 0xf0000;  // size of kernel to copy
      break;
    case FLASH_CODE_EN320:  // 4MB
      FLASHMAP_DEVICE_SIZE = 4;
      FLASHMAP_BASE = 0xffc00000;
      FLASHMAP_SIZE = 0x400000;
      FLASHMAP_KERNEL_SIZE = 0x100000;
      break;
    case FLASH_CODE_EN640:  // 8MB
      FLASHMAP_DEVICE_SIZE = 8;
      FLASHMAP_BASE = 0xff800000;
      FLASHMAP_SIZE = 0x800000;
      FLASHMAP_KERNEL_SIZE = 0x100000;
      break;
    default:
      printf("flash_detect() returned %x\n", val);
      fatal("Unrecognised flash\n");
  }
  
  FLASHMAP_CHUNK_COUNT = ((FLASHMAP_SIZE-FLASHMAP_BOOT_SIZE)/FLASHMAP_CHUNK_SIZE);
  
  // flash_print_info();
}


// Return true if we're at the start of a sector.
int flashmap_isboundary(unsigned long addr)
{
  if (addr & 0x1fff) return 0;   // optimise: no sector smaller than 8k
  if (addr >= FLASHMAP_SIZE) return 0;   // not one of ours.
  if (!addr) return 1;   // start of flash, always a sector

  if (addr >= 0x10000)   // bottom-boot: high sectors all 64k
  {
    // only 64k sectors allowed
    if (addr & 0xffff) return 0;  // not 64k boundary
    return 1;
  }

  // 1M devices have only 4 sectors.
  if (FLASHMAP_DEVICE_SIZE == 1)
  {
    switch (addr)
    {  // 16k:8k:8k:32k only
      case 0x4000:  case 0x6000:  case 0x8000:
        return 1;
    }
    return 0;
  }
  
  // 4,8M devices have only 8k bottom-boot blocks
  return 1;
}


// Return true if this flash sector is 0xff.
int flashmap_iserased(unsigned long addr)
{
  // check the first:
  unsigned long val;
  unsigned long i;
  
  if (!flashmap_isboundary(addr))
    return 0;
  // do the first 
  while (addr<FLASHMAP_SIZE)  // don't go off the top!
  {
    // Check this chunk
    i = FLASHMAP_CHUNK_SIZE;
    while (i)
    {
      i-=4;
      val = ReadFLASH32_FAST(addr);
      if (val != 0xffffffff) return 0;
      addr += 4;
    }
    // We're done with this sector
    if (flashmap_isboundary(addr)) return 1;
  }
  
  // reached end of flash
  return 1;
}

