
#include <string.h>
#include <stdio.h>
#include "iolib.h"
#include "flash.h"
#include "flashmap.h"
#include "md5.h"
#include "stdlib.h"
#include "config.h"


// The entire 8k config block
struct _cfg
{
  u8 buffer[0x1ff0];
  md5_byte_t digest[0x10];
};


struct _cfg_vals
{
  // v1
  int version;       // one for first version
  u8 bootsource;   // 0=flash, 1=MMC 2=NET 3=USB  (0)
  u8 console; // 0 = no console output (console input disabled if button enabled), 1= console output (1)
  u8 nic; // 0 = no nic, 1= nic init, 2=promiscuous  (1)
  u8 boottype;  // 3 == Coreboot payload, 2 == Multiboot, 1 == linux, 0 == flat bin
  u32 loadaddress;  // load address of payload (0x400000)
  char cmndline[1024];  // null term, 1023 chars max
  unsigned short kernelmax;  // counted in sectors
  u8 button;             // 1=enabled, 0=disabled
  // v2
  u8 upgrade_available;
  u8 bootcount;
  u8 boot_part;
}  __attribute__((__packed__));


// Definitions for type information.

typedef struct _cfg_opt_enum
{
  u8 val;
  const char* name;
} _cfg_opt_enum_t;


// structure to hold the hex type
typedef struct _cfg_opt_hex
{
  u32 min;
  u32 max;
  u32 step;  // how many units to allow.
} _cfg_opt_hex_t;


#define CONFIG_TYPE_ENUM 0
#define CONFIG_TYPE_STRING 1   // always 1024 bytes
#define CONFIG_TYPE_MEMADDR 2
#define CONFIG_TYPE_SECTOR 3
#define CONFIG_TYPE_BYTE 4


typedef struct _cfg_value
{
  const char* name;    // name, for the 'set' menu
  const char* description;    // long description menu
  char  type;		// type of the parameter (CONFIG_TYPE_*)
  void* typeinfo;    // opaque structure depending on CFG type 
  u32 initial;    // structure with default value
  u32 offset;
} _cfg_value_t;


// Low-level containers for the config type information

static struct _cfg_opt_enum config_console_opts[] = {
  {0, "disabled"},
  {1, "enabled"},
  {0, (const char*)NULL}
};

static struct _cfg_opt_enum config_nic_opts[] = {
  {CONFIG_NIC_DISABLED, "disabled"},
  {CONFIG_NIC_ENABLED, "enabled"},
  {CONFIG_NIC_PROMISCUOUS, "promiscuous"},
  {0, (const char*)NULL}
};

static struct _cfg_opt_enum config_bootsource_opts[] = {
  {CONFIG_BOOTSOURCE_FLASH, "on-board flash"},
  {CONFIG_BOOTSOURCE_MMC, "MMC/SD"},
  {CONFIG_BOOTSOURCE_NETWORK, "Network"},
//  {CONFIG_BOOTSOURCE_USB, "USB"},
  {0, (const char*)NULL}
};

static struct _cfg_opt_enum config_boottype_opts[] = {
  {CONFIG_BOOTTYPE_FLATBIN, "Flat binary (entry point==load address)"},
  {CONFIG_BOOTTYPE_LINUX26, "Linux 2.6 parameter block"},
  {CONFIG_BOOTTYPE_MULTI, "Multiboot (ELF)"},
  {CONFIG_BOOTTYPE_COREBOOT, "Coreboot payload"},
  {0, (const char*)NULL}
};

#define LOAD_ADDR_MAX 0x1000000
#define LOAD_ADDR_DEF 0x400000

static struct _cfg_opt_hex config_loadaddress_opts = {
  min : 0x50000,
  max : LOAD_ADDR_MAX,
  step : 0x1,
};

static struct _cfg_opt_hex config_kernelmax_opts= {
  min : 0x1,
  max : 0x7f,   // the equivalent for 8MB devices.
  step : 0x1,
};

static u32 KERNELMAX_MAX = 0;


