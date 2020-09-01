// Load kernel data from boot source.


#include "config.h"
#include "stdlib.h"
#include <string.h>
#include <stdio.h>
#include "iolib.h"
#include "mmc.h"
#include "loader.h"
#include "flash.h"
#include "flashmap.h"
#include "bootp.h"
#include "tftp.h"
#include "ohci.h"


static int copy_from_usb(u32 dest)
{
  ohci_init();
  printf("Copied from USB\n");
  while(1) {};
  return 0;
}



static char bootpath[BOOTP_SZFNAME];

// For simplicity we will only allow a kernel+initrd to be copied
// no separate ram disk
u32 loader_copy_from_network(u32 dest)
{
  u16 random;
  u32 length;
  _ip_addr_t saddr;
  printf("Obtaining IP address");
  // no hostname == want a boot file.
  random = bootp_getaddr(&saddr, NULL_STRING, bootpath);
  tftp_setrandom(random);
  printf("Server IP: ");  ip_print(&saddr);  printf("\n");
  printf("Boot file: ");  printf(bootpath);  printf("\n");

  length = 0;  
  // Get tftp file to the destination
  while (!length)
  {
    length = tftp_getfile((char*)dest, &saddr, bootpath);
  }
  printf("%d byte image received from ", length);  ip_print(&saddr);  printf("\n");
  return length;
}



static u32 copy_from_flash(u32 dest)
{
  u32 chunk, chunkmax;
  u32 sector;
  // Copy the first 120/256 sectors - this also copies the bios config, if there 
  // is any onto the end, but it's quick so we don't care.
  
  // calc the max number of chunks to copy.
  chunkmax = (config_kernelmax_get()/FLASHMAP_CHUNK_SIZE);
  
  // chunks to copy.
  for (chunk = 0; chunk < chunkmax; chunk++)
  {
    sector = EON_ChunkToSector(chunk) * FLASHMAP_CHUNK_SIZE;
    //printf("Dest: "); print_hex32(dest); 
    //printf(" Src: "); print_hex32((FLASHMAP_BASE+sector)); 
    //printf("\n");
    memcpy((char*)dest, (char*)(FLASHMAP_BASE+sector), FLASHMAP_CHUNK_SIZE);
    dest += FLASHMAP_CHUNK_SIZE;
  }
  printf("%x loaded from flash.\n", config_kernelmax_get());
  return config_kernelmax_get();
}


// Null start_sector indicates end of list
// Null count indicates jump to next sector.
struct _kernel_map 
{
  u32 start;
  u32 count;
};


static u32 copy_from_mmc(u32 dest)
{
  u32 i;
  u8* map;
  u8* buffer;
  struct _kernel_map* map_entry;
  if (mmc_init())
  {
    printf("SD/MMC card not recognised\n");
    return 0;
  }
  
  map = (u8*)malloc(512);
  buffer = (u8*)malloc(512);
  // Load MBR
  mmc_read(map,0,1);
  // First pointer is 
  map_entry = (struct _kernel_map*)(map + 0x100);
  // examine structure at 0x100 offset, this is the map key sector
  
  // 8M limit on compressed kernel+ramdisk size.
  while (map_entry->start && (dest <0xc00000))
  {
    printf("Sector: %x\n", map_entry->start);
    printf(" Count: %x\n", map_entry->count);
    // copy the number of blocks to the destination
    for (i=0;i<map_entry->count;i++)
    {
      mmc_read(buffer,map_entry->start+i, 1);
      memcpy((char*)dest, buffer, 512);
      dest += 512;
    }
    map_entry++;
    if (!map_entry->start) break;  // no more blocks
    if (!map_entry->count)    // this is a link.
    {
      mmc_read(map,map_entry->start,1);
      map_entry = (struct _kernel_map*)map;
    }
  }
  printf("Loaded from SD/MMC.\n");
  return (dest - config_loadaddress_get());
}


u32 loader_load()
{
  u32 ret=0;
  u32 dest = config_loadaddress_get();
  switch (config_bootsource_get())
  {
    case CONFIG_BOOTSOURCE_FLASH:
      ret = copy_from_flash(dest);
      break;

    case CONFIG_BOOTSOURCE_MMC:
      ret = copy_from_mmc(dest);
      break;
    case CONFIG_BOOTSOURCE_NETWORK:
      ret = loader_copy_from_network(dest);
      break;
    case CONFIG_BOOTSOURCE_USB:
      ret = copy_from_usb(dest);
      break;

    default:
      printf("Unrecognised boot source - corrupt config?\n");
      return ret;
  }
  return ret;
}

