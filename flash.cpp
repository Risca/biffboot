#include <string.h>
#include <stdio.h>
#include "iolib.h"
#include "flash.h"
#include "flashmap.h"


#define ERROR_FAULT   -3
#define ERROR_RANGE   -2      // error with range
#define ERROR_TIMEOUT -1      // fault programming
#define ERROR_NONE     0       // no error

// Positive returns give timing info.



void WriteFLASH(u32 addr, unsigned char data)
{
  *(unsigned char *)(FLASHMAP_BASE+addr) = data;
}
void WriteFLASH16(u32 addr, unsigned short data)
{
  *(unsigned short *)(FLASHMAP_BASE+addr) = data;
}

unsigned char ReadFLASH(u32 addr, int delay)
{
  unsigned char val;
  int i;
  val = *(unsigned char *)(FLASHMAP_BASE+addr);
  for (i=0;i<delay;i++)
  {   // for some reason reading needs a wait state.  values under 5 cause problems so I backed off
      // to 8.
  };
  return val;
}

unsigned char ReadFLASH_FAST(u32 addr)
{
  return *(unsigned char *)(FLASHMAP_BASE+addr);
}

unsigned short ReadFLASH16_FAST(u32 addr)
{
  return *(unsigned short *)(FLASHMAP_BASE+addr);
}

u32 ReadFLASH32_FAST(u32 addr)
{
  return *(u32 *)(FLASHMAP_BASE+addr);
}


int EON_EraseSector(u32 addr)
{
  int prev, cur, count = 0;

  if (addr>=(FLASHMAP_SIZE-FLASHMAP_BOOT_SIZE)) return ERROR_RANGE;   // refuse to erase the bootloader
  
  WriteFLASH(0xAAA,0xAA);
  WriteFLASH(0x555,0x55);
  WriteFLASH(0xAAA,0x80);
  WriteFLASH(0xAAA,0xAA);
  WriteFLASH(0x555,0x55);
  WriteFLASH(addr,0x30);

  prev = ReadFLASH(addr,8);
  //printf("value: %x\n", prev);
  prev &= 0x40;

  cur  = ReadFLASH(addr,8);
  //printf("value: %x\n", cur);
  cur  &= 0x40;

  while (prev != cur)
  {
    prev = ReadFLASH(addr,8);
    //printf("value: %x\n", prev);
    prev &= 0x40;

    cur  = ReadFLASH(addr,8);
    //printf("value: %x\n", cur);
    cur  &= 0x40;

    if (cur & 0x20)  // DQ5==1
    {
      prev = ReadFLASH(addr,8) & 0x40;
      cur  = ReadFLASH(addr,8) & 0x40;
      if (prev!=cur) count = 0xffffff;
      break;
    }
    cur &= 0x40;
    //printf("value: %x %d\n", cur, count);
    if (count++ > 0x100000) break;   // way too long.
  }

  if (count>0x100000)
  {
    return ERROR_TIMEOUT;
  } else {
    return count;
  }
}


int EON_config_erase()
{  
  u32 i = FLASHMAP_CONFIG;
  if (!flashmap_isboundary(i)) 
    return ERROR_RANGE;
  if (flashmap_iserased(i)) return ERROR_NONE;
  
  EON_EraseSector(i);
  // Now check afterwards...
  if (!flashmap_iserased(i))
  {
    printf("Found fault in sector %x, try again :-(\n", unsigned(i));
    return ERROR_FAULT;
  }
  return ERROR_NONE;
}



// Doesn't erase the config area.
int EON_flash_erase(int progress)
{ 
  u32 i;
  int ret = ERROR_NONE;

  // then the 64KB sectors.
  if (progress) {
    printf("Erasing: 0x%x", 0);
  }
  i = 0x0;
  while (i<(FLASHMAP_SIZE-FLASHMAP_BOOT_SIZE))
  {
    if (flashmap_isboundary(i))
    {
      // skip config area @0x4000
      if (i!=FLASHMAP_CONFIG) 
      {
        if (progress)
        {
          print_erase(8);
	  printf("%x", unsigned(i));
        }
        if (!flashmap_iserased(i))
        {
          EON_EraseSector(i);  // try once
          // Now check afterwards...
          if (!flashmap_iserased(i))
          {
            printf("\r\nFound fault in sector %x\n", unsigned(i));
            ret = ERROR_FAULT;
          }
        }
      }
    }
    i += FLASHMAP_CHUNK_SIZE;
  }
  if (progress) printf("\n");

  return ret;
}