static struct _cfg_opt_enum config_button_opts[] = {
  {CONFIG_BUTTON_FORCEDOFF, "forced off"},
  {CONFIG_BUTTON_ON, "enabled"},
  {CONFIG_BUTTON_FORCEDON, "forced on"},
  {0, (const char*)NULL}
};

static struct _cfg_opt_enum config_upgrade_opts[] = {
  {CONFIG_UPGRADE_NOT_AVAILABLE, "Upgrade NOT in progress"},
  {CONFIG_UPGRADE_AVAILABLE, "Upgrade in progress"},
  {0, (const char*)NULL}
};

static struct _cfg_opt_hex config_bootcount_opts = {
  min : 0x0,
  max : 0x3,
  step : 0x1,
};

static struct _cfg_opt_hex config_boot_part_opts = {
  min : 0x0,
  max : 0xff,
  step : 0x1,
};


#define PV(n) ((void*)(n))

static struct _cfg_value config_values[] = {
  {"bootsource", "Where to load the kernel from",
  CONFIG_TYPE_ENUM, PV(config_bootsource_opts), CONFIG_BOOTSOURCE_FLASH, 0},
  {"console",    "Whether the serial console is enabled",
  CONFIG_TYPE_ENUM, PV(config_console_opts), 1, 0},
  {"nic",        "Determines whether ethernet flashing is possible",
  CONFIG_TYPE_ENUM, PV(config_nic_opts), CONFIG_NIC_ENABLED, 0},
  {"boottype",   "The type of binary to boot",
  CONFIG_TYPE_ENUM, PV(config_boottype_opts), CONFIG_BOOTTYPE_LINUX26, 0},
  {"loadaddress","Where to boot the binary from",
  CONFIG_TYPE_MEMADDR,  PV(&config_loadaddress_opts), LOAD_ADDR_DEF, 0},
  {"cmndline",   "The command-line to supply to the booted kernel or multiboot image",
  CONFIG_TYPE_STRING, PV(NULL), 
    (u32)"", 0},
  {"kernelmax",  "The number of 0x10000-byte sectors taken up by the kernel",
  CONFIG_TYPE_SECTOR,  PV(&config_kernelmax_opts), 0x10, 0},
  {"button",     "Whether the button triggers UDP flashing mode on power-on",
  CONFIG_TYPE_ENUM, PV(config_button_opts), 1, 0},
  {"upgrade",    "Whether upgrade is in progress",
  CONFIG_TYPE_ENUM, PV(config_upgrade_opts), CONFIG_UPGRADE_NOT_AVAILABLE, 0},
  {"bootcount",  "Incremented when upgrade_available == 1",
  CONFIG_TYPE_BYTE, PV(&config_bootcount_opts), 0, 0},
  {"boot_part",  "Boot partition index",
  CONFIG_TYPE_BYTE, PV(&config_boot_part_opts), 1, 0},
  {(const char*)NULL,  (const char*)NULL, 0,  NULL, 0, 0},
};



#define SIZEOF(name, type) (sizeof(name)/sizeof(type))

// The config block container
static struct _cfg* g_cfg;
static struct _cfg_vals* g_vals;
static struct _cfg_vals* g_last_written_vals;

static int g_config_changed;

#define CONFIG_INUSE  (sizeof(struct _cfg_vals))
#define CONFIG_PADDING  (sizeof(g_cfg->buffer) - CONFIG_INUSE)


// find out if config contains a valid block
int config_verify()
{
  md5_byte_t digest[16];
  md5_state_t pms;

  md5_init(&pms);
  md5_append(&pms, g_cfg->buffer, sizeof(g_cfg->buffer));
  md5_finish(&pms, digest);

  if (0==memcmp((void*)digest, (void*)g_cfg->digest, sizeof(digest)))
  {
    return 1;
  }
  return 0;
}


// update the digest of the block
void config_update_md5()
{
  md5_byte_t digest[16];
  md5_state_t pms;

  md5_init(&pms);
  md5_append(&pms, g_cfg->buffer, sizeof(g_cfg->buffer));
  md5_finish(&pms, digest);

  memcpy((void*)g_cfg->digest, (void*)digest, sizeof(g_cfg->digest));
}


