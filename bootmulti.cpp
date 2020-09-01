
// Implement the multi-boot protocol


#include "bootmulti.h"
#include "config.h"
#include "iolib.h"
#include <string.h>
#include <stdio.h>
#include "assem.h"
#include "elfutils.h"

// Comment this in to find out what's gone wrong with the multiboot
#define USE_DUMP 1


typedef struct _multiboot_hdr
{
  u32 magic;
  u32 flags;    
  u32 checksum;
  u32 header_addr;
  u32 load_addr;
  u32 load_end_addr;
  u32 bss_end_addr;
  u32 entry_addr;
} _multiboot_hdr_t;


typedef struct _multiboot_info
{
  u32 flags;
  u32 mem_lower;    // bit0
  u32 mem_upper;    // bit0 
  // boot device missing, bit1
  u32 boot_dev;
  char* cmndline;     // bit2
  u32 mods_count;
  u32 mods_addr;
  u32 syms[3];
  u32 mmap_length;
  u32 mmap_addr;
  u32 drives_length;
  u32 drives_addr;
  u32 config_table;
  u32 apm_table;
  char* bootloader_name;  // bit9
  char cmndline_data[1024];
  char bootloader_name_data[20];
} _multiboot_info_t;

static _multiboot_info_t info;

#define BIT0 (1<<0)
#define BIT1 (1<<1)
#define BIT2 (1<<2)
#define BIT9 (1<<9)
#define BIT16 (1<<16)

void init_info(_multiboot_info_t* i)
{
  i->flags = BIT0 | BIT2 | BIT9;
  i->mem_lower = 640;
  i->mem_upper = (0x2000000 - 0x100000)>>10;

  memcpy(i->cmndline_data, config_cmndline_get(), sizeof(i->cmndline_data));
  i->cmndline_data[sizeof(i->cmndline_data)-1] = 0;
  strcpy(i->bootloader_name_data, "Biffboot");  
  
  i->cmndline = i->cmndline_data;
  i->bootloader_name = i->bootloader_name_data;
}


_multiboot_hdr_t* get_header_addr(void)
{
  u32* i = (u32*)config_loadaddress_get();
  int count=0x2000;
  while (count)
  {
    if ((*i) == 0x1BADB002)
      return (_multiboot_hdr_t*)i;
    i++;
    count--;
  }
  printf("Multiboot magic (0x1BADB002) not found in first 8192 bytes of image\n");
  return 0;
}

#ifdef USE_DUMP
static void Dump(const char* name, u32 val)
{
    printf("%s: %x\n", name, unsigned(val));
}
#endif



static u32 bootmulti_relocate(_multiboot_hdr_t* hdr)
{
  u32 src;
  u32 dest;
  u32 len;
  u32 tozero;
  
//  printf("Multiboot Relocation info supplied\n");
//  Dump("header_addr", hdr->header_addr);
//  Dump("load_addr", hdr->load_addr);
//  Dump("load_end_addr", hdr->load_end_addr);
//  Dump("bss_end_addr", hdr->bss_end_addr);
//  Dump("entry_addr", hdr->entry_addr);
  
  // Where is the start of this image?
  src = (u32)hdr - (hdr->header_addr - hdr->load_addr);
  
  // Where does the image get copied to?
  dest = hdr->load_addr;
  
  // How long is the text section
  len = hdr->load_end_addr - hdr->load_addr;
  
  // Relocate it  
  memcpy((char*)dest, (char*)src, len);
  
  dest += len;
  tozero = hdr->bss_end_addr - len;
  
  memset((char*)dest, 0, tozero);
  
  return hdr->entry_addr;
}


#ifdef USE_DUMP
#define TRACE(n) printf(n);
#else
#define TRACE(n)
#endif


void bootmulti_go(void)
{
  _multiboot_hdr_t* hdr = get_header_addr();
  if (!hdr) return;
  u32 addr = config_loadaddress_get();
  init_info(&info);

#ifdef USE_DUMP  
  Dump("MB header", (u32)hdr);

  printf("Flags: %x\n", hdr->flags);
#endif
  
  if (hdr->flags & BIT0) {
    TRACE("4k aligned block - OK\n");
  }
  
  if (hdr->flags & BIT1) {
    TRACE("Memory ranges requested - OK\n");
  }
  
  if (hdr->flags & ~(BIT0 | BIT1 | BIT16)) {
    printf("Multiboot binary requested features unsupported by Biffboot, sorry\n");
    return;
  }

  if (hdr->flags & BIT16) {
    // Don't treat as elf, read the rellocation info from specific location
    TRACE("relocate using specified reloc. info\n");
    addr = bootmulti_relocate(hdr);
  }
  else
  {
    // relocates and returns the new load address.
    TRACE("ELF relocation\n");
    addr = elfutils_relocate();
    TRACE("ELF relocation complete, calling 0x");
    printf("%x", addr);
    TRACE("  \n");
  }
  
  assem_multi_boot((u32)&info, (u32)addr);
  
}

