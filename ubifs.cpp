
#include "crc32.h"

#include "ether.h"
#include "iolib.h"
#include "flash.h"
#include "config.h"
#include "flashmap.h"
#include <string.h>
#include <stdio.h>


#define UBI_HASH 0x23494255
#define UBI_BANG 0x21494255
#define UBI_NON_INDEX 0x7fc1555b
#define UBI_INDEX_END 0x889bc4d4
#define UBI_DATA_END 0xc8069395


static u32 g_ptr_u32 = 0;



// erase given sector
static void clear_sector()
{
  while (!flashmap_isboundary(g_ptr_u32))
    g_ptr_u32 ++;
  print_erase(8);
  printf("%x", g_ptr_u32);
  EON_EraseSector(g_ptr_u32);
}



// Sequence number.  Really just a tag for the write operation, pick one.
//#define UBI_IMAGE_SEQ 0x7a2c361d
#define UBI_IMAGE_SEQ 0x24104671

// Erase counter header magic number (ASCII "UBI#") 
#define UBI_EC_HDR_MAGIC  0x55424923

// Volume identifier header magic number (ASCII "UBI!") 
#define UBI_VID_HDR_MAGIC 0x55424921

// The initial CRC32 value used when calculating CRC checksums
#define UBI_CRC32_INIT 0xFFFFFFFFU

// Volume name
#define UBI_VOL_NAME_MAX 127



static void ProgramRec(u8* src, u32 len)
{
  EON_ProgramRange((u16*)src, g_ptr_u32, len/2);
  g_ptr_u32 += len;
}



typedef struct ubi_ec_hdr {
        u32  magic;
        u8   version;
        u8   padding1[3];
        u32  ec_high;
        u32  ec;  // only this used
        u32  vid_hdr_offset;
        u32  data_offset;
        u32  image_seq;
        u8   padding2[32];
        u32  hdr_crc;
} __attribute__ ((packed)) ubi_ec_hdr_t;


typedef struct ubi_vid_hdr {
        u32  magic;
        u8   version;
        u8   vol_type;
        u8   copy_flag;
        u8   compat;
        u32  vol_id;
        u32  lnum;
        u32  leb_ver;
        u32  data_size;
        u32  used_ebs;
        u32  data_pad;
        u32  data_crc;
        u8   padding2[4];
        u32  sqnum_high;
        u32  sqnum;
        u8   padding3[12];
        u32  hdr_crc;
} __attribute__ ((packed)) ubi_vid_hdr_t;


typedef struct ubi_vtbl_rec {
        u32  reserved_pebs;
        u32  alignment;
        u32  data_pad;
        u8   vol_type;
        u8   upd_marker;
        u16  name_len;
        char name[UBI_VOL_NAME_MAX+1];
        u8   flags;
        u8   padding[23];
        u32  crc;
} __attribute__ ((packed)) ubi_vtlb_rec_t;


// Fill in some default values
void ubifs_ec_header(u32 ec, u32 seq)
{
  ubi_ec_hdr_t hdr;
  memset(&hdr, 0, sizeof(hdr));
  hdr.magic = htonl(UBI_EC_HDR_MAGIC);
  hdr.version = 1;
  hdr.ec = htonl(ec);
  hdr.vid_hdr_offset = htonl(0x40);
  hdr.data_offset = htonl(0x80);
  hdr.image_seq = htonl(seq);
  

  hdr.hdr_crc = htonl( crc32(UBI_CRC32_INIT, &hdr, sizeof(hdr) - sizeof(u32)) );
  
  ProgramRec((u8*)&hdr, sizeof(hdr));
}


// Fill in some default values
void ubifs_vid_header(u32 lnum, u32 sqnum)
{
  ubi_vid_hdr_t hdr;
  memset(&hdr, 0, sizeof(hdr));
  hdr.magic = htonl(UBI_VID_HDR_MAGIC);
  hdr.version = 1;
  hdr.vol_type = 1;
  hdr.copy_flag = 0;
  hdr.compat = 5;
  hdr.vol_id = htonl(0x7fffefff);
  hdr.lnum = htonl(lnum);
  hdr.sqnum = htonl(sqnum);
  hdr.hdr_crc = htonl(crc32(UBI_CRC32_INIT, &hdr, sizeof(hdr) - sizeof(u32)));

  ProgramRec((u8*)&hdr, sizeof(hdr));
}


void ubifs_vtlb_rec(const char* name, u32 pebs)
{
  ubi_vtlb_rec_t rec;
  memset(&rec, 0, sizeof(rec));

  if (name)
  {  
    rec.reserved_pebs = htonl(pebs);
    rec.alignment = htonl(1);
    rec.vol_type = 1;
    rec.name_len = htons(strlen(name));
    strcpy(rec.name, name);
  }
  rec.crc = htonl(crc32(UBI_CRC32_INIT, &rec, sizeof(rec) - sizeof(u32)));
  
  ProgramRec((u8*)&rec, sizeof(rec));
}


void ubifs_data_sector()
{
	clear_sector();
	ubifs_ec_header(1, 0);
}


void ubifs_data_sector_seq()
{
	clear_sector();
	ubifs_ec_header(0, UBI_IMAGE_SEQ);
}


void ubifs_vid_sector(u32 count, u32 lnum, u32 sqnum, u32 pebs)
{
	clear_sector();
	ubifs_ec_header(0, UBI_IMAGE_SEQ);
	ubifs_vid_header(lnum, sqnum);
	ubifs_vtlb_rec("bbroot", pebs);
	while (count--)
	{
		ubifs_vtlb_rec(NULL_STRING, 0);
	}
}




#define MAX_KMAX (FLASHMAP_SIZE-FLASHMAP_BOOT_SIZE)

#define MAX_SENSIBLE_SIZE (MAX_KMAX - 0x210000)

void ubifs_command(const char* arg)
{	
  u32 i;
  u32 pebs = MAX_KMAX/0x10000 - config_kernelmax_get()/0x10000 - 0x4;
  if (FLASHMAP_DEVICE_SIZE == 1)
  {
    printf("Error: flash is too small to support ubifs\n");
    return;
  }
  if (config_kernelmax_get() > u32(MAX_SENSIBLE_SIZE))
  {
    printf("Error: kernelmax setting doesn't leave enough space for a UBIFS partition\n");
    return;
  }
  printf("This will erase your flash above 0x");
  g_ptr_u32 = config_kernelmax_get();
  printf("%x",g_ptr_u32);
  printf(",");
  if (!are_you_sure())
    return;
  printf("Formatting: 0x        ");
  
  
  ubifs_data_sector();
  ubifs_data_sector();

  for (i=0;i<pebs;i++)
  {
    ubifs_data_sector_seq();
  }

  ubifs_vid_sector(127, 1, 2, pebs);
  ubifs_vid_sector(127, 0, 1, pebs);

  printf("\r\nFormat complete\n");
}