// Sizes of different types
u32 config_type_size(u8 cfg_type)
{
  switch(cfg_type)
  {
    case CONFIG_TYPE_ENUM:
    case CONFIG_TYPE_BYTE:
      return 1;
    case CONFIG_TYPE_STRING:
      return 1024;
    case CONFIG_TYPE_MEMADDR:
      return sizeof(u32);
    case CONFIG_TYPE_SECTOR:
      return sizeof(u16);
  }
  return 0;
}


// Iterate the configuration values
static _cfg_value_t* g_current_value;
static u32 g_current_offset;
static const u32 g_v2_offset = (u32)(&((struct _cfg_vals *)NULL)->upgrade_available);

void config_iter_start()
{
  g_current_offset = 4;
  g_current_value = config_values;
}

_cfg_value_t* config_iter_next()
{
  if (g_vals->version == 1 && g_current_offset >= g_v2_offset)
    return (_cfg_value_t*)NULL;

  _cfg_value_t* tmp = g_current_value;
  g_current_value++;
  if (tmp->name)
  {
    tmp->offset = g_current_offset;
    g_current_offset += config_type_size(tmp->type);
    return tmp;
  }
  return (_cfg_value_t*)NULL;
}


void config_set_default(_cfg_value_t* val)
{
  u8* ptr = (u8*)g_vals;   // start of config block.
  ptr += val->offset;   // find location in memory of value
  switch (val->type)
  {
    case CONFIG_TYPE_ENUM:
    case CONFIG_TYPE_BYTE:
      *ptr = val->initial & 0xff;
      return;
    case CONFIG_TYPE_STRING:
      memset(ptr, 0xff, 1024);
      strcpy((char*)ptr, (char*)val->initial);
      return;
    case CONFIG_TYPE_MEMADDR:
      *((u32*)ptr) = val->initial;
      return;
    case CONFIG_TYPE_SECTOR:
      *((u16*)ptr) = val->initial & 0xffff;
      return;
  }
}


void config_setdefaults(const char*)
{
  _cfg_value_t* val;

  g_vals->version = 2;

  config_iter_start();
  while ((val = config_iter_next()))
    config_set_default(val);
  printf("Config values reset to defaults\n");
}




static void config_kernelmax_check(int bb)
{
  const char* err = "Invalid kernelmax value, defaulting to 0x10\n";
  // Check range of kernelmax, it makes no sense to set it outside flash range
  if (g_vals->kernelmax>KERNELMAX_MAX)
  {
    bb ? print_biffboot(err) : printf(err);
    g_vals->kernelmax = FLASHMAP_KERNEL_SIZE/0x10000;
  }
}


void config_adjust_for_flashmap()
{
  // Sort out defaults that depend on the flash detection outcome.  
  // Assume this runs after flash detection!
  config_kernelmax_opts.max = (FLASHMAP_SIZE-FLASHMAP_BOOT_SIZE)/0x10000; 
    
  // Update these values
  KERNELMAX_MAX = FLASHMAP_SIZE/0x10000 - 1;  
  
  _cfg_value_t* val;
  config_iter_start();
  while ((val = config_iter_next()))
    if (strcmp(val->name,"kernelmax")==0)
    {
      val->initial = FLASHMAP_KERNEL_SIZE/0x10000;
    }
}

