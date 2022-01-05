#pragma once
#ifdef __cplusplus
extern "C" {
#endif 


extern u32 FLASHMAP_DEVICE_SIZE;
extern u32 FLASHMAP_BASE;
extern u32 FLASHMAP_SIZE;
#define FLASHMAP_CONFIG 0x4000
extern u32 FLASHMAP_KERNEL_SIZE;

// Always this value
#define FLASHMAP_CHUNK_SIZE 0x2000

#define FLASHMAP_BOOT_SIZE 0x10000

extern u32 FLASHMAP_CHUNK_COUNT;

// flash detection codes.
#define FLASH_CODE_EN800 (0x7f1c225b)
#define FLASH_CODE_EN320 (0x7f1c22f9)
#define FLASH_CODE_EN640 (0x7f1c22cb)


// the flash detected during init.
u32 flashmap_get_detected();

// Detect flash and setup the various parameters
void flashmap_init();

// Return true if this address is start of sector.
int flashmap_isboundary(unsigned long addr);

// Return true if given flash sector is all 0xff
int flashmap_iserased(unsigned long addr);

#ifdef __cplusplus
}
#endif
