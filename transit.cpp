// Transit buffer accumulation and validation

#include <string.h>
#include "stdlib.h"
#include "md5.h"
#include "iolib.h"
#include "transit.h"


// Buffer for received kernel data
typedef struct pkt
{
unsigned char buffer[8192];
} pkt_t;

static pkt_t* g_pkt=(pkt_t*)NULL;
md5_state_t g_pms;
size_t g_offset=0;


void transit_init()
{
  g_pkt = (pkt_t*)malloc(sizeof(pkt_t));
}

void transit_reset()
{
  g_offset = 0;
}

// Return '0' if the buffer fits, 1 if overrun.
int transit_add(unsigned char * buffer, size_t length)
{
  unsigned char* ptr;
  if ((g_offset+length)>sizeof(pkt_t)) return 1;
  ptr = (unsigned char*)g_pkt;
  ptr += g_offset;
  memcpy(ptr, buffer, length);
  g_offset += length;
  return 0;
}

unsigned char* transit_payload_ptr()
{
  return g_pkt->buffer;
}


unsigned long transit_payload_length()
{
  return sizeof(g_pkt->buffer);
}


// return the digest (16 bytes), copied to supplied buffer.
void transit_digest(unsigned char* copyto)
{
  md5_byte_t digest[16];
  
  md5_init(&g_pms);
  md5_append(&g_pms, g_pkt->buffer, sizeof(g_pkt->buffer));
  md5_finish(&g_pms, digest);

  memcpy(copyto, digest, sizeof(digest));
}