static void config_adjust_cmndline(void)
{
  // Ensure cmndline terminated properly in case someone wrote garbage
  u32 i;
  for (i=0;i<sizeof(g_vals->cmndline);i++)
  {
    if (!(g_vals->cmndline[i]))
    {
      break;
    }
  }
  if ((i+1)==sizeof(g_vals->cmndline))
  {
    print_biffboot("cmndline found without termination, adding it\n");
    g_vals->cmndline[i] = 0;
  }
  if (g_vals->version >= 2) {
    char* ptr = g_vals->cmndline;
    while (*ptr != '\0') {
      if (!strncmp(ptr, "root=", 5))
        break;
      ptr++;
    }
    if (ptr != '\0') {
      char rest[sizeof(g_vals->cmndline)];
      char *partition;
      const char *end = &g_vals->cmndline[sizeof(g_vals->cmndline) - 1];

      // find partition
      while ((*ptr < '1' || *ptr > '9') && *ptr != ' ' && *ptr != '\0')
        ptr++;
      partition = ptr;
      // find end of root= argument
      while (*ptr != ' ' && *ptr != '\0')
        ptr++;
      if (partition + (end - ptr) + 1 > end) {
        print_biffboot("Adjusting cmndline would write out of bounds!\n");
        return;
      }
      strcpy(rest, ptr);
      // seek back to partition
      ptr = partition;
      // append boot_part to root argument
      *ptr++ = '0' + g_vals->boot_part;
      *ptr = '\0'; // safety precaution
      strcpy(ptr, rest);
    }
  }
}

// initialise config system, loading config from flash.
void config_init()
{
  config_adjust_for_flashmap();

  // create a buffer to put config in.
  g_cfg = (struct _cfg*)malloc(sizeof(struct _cfg));
  g_vals = (struct _cfg_vals*)g_cfg;
  g_last_written_vals = (struct _cfg_vals*)malloc(sizeof(struct _cfg_vals));
  // copy the config from flash to mainram for any alterations
  memcpy((char*)g_cfg, 
          (char*)(FLASHMAP_BASE + FLASHMAP_CONFIG), 
          sizeof(struct _cfg));
  
  if (config_verify())
  {
    // config OK.
  }
  else 
  {
    print_biffboot("Config block not present, using defaults\n");
    config_setdefaults("");
  }
  if (g_vals->loadaddress > LOAD_ADDR_MAX)
  {
    g_vals->loadaddress = LOAD_ADDR_DEF;
  }
  if (g_vals->button == 0xff)  // flash not written to
  {
    // enable on upgrade
    g_vals->button = 1;
  }

  config_adjust_cmndline();
  config_kernelmax_check(1);
  
  // take backup for last written comparison
  memcpy((char*)g_last_written_vals, (char*)g_vals, sizeof(struct _cfg_vals));
  g_config_changed = 0;
}


void config_set_metadata()
{
  _cfg_value_t* val;
  char* ptr = (char*)g_cfg + 0x1000;

  config_iter_start();
  while ((val = config_iter_next()))
  {
    // Write out the type
    stream_write_char(&ptr, val->type & 0xff);
    // Write out the name
    stream_write(&ptr, val->name);
    // Write out the description
    stream_write(&ptr, val->description);
    
    if (val->type == CONFIG_TYPE_ENUM)
    {
      // write enum data, each option in turn.
      _cfg_opt_enum_t* opts = (_cfg_opt_enum_t*)val->typeinfo;

      while (opts->name)
      {
        stream_write(&ptr, opts->name);
        opts++;    
      }
    }
    // Double-null indicates end of metadata for this type.
    stream_write_char(&ptr, 0);
  }
}


void config_store(const char* arg)
{
  char* tmp = (char*)g_cfg;
  
  // ensure unused blocks are 0xff, to save flash writes
  memset(tmp + CONFIG_INUSE, 0xff, CONFIG_PADDING);

  // Write some metadata to the midpoint
  config_set_metadata();
  
  // update the stored md5.
  config_update_md5();
  
  // Write the block to the flash area
  EON_config_erase();

  EON_ProgramRange((unsigned short*)g_cfg, FLASHMAP_CONFIG, sizeof(struct _cfg)/2);
  EON_VerifyRange((unsigned short*)g_cfg, FLASHMAP_CONFIG, sizeof(struct _cfg)/2);
  
  memcpy((char*)g_last_written_vals, (char*)g_vals, sizeof(struct _cfg_vals));
  g_config_changed = 0;
  printf("Config written\n");
}


