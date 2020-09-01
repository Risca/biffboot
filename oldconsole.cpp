
#include "iolib.h"
#include "flashmap.h"
#include "flash.h"
#include "md5.h"
#include <string.h>
#include <stdio.h>
#include "oldconsole.h"


// Buffer for received kernel data, for serial flashing.
typedef struct pkt
{
char magic[4];
md5_byte_t digest[16];
unsigned char buffer[8192];
} pkt_t;

static pkt_t g_pkt;
static md5_state_t g_pms;


void oldconsole_run()
{
  int key = 1;
  md5_byte_t digest[16];
  int chunk;
  u32 start_address = 0;
  printf("Erasing flash\n");
  
  while (key != '4')
  {
    if (key) prompt();
    key = getkey(0);
    if (!key) continue;
    if (key==27) { key = 0; continue; }
    putchar(key);
    printf("\n");
    switch (key)
    {
      case '2':
        printf("\r\nWaiting for data...");
        get_serial_data((unsigned char*)&g_pkt, sizeof(g_pkt));
          
        md5_init(&g_pms);
        md5_append(&g_pms, g_pkt.buffer, sizeof(g_pkt.buffer));
        md5_finish(&g_pms, digest);
          
        if (0==memcmp((void*)digest, (void*)g_pkt.digest, sizeof(digest)))
        {
          printf("Digest matches, OK\n");
        } else {
          printf("Received Digest: ");
          hex_dump((void*)g_pkt.digest, sizeof(g_pkt.digest));
          printf("\n");
          printf("Computed Digest: ");
          hex_dump((void*)digest, sizeof(digest));
          printf("\n");
          printf("Digest doesn't match ERROR\n");
        }
          
        break;
         
      case '3':
        printf("\r\nEnter chunk position (3 digits)\n");
        prompt();
        chunk = EON_ChunkToSector(getint(3));
        printf("\r\nSelected chunk: %x\n", chunk);
        if (chunk < FLASHMAP_CHUNK_COUNT)
        {
        
          start_address = chunk*sizeof(g_pkt.buffer);

          // check if we are on sector boundary, and erase as needed.
          if (flashmap_isboundary(start_address))
          {
            // Check if this sector is all 0xff
            if (!flashmap_iserased(start_address))
            {
              // Need to delete this sector first.
              EON_EraseSector(start_address);   
            }
          }

          //hex_dump((void*)g_pkt.buffer, 32);
          //printf("\n");
          EON_ProgramRange((unsigned short*)g_pkt.buffer, start_address, sizeof(g_pkt.buffer)/2);
          if (EON_VerifyRange((unsigned short*)g_pkt.buffer, start_address, sizeof(g_pkt.buffer)/2) ==0)
            printf("Programmed OK\n");
        }
        break;
      }
   }       
}
