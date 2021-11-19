

#include "elf.h"
#include "elfutils.h"
#include "iolib.h"
#include <string.h>
#include <stdio.h>
#include "config.h"


// Deal with the case that the filesz part is 0xffffffff
// Some binaries have this setting, e.g. OFW.
u32 to_copy(u32 voffs, u32 length)
{
  u32 max = (0x2000000 - voffs);
  if (length > max)
    length = max;
  return length;
}


u32 VirtToPhys(Elf32_Phdr* p, Elf32_Addr addr)
{
  // Most likely scenario
  if (p->p_vaddr > p->p_paddr)
  {
    addr -= (p->p_vaddr - p->p_paddr);
  } 
  else
  {
    addr += (p->p_paddr - p->p_vaddr);
  }
  return addr;
}


void DumpProgramHeader(Elf32_Phdr* p)
{
  printf("=== New header ===\n");
  printf("e_phoff: %x\n",(u32)p);
  printf("p_type: "); 
  switch (p->p_type)
  {
    case PT_LOAD:
      printf("PT_LOAD");
      break;
    case PT_GNU_STACK:
      printf("PT_GNU_STACK");
      break;
    default:
      printf("%x", p->p_type);
  }
  printf("\n");
  printf("p_offset: %x\n",p->p_offset); 
  printf("p_vaddr: %x\n",p->p_vaddr);  
  printf("p_paddr: %x\n",p->p_paddr);  
  printf("p_filesz: %x\n",p->p_filesz);
}


u32 LoadProgramHeader(u32 h, Elf32_Phdr* p)
{
  u32 len;
  u32 dest;
  u32 src;
//  DumpProgramHeader(p);
  switch (p->p_type)
  {
    case PT_LOAD:
      len = to_copy(p->p_vaddr, p->p_filesz);
      dest = (u32)p->p_paddr;      
      src = (u32)h + p->p_offset;
      memcpy((char*)dest, (char*)src, len);
//      printf("Copy, from: "); print_hex32(src);
//      printf(" to: "); print_hex32(dest);
//      printf(" count: "); print_hex32(len);
//      printf("\n");
      // initialise last part to zero.
      dest += len;
      memset((char*)dest, 0, p->p_memsz - p->p_filesz);
//      printf("Zero, from: "); print_hex32(dest);
//      printf(" count: "); print_hex32(p->p_memsz - p->p_filesz);
//      printf("\n");
      break;
  }
  return p->p_type;
}


// Move the sections of the elf header to appropriate load locations,
// return the entry point
u32 elfutils_relocate(void)
{
  Elf32_Ehdr* h = (Elf32_Ehdr*)config_loadaddress_get();
  u32 next_p;
  u32 ptype;
  u32 has_stack = 0;
  u32 i;
  u32 entry_point;

  if (memcmp(h->e_ident, ELFMAG, 4)!=0)
  {
    printf("ELF header magic number mismatch\n");
    return 0;
  }

  next_p = (u32)((char*)h + h->e_phoff);
  
  // Work out any physical->virtual adjustments from the difference between p_vaddr and p_paddr, assume virtual 
  // is always higher than physical if they differ.
  entry_point = VirtToPhys((Elf32_Phdr*)next_p, h->e_entry);

  for (i=0; i < h->e_phnum; i++) {
    ptype = LoadProgramHeader((u32)h, (Elf32_Phdr*)next_p);
    if (ptype==PT_GNU_STACK)
    {
      printf("PT_GNU_STACK header will be ignored.\n");
      has_stack = 1;
    }
    next_p += h->e_phentsize;
  }
  
  // Figure out where to put the stack
  
  return entry_point;
}