void config_revert(const char* arg)
{
  if (!g_config_changed)
  {
    printf("Nothing to revert\n");
    return;
  }
  memcpy((char*)g_vals, (char*)g_last_written_vals, sizeof(struct _cfg_vals));
}


void config_enum_dump(_cfg_opt_enum_t* opts, u8 now)
{
  while (opts->name)
  {
    if (opts->val == now)
    {
      printf(opts->name);
      return;
    }
    opts++;    
  }
  printf("Invalid");
}




// Dump a config value
void config_show_value(_cfg_value_t* val, int pad)
{
  u8* ptr = (u8*)g_vals;   // start of config block.
  u32 spaces = 0;
  ptr += val->offset;   // find location in memory of value
  spaces = 11-strlen(val->name);
  printf(val->name);
  printf(": ");
  if (pad)
  {
    while (spaces--) printf(" ");
  }
  switch (val->type)
  {
    case CONFIG_TYPE_ENUM:
      config_enum_dump((_cfg_opt_enum_t*)val->typeinfo, *ptr);
      break;
    case CONFIG_TYPE_BYTE:
      printf("%d", *ptr & 0xff);
      break;
    case CONFIG_TYPE_STRING:
      printf((char*)ptr);
      break;
    case CONFIG_TYPE_MEMADDR:
      printf("0x%x", *((u32*)ptr));
      break;
    case CONFIG_TYPE_SECTOR:
      printf("0x%04x", *((u16*)ptr));
      break;
  }
  printf("\n");
}


// Print out all the config values
void config_dump(const char* arg)
{
  _cfg_value_t* val;

  printf("Configuration values:\n");
  printf("version:     %d\n", g_vals->version);
  config_iter_start();
  while ((val = config_iter_next()))
    config_show_value(val, 1);
}


void config_enum_choose(_cfg_opt_enum_t* opts, u8* val)
{
  _cfg_opt_enum_t* tmp = opts;
  int count=0;
  char key=0;
  printf("Options:\n");
  while (tmp->name)
  {
    printf("  ");
    putchar(tmp->val+48);
    printf(" - ");
    printf(tmp->name);
    printf("\n");
    count++;
    tmp++;
  }
  printf("Enter choice, <ESC> to abort: ");
  while (!key)
  {
    key = getkey(0);
    if (key==27)
    {
       printf(" Aborted!\n");
       return;
    }
    if (key<48)
    { 
      key = 0;
      continue;
    }
    key -= 48;
    if (key<count) break;
    key = 0;
  }
  *val = key;
  putchar(key+48);
  printf("\n");
}


void config_enum_set(_cfg_value_t* val)
{
  _cfg_opt_enum_t* tmp;
  u8* now = (u8*)g_vals + val->offset;
  tmp = (_cfg_opt_enum_t*)val->typeinfo;
  config_show_value(val, 0);
  config_enum_choose(tmp, now);
  config_show_value(val, 0);
}



static char g_tmp_cmnd[sizeof(g_vals->cmndline)];

// Capture a new string value
void config_string_set(_cfg_value_t* val)
{  
  char* dest;
  char out;
  config_show_value(val, 0);
  printf("Enter new value, <ENTER> when done, <ESC> to abort\n");
  out = get_ascii_data(g_tmp_cmnd, sizeof(g_vals->cmndline), 0, 0, 0, NULL_COMPLETE);
  if (out ==27) {
    printf("\r\nAborted!");
  } else {
    // minimise number of writes, by leaving 0xff
    dest = ((char*)g_vals + val->offset);
    memset(dest, 0xff, 1024);
    strcpy(dest, g_tmp_cmnd);
  }
  printf("\n");
  config_show_value(val, 0);
}


