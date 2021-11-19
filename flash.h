#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

void WriteFLASH(u32 addr, unsigned char data);
void WriteFLASH16(u32 addr, unsigned short data);
unsigned char ReadFLASH(u32 addr, int delay);
unsigned char ReadFLASH_FAST(u32 addr);
unsigned short ReadFLASH16_FAST(u32 addr);
u32 ReadFLASH32_FAST(u32 addr);
int EON_EraseSector(u32 addr);
int EON_flash_erase(int progress);
int EON_config_erase();
int EON_ProgramWord(u32 addr, unsigned short val);

// dest will be a flash ram offset (i.e. not absolute).
// count is a count of shorts
int EON_ProgramRange(unsigned short* src, u32 dest,u32 count);
int EON_VerifyRange(unsigned short* src, u32 dest,u32 count);

// count is a count of shorts.
void EON_ZeroRange(u32 dest,u32 count);

// Return the real sector to copy
int EON_ChunkToSector(int chunk);

void EON_flash_test();
void flash_command(const char* arg);
void flash_setmac(u8* buf);
u32 flash_detect(u32 base);
void flash_print_info();
const char* flash_complete(const char* arg);
int flash_write_chunk(u16* src, u32 dest, u32 len);

#ifdef __cplusplus
}
#endif