int EON_ProgramWord(u32 addr, unsigned short val)
{
  int prev, cur;
  u32 count = 0;
  if (
      (addr>=(FLASHMAP_SIZE-FLASHMAP_BOOT_SIZE)) &&
      (addr<(FLASHMAP_SIZE-8))
     ) return ERROR_RANGE;  // bootloader

  WriteFLASH(0xAAA,0xAA);
  WriteFLASH(0x555,0x55);
  WriteFLASH(0xAAA,0xA0);
  WriteFLASH16(addr,val);
  prev = ReadFLASH(addr,1) & 0x40;
  cur  = ReadFLASH(addr,1) & 0x40;
  while (prev != cur)
  {
    prev = ReadFLASH(addr,1) & 0x40;
    cur  = ReadFLASH(addr,1);
    if (cur & 0x20)  // DQ5==1
    {
      prev = ReadFLASH(addr,1) & 0x40;
      cur  = ReadFLASH(addr,1) & 0x40;
      if (prev!=cur) count = 0xffffff;
      break;
    }
    cur &= 0x40;
    //printf("value: %x %d\n", cur, count);
    if (count++ > 0x100000) break;   // way too long.
  }

  if (count>0x10000)
    return ERROR_TIMEOUT;  // error

  //printf("Programmed 0x%x in %d ticks\n", addr, count);
  return count;
}


// dest will be a flash ram offset (i.e. not absolute).
// count is a count of shorts
int EON_ProgramRange(unsigned short* src, u32 dest,u32 count)
{
  unsigned short val;
  while (count--)
  {
    val = *src;
    //printf("dest: %x, val: %x\n", dest, val);
    if (EON_ProgramWord(dest, val)<0)
      return ERROR_FAULT;   // programmed incorrectly
    src++;
    dest += 2;
  }
  return ERROR_NONE;
}


int EON_VerifyRange(unsigned short* src, u32 dest,u32 count)
{
  unsigned short val;
  while (count--)
  {
    val = *src;
    if (val!=ReadFLASH16_FAST(dest))
    {
      return ERROR_FAULT;
    }
    src++;
    dest += 2;
  }
  return ERROR_NONE;
}


// count is a count of shorts.
void EON_ZeroRange(u32 dest,u32 count)
{
  while (count--)
  {
    EON_ProgramWord(dest, 0);
    dest += 2;
  }
}


// legacy  location  With config    (16k/8k/8k/32k sectors)
//    000   0x0000      117
//    001   0x2000      118
//    002   0x4000      119
//    003   0x6000      000
//    004   0x8000      001
//    ...
//    116               113
//    119               116


// Given the chunk number, return the actual location of the 8k block
int EON_ChunkToSector(int chunk)
{
  if (chunk < (FLASHMAP_CHUNK_COUNT-3))
  {
    return chunk + 3;   // Real place to write.
  }
  else
  {
    return chunk - (FLASHMAP_CHUNK_COUNT-3);   // real place to write
  }
}


typedef struct _entry
{
  const char* name;
  int value;
} _entry_t; 


#define FLASH_ARG_ERASE 0
#define FLASH_ARG_TEST 1

static struct _entry g_entries[] = 
{
  {"erase", FLASH_ARG_ERASE},
  {"test", FLASH_ARG_TEST},
  {0, 0}
};



// Test all flash.
void EON_flash_test()
{
  u32 addr;
  int error=0;
  if (EON_flash_erase(1)>=0)
  {
    printf("Erase OK\n");
  }
  else
  {
    printf("Erase failed, aborting test\n");
    return;
  }
  
  addr = 0;

  printf("Program/verify with zeros: 0x%x", 0);
  
  while (addr < (FLASHMAP_SIZE-FLASHMAP_BOOT_SIZE))
  {
    if ((addr < (FLASHMAP_CONFIG)) || (addr >= (FLASHMAP_CONFIG+FLASHMAP_CHUNK_SIZE)))
    { 
      if (EON_ProgramWord(addr, 0) < 0)
      {
        printf("Error at: %x\n", unsigned(addr));
        
        error =1;
        break;
      }
      else
      {
        if (!(addr%0x2000))
        {
          print_erase(8);
          printf("%x",unsigned(addr));
        }
      }
    }
    addr += 2;
  }
  printf("\n");
  if (!error) {
    printf("Program OK, flash looks good :-)\n");
  }
}


u32 flash_detect(u32 base)
{
  if (base)
    FLASHMAP_BASE = base;
  
  u32 ret = 0;
  /* put flash in auto-detect mode */
  WriteFLASH16(0xaaa, 0xaa);
  WriteFLASH16(0x554, 0x55);
  WriteFLASH16(0xaaa, 0x90);

  /* Read the auto-config data - 4 values in total */
  ret = ReadFLASH(0x0000, 8);
  ret <<= 8;
  ret |= ReadFLASH(0x0200, 8);
  ret <<= 8;
  ret |= ReadFLASH(0x0003, 8);
  ret <<= 8;
  ret |= ReadFLASH(0x0002, 8);

  /* exit the autodetect state */
  WriteFLASH16(0x0000, 0xf0f0);
  
  switch (ret)
  {
    case FLASH_CODE_EN800: return ret;
    case FLASH_CODE_EN320: return ret;
    case FLASH_CODE_EN640: return ret;
    default:
      printf("Detected %x\n", ret);
  }
 	 
  return 0;
}



