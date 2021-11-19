#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

int mmc_init(void);

int mmc_read (unsigned char *buff, unsigned long sector, unsigned char count);

#ifdef __cplusplus
}
#endif

