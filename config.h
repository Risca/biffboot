#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

#include "iolib.h"

void config_init(void);
// find out if config contains a valid block
int config_verify(void);

void config_store(const char* arg);
void config_revert(const char* arg);

void config_dump(const char* arg);

int config_console_get(void);

#define CONFIG_BOOTSOURCE_FLASH    0
#define CONFIG_BOOTSOURCE_MMC      1
#define CONFIG_BOOTSOURCE_NETWORK  2
#define CONFIG_BOOTSOURCE_USB      3
#define CONFIG_BOOTSOURCE_MAX      3      // always the last one

int config_bootsource_get(void);

#define CONFIG_NIC_DISABLED        0
#define CONFIG_NIC_ENABLED         1
#define CONFIG_NIC_PROMISCUOUS     2
#define CONFIG_NIC_MAX             2

int config_nic_get(void);

u32 config_loadaddress_get(void);

#define CONFIG_BOOTTYPE_FLATBIN    0
#define CONFIG_BOOTTYPE_LINUX26    1
#define CONFIG_BOOTTYPE_MULTI      2
#define CONFIG_BOOTTYPE_COREBOOT   3
#define CONFIG_BOOTTYPE_MAX        3
int config_boottype_get(void);

char* config_cmndline_get(void);

// default command-lines for various purposes.
void config_cmndline_usbroot();
void config_cmndline_mtdroot();
void config_cmndline_rdroot();


u32 config_kernelmax_get(void);

// Do we check the button on boot?
#define CONFIG_BUTTON_FORCEDOFF 0
#define CONFIG_BUTTON_ON 1
#define CONFIG_BUTTON_FORCEDON 2
int config_button_get(void);

#define CONFIG_UPGRADE_NOT_AVAILABLE 0
#define CONFIG_UPGRADE_AVAILABLE     1
int config_upgrade_available_get(void);

int config_bootcount_get(void);
// returns -1 for config version 1
int config_boot_part_get(void);

void config_setdefaults(const char*);
void config_set_value(const char* arg);

int config_get_changed();

typedef const char* (*config_complete_t)(const char*);
#define NULL_COMPLETE ((config_complete_t)0)

const char* config_complete(const char* partial);

#ifdef __cplusplus
}
#endif