void config_hex_set(_cfg_value_t* val, int bits, u32 maxval)
{
  u8* dest;
  u32 tmp;
  char out;
  char hexbuffer[9];
  config_show_value(val, 0);
  printf("Enter hex value (max ");
  printf( (bits==16) ? "%04x" : "%x",  maxval);
  printf("), <ENTER> when done, <ESC> to abort\n");
  out = get_ascii_data(hexbuffer, bits/4 + 1, 0, 1, 0, NULL_COMPLETE);
  if (out == 27) {
    printf("\r\nAborted!");
  } else {
    dest = (u8*)g_vals + val->offset;
    tmp = hex_to_u32(hexbuffer, strlen(hexbuffer));
    if (tmp <= maxval)
    {
      if (bits==16) {
        *(u16*)dest = tmp & 0xffff;
      } else {
        *(u32*)dest = tmp;    
      }
    } else {
      printf("\r\nValue too large, leaving unchanged");
    }
  }
  printf("\n");
  config_show_value(val, 0);
}


// Type-safe accessor functions 

int config_console_get()
{
  return g_vals->console;
}

int config_bootsource_get()
{
  return g_vals->bootsource;
}

int config_boottype_get()
{
  return g_vals->boottype;
}

int config_nic_get()
{
  return g_vals->nic;
}

u32 config_loadaddress_get()
{
  return g_vals->loadaddress;
}

char* config_cmndline_get()
{
  return g_vals->cmndline;
}


u32 config_kernelmax_get(void)
{
  return g_vals->kernelmax * 0x10000;
}

int config_button_get()
{
  return g_vals->button;
}

int config_upgrade_available_get(void)
{
  return g_vals->version == 1 ? 0 : g_vals->upgrade_available;
}

int config_bootcount_get(void)
{
  return g_vals->version == 1 ? 0 : g_vals->bootcount;
}

int config_boot_part_get(void)
{
  return g_vals->version == 1 ? -1 : g_vals->boot_part;
}


// Set an individual config value
void config_set_value(const char* arg)
{
  const char* ptr;
  _cfg_value_t* val;
  u32 spaces;

  ptr = eat_token(arg, "help");
  if (ptr)
  {
    printf("Available config options:\n");
    printf("defaults\n");
    printf("OR\n");
    config_iter_start();
    while ((val = config_iter_next()))
    {
      printf(val->name);
      spaces = 12 - strlen(val->name);
      while (spaces--) printf(" ");
      printf(val->description);
      printf("\n");
    }
    return;
  }

  ptr = eat_token(arg, "defaults");
  if (ptr)
  {
    config_setdefaults("");
    return;
  }

  config_iter_start();
  while ((val = config_iter_next()))
  {
    ptr = eat_token(arg, val->name);
    if (ptr)
    {
      switch (val->type)
      {
        case CONFIG_TYPE_ENUM:
          config_enum_set(val);
          return;
        case CONFIG_TYPE_STRING:
          config_string_set(val);
          return;
        case CONFIG_TYPE_MEMADDR:
          config_hex_set(val, 32, 0x1ffffff);
          return;
        case CONFIG_TYPE_SECTOR:
          config_hex_set(val, 16, KERNELMAX_MAX);
          return;
        case CONFIG_TYPE_BYTE:
          config_hex_set(val, 8, 0xff);
          return;
      }
      break;
    }
  }
  printf("Required option missing, type 'set help' for options\n");

}


const char* config_complete(const char* partial)
{
u32 count=0;
_cfg_value_t* val;
_cfg_value_t* val_out;

  config_iter_start();
  while ((val = config_iter_next()))
  {    if (strncmp(partial, val->name, strlen(partial))==0)
    {
      count++;
      val_out = val;
    }
  }
  if (count==1)
  {
    return val_out->name;
  }
  return NULL_STRING;
}


// return whether the config changed since the boot
// and needs to be saved
int config_get_changed()
{
  if (memcmp((char*)g_last_written_vals, (char*)g_vals, sizeof(struct _cfg_vals)))
  {
    if (!g_config_changed)
    {
      printf("Config has changed, 'save' to make permanent, 'revert' to go back\n");
    }
    g_config_changed = 1;
  }
  else
  {
    if (g_config_changed)
    {
      printf("Config reverted, 'save' no longer needed\n");
    }
    g_config_changed = 0;
  }
  return g_config_changed;
}


