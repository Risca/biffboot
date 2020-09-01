
#include <stdlib.h>
#include <stdio.h>
#include "iolib.h"
#include "bootlinux.h"
#include <string.h>
//#include "LzmaDec.h"
#include "flash.h"
#include "flashmap.h"
#include "config.h"
#include "mmc.h"
#include "assem.h"



// Defines for the linux loader
#define SETUP_SIZE_OFF 	497
#define SECTSIZE       	512
#define SETUP_VERSION  	0x0201
#define SETUP_HIGH     	0x01
#define BIG_SYSSEG     	0x10000
#define DEF_BOOTLSEG	0x9020



struct bootsect_header {
    u8		pad0[0x1f1];
    u8		setup_sects;
    u16		root_flags;	// If set, the root is mounted readonly
    u16		syssize;	// DO NOT USE - for bootsect.S use only
    u16		swap_dev;	// DO NOT USE - obsolete
    u16		ram_size;	// DO NOT USE - for bootsect.S use only
    u16		vid_mode;	
    u16		root_dev;	// Default root device number
    u16		boot_flag;	// 0xAA55 magic number
};


struct setup_header {
    u8 		jump[2];
    u8 		magic[4];     	// "HdrS"
    u16 		version;      	// >= 0x0201 for initrd
    u8 		realmode_swtch[4];
    u16 		start_sys_seg;
    u16 		kernel_version;
    /* note: above part of header is compatible with loadlin-1.5 (header v1.5),*/
    /*       must not change it */
    u8 		type_of_loader;
    u8 		loadflags;
    u16 		setup_move_size;
    u32 	code32_start;
    u32 	ramdisk_image;
    u32 	ramdisk_size;
    u32 	bootsect_kludge;
    /* VERSION: 2.01+ */
    u16 		heap_end_ptr;
    u16 		pad1;
    /* VERSION: 2.02+ */
    u32 	cmd_line_ptr;
    /* VERSION: 2.03+ */
    u32 	initrd_addr_max;
};



#define PARAM			0x90000
#define PARAM_ORIG_X		*(u8*) (PARAM+0x000)
#define PARAM_ORIG_Y		*(u8*) (PARAM+0x001)
#define PARAM_EXT_MEM_K		*(u16*)(PARAM+0x002)
#define PARAM_ORIG_VIDEO_PAGE	*(u16*)(PARAM+0x004)
#define PARAM_ORIG_VIDEO_MODE	*(u8*) (PARAM+0x006)
#define PARAM_ORIG_VIDEO_COLS	*(u8*) (PARAM+0x007)
#define PARAM_ORIG_VIDEO_EGA_BX	*(u16*)(PARAM+0x00a)
#define PARAM_ORIG_VIDEO_LINES	*(u8*) (PARAM+0x00E)
#define PARAM_ORIG_VIDEO_ISVGA	*(u8*) (PARAM+0x00F)
#define PARAM_ORIG_VIDEO_POINTS	*(u16*)(PARAM+0x010)

#define PARAM_ALT_MEM_K		*(u32*)(PARAM+0x1e0)
#define PARAM_E820NR		*(u8*) (PARAM+0x1e8)
// hlin ==>
#define PARAM_DF_ROOT		*(u16*) (PARAM+0x1fc)
#define PARAM_LOADER		*(u8*) (PARAM+0x210)
#define PARAM_INITRD_ADDR	*(u32*)(PARAM+0x218)
#define PARAM_INITRD_LEN	*(u32*)(PARAM+0x21c)
// hlin <==
#define PARAM_VID_MODE		*(u16*)(PARAM+0x1fa)
#define PARAM_E820MAP		(struct e820entry*)(PARAM+0x2d0);
#define PARAM_CMDLINE		(char *)(PARAM+0x3400)



void bootlinux_go(u32 length)
{
    u32 base_addr = config_loadaddress_get();

    u32 mem_size = 0x2000000;   /// 32 meg

    u32 int15_e801 = 0;

    struct bootsect_header *bs_header = 0;
    struct setup_header *s_header = 0;
    int setup_sects = 0;
    
    // Ensure we're operating with a standard gdt
    assem_linux_gdt();

    // zero any low memory
    dma_free_all();

    // Round length up to the next quad word
    length = (length + 3) & ~0x3;

    mem_size >>= 10;   // convert from bytes to kilobytes.
    // Result of int15 ax=0xe801
    int15_e801 = mem_size - 1024 ; // 1M+ only

    bs_header = (struct bootsect_header *)base_addr;
    s_header = (struct setup_header *)(base_addr + SECTSIZE);

    if (bs_header->boot_flag != 0xAA55) {
        printf("boot_flag not found\n");
        return;
    }
    if (memcmp(s_header->magic,"HdrS",4) != 0) {
        printf("Linux kernel not found\n");
        return;
    }
    if (s_header->version < SETUP_VERSION) {
        printf("Kernel too old\n");
        return;
    }

    setup_sects = bs_header->setup_sects ? bs_header->setup_sects : 4;

    // + 1 for boot sector
    base_addr += (setup_sects + 1 ) * SECTSIZE;
    length -= (setup_sects + 1 ) * SECTSIZE;

    // Clear the data area
    memset ( (void*)PARAM, 0, 0x10000 );

    strcpy( PARAM_CMDLINE, config_cmndline_get() );

    printf("Booting Linux with: ");
    printf(config_cmndline_get());
    printf("\n");

    memcpy((void*)(PARAM+SECTSIZE), s_header, sizeof(struct setup_header));
    s_header = (struct setup_header*)(PARAM+SECTSIZE);

    s_header->version = SETUP_VERSION;

    // Command Line
    s_header->cmd_line_ptr = (u32)PARAM_CMDLINE;
 
    // Loader type
    s_header->type_of_loader = 0xFF;

    // Fill in the interesting bits of data area...
    // ... Memory sizes
    PARAM_EXT_MEM_K = int15_e801;
    PARAM_ALT_MEM_K = (u32)int15_e801;

    // ... No e820 map!
//    PARAM_E820NR = (u8)0; 	// Length of map

//    PARAM_DF_ROOT = (u16)0x0000 ;	// default root device

    PARAM_LOADER = (u8)0x20 ;		// type of loader
//    PARAM_INITRD_ADDR = (u32)0 ;		// ramdisk address
//    PARAM_INITRD_LEN = (u32)0 ;	// ramdisk size

//    printf("Copy to: ");   print_hex32(s_header->code32_start);
//    printf(" Copy from: "); print_hex32(base_addr);
//    printf(" bytes: "); print_hex32(length);
//    printf("\n");
    
    memcpy((char*)s_header->code32_start, (char*)base_addr, length);
    assem_linux_boot(s_header->code32_start, PARAM);

    while (1);
}