void flash_print_info()
{
  u32 detected = flashmap_get_detected();
  
  printf("Detected: ");

  switch (detected)
  {
    case FLASH_CODE_EN800:
      printf("EN29LV800B");
      break;
    case FLASH_CODE_EN320:
      printf("EN29LV320B");
      break;      
    case FLASH_CODE_EN640:
      printf("EN29LV640B");
      break;      
    default:
      printf("Unknown");
  }
  printf("\n");
  printf("Read manufacturer/device code 0x%x\n", detected);
  
  printf("FLASHMAP_DEVICE_SIZE (MB): 0x%x\n", FLASHMAP_DEVICE_SIZE);
  printf("FLASHMAP_SIZE (Bytes): 0x%x\n", FLASHMAP_SIZE);
  printf("FLASHMAP_BASE: 0x%x\n", FLASHMAP_BASE);
  printf("FLASHMAP_KERNEL_SIZE: 0x%x\n", FLASHMAP_KERNEL_SIZE);
  printf("FLASHMAP_BOOT_SIZE: 0x%x\n", FLASHMAP_BOOT_SIZE);
    
}


void flash_command(const char* arg)
{
_entry_t* ptr = g_entries;
const char* tok;
int val = 0;

  flash_print_info();

  while (ptr->name)
  {
    tok = eat_token(arg, ptr->name);
    if (tok)
    {
      val = ptr->value;
      break;     
    }     
    ptr++;
  }
  if (!tok)
  {
    printf("flash: Unrecognised option ('erase' or 'test')\n");
    return;
  }

  printf("This will erase your flash,");
  if (!are_you_sure())
    return;
  
  switch (val)
  {
    case FLASH_ARG_ERASE:
      if (EON_flash_erase(1)>=0)
      {
        printf("Flash Erase OK\n");
      } else 
      {
        printf("Flash Erase failed\n");
      }
      break;
    case FLASH_ARG_TEST:
      EON_flash_test();
      break;
  }
}


// program the 6-byte MAC address at end of flash (if not set already)
void flash_setmac(u8* buf)
{
  unsigned short* ptr = (unsigned short*)buf;
  EON_ProgramRange(ptr, FLASHMAP_SIZE-8,3);
}




const char* flash_complete(const char* partial)
{
_entry_t* ptr = g_entries;
_entry_t* ptr_out = (_entry_t*)NULL;
u32 count=0;

  while (ptr->name)
  {
    if (strncmp(partial, ptr->name, strlen(partial))==0)
    {
      count++;
      ptr_out = ptr;
    }
    ptr++;
  }
  if (count==1)
  {
    return ptr_out->name;
  }
  return (const char*)NULL;
}



// Src = data to write
// dest = target flash offset
// len = length
int flash_write_chunk(u16* src, u32 dest, u32 len)
{
  int tmp;
  
  // Check if the specified location makes sense.
  if ((dest + len) > (FLASHMAP_SIZE - FLASHMAP_BOOT_SIZE)) 
  {
    //printf("dest: ");
    //print_hex32(dest);
    //printf("\n");
    
    //printf("len: ");
    //print_hex32(len);
    //printf("\n");
    
    //printf("size: ");
    //print_hex32(FLASHMAP_SIZE);
    //printf("\n");
    
    //printf("boot: ");
    //print_hex32(FLASHMAP_BOOT_SIZE);
    //printf("\n");
    
    
    
    printf("Invalid location\n");
    return 1;
  }
  
  //printf("sector: ");
  //print_hex32(dest);
  //printf("\n");
  //printf("start_address: ");
  //print_hex32(start_address);
  //printf("\n");
    
  // check if we are on sector boundary, and erase as needed.
  if (flashmap_isboundary(dest))
  {
    // Check if this sector is all 0xff
    if (!flashmap_iserased(dest))
    {
      // Need to delete this sector first.
      //printf("Erasing sector\n");
      EON_EraseSector(dest);   
         
    }
  }
  
  tmp = EON_ProgramRange( src,
                          dest,
                          len/2);
  
  if (tmp<0) return 1;   // bail on any error.
  
  tmp = EON_VerifyRange( src,
                          dest, 
                          len/2);
  if (tmp<0) return 1;   // bail on error
  
  return 0;   // tell the caller.
}



