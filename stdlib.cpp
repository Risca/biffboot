
#include "setup.h"
#include "stdlib.h"
#include "iolib.h"
#include <string.h>


// Starts at 0x50000 and goes up.
// Keep below 1M, but way above bootloader extent.
static unsigned long g_dma_heap_ptr = BIOS_DMA_HEAP_MIN;

void* dma_malloc(size_t count)
{
  void* tmp;
  
  // Round up to nearest 16-bytes
  if (count & 0xf)
  {
    count += 0x10;
    count &= 0xfffffff0;
  }
  
  tmp = (void*)g_dma_heap_ptr;
  memset((void*)g_dma_heap_ptr, 0, count);

  g_dma_heap_ptr += count;
  if (g_dma_heap_ptr > BIOS_DMA_HEAP_MAX)
  {
    fatal("DMA_MALLOC: fatal: Allocated over DMA_HEAP_MAX");
  }
  //print_hex32(g_dma_heap_ptr);
  //printf("\n");
  return tmp;
}


void dma_free(void* ptr)
{
  // noop.
}

// Zero any allocated memory
void dma_free_all()
{
  memset((char*)BIOS_DMA_HEAP_MIN, 0, BIOS_DMA_HEAP_MAX - BIOS_DMA_HEAP_MIN);
}


// Start it well out of the way, up at about 14MB.
// leaving 0x400000 + 0x800000 i.e. 8MB can be loaded into flash 
// for really large kernels.
static unsigned long g_mem_heap_ptr = BIOS_HEAP_ADDR;

void* malloc(size_t count)
{
  void* tmp;
  
  // Round up to nearest 16-bytes
  if (count & 0xf)
  {
    count += 0x10;
    count &= 0xfffffff0;
  }
  
//  print_hex32(count);
//  printf("\n");
  tmp = (void*)g_mem_heap_ptr;
  g_mem_heap_ptr += count;
  if (g_mem_heap_ptr > 0x2000000)
  {
    g_mem_heap_ptr -= count;
    return NULL;
  }
    
  // zero the memory before returning it.
  memset((void*)g_mem_heap_ptr, 0, count);
  return tmp;
}


void free(void* ptr)
{
  // noop.
}


void abort()
{
  // nop, make malloc compile
  while (1) {}
}

