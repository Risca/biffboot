
#include "types.h"
#include "io.h"
#include <string.h>
#include <stdio.h>
#include "stdlib.h"
#include "iolib.h"
#include "flash.h"
#include "flashmap.h"
#include "md5.h"
#include "bootlinux.h"
#include "bootmulti.h"
#include "bootcoreboot.h"
#include "r6040.h"
#include "config.h"
#include "timer.h"
#include "netconsole.h"
#include "oldconsole.h"
#include "loader.h"
#include "assem.h"
#include "gpio.h"
#include "led.h"
#include "version.h"
#include "history.h"
#include "ubifs.h"
#include "button.h"
#include "udpconsole.h"
#include "ohci.h"
#include "ether.h"


void boot_linux();
void print_pci_reg(u32 n);
void print_io_reg(u16 n);
void print_io_reg8(u16 n);
void set_pci_reg(u32 addr, u32 data);
void uart_init();
void misc_init();
void toggle_led();
void logic_analyser();
void reboot_system(const char* arg);
void boot_now(const char* arg);
const char* match_command(const char* partial);


typedef struct cmnd
{
const char* name;
void (*function)(const char*);
const char* (*complete)(const char*);
const char* description;
} cmnd_t;

void dispatch_command(const char* cmnd);


// Run the UART console
static void uart_console(u32 key)
{
  u32 first=1;
  u32 init_key=0;
  char cmnd_buffer[60];
    serial_enable(1);   // enable the console
    history_init();
    printf("'help' or '?' for a list of commands\n");

    while (1)   // loop forever
    {
      prompt();

      if (first)
      {
        // wait 250mS for a request to erase flash.
        init_key = getkey(25);
    
        if (init_key=='1')
        {
          oldconsole_run();
          break;  // continue with boot, 4 quits.
        }
        first = 0;  // don't call again
      }

      key = get_ascii_data(cmnd_buffer, sizeof(cmnd_buffer), init_key, 0, 1, match_command);
      init_key = 0;
      dispatch_command(cmnd_buffer);
    }
}


static bool DETECTED_QEMU = false;


// Really simply hack to detect if we're running under Qemu, if so extra init required.
void detect_qemu()
{
  outl(0x80000800, 0xcf8);
  if (inl(0xcfc) == 0x70008086)
    DETECTED_QEMU = true;  // found intel 
  else
    DETECTED_QEMU = false;
}



// main() has to be at the top of this file.
// definitions are OK before, but not function bodies, as they change
// the entry point.

void version(const char* arg);


extern "C" int main()
{
  int button_state=BUTTON_NOT_PRESSED;   
  u32 key=0;
  int t_linkup=0;
  int t_wait_for_link=0;

  detect_qemu(); 
  

  flashmap_init();

  // Force LED off, in case the operating system switched it on.
  led_off();
  uart_init();
  misc_init();
  config_init();
  
  // Disable echoing to output if it was requested.
  serial_enable(config_console_get());
  
  // Check the button state
  button_state = button_now();
  r6040_set_mac();
  
  printf("\r\n");
  version("");
    
  printf("Press <ESC>\n");


  ohci_init();

  if (DETECTED_QEMU)
  {
    // Use PIC to give time delay
    key = getkey(200);
    t_linkup = 0;
  }
  else
  {

    // Wait up to 3 seconds for the PHY link to come up
    // Return how long we actually waited for the link
    t_linkup = r6040_wait_linkup(300, &t_wait_for_link);

    // allocate r6040 buffers
    r6040_init();    
    if (t_linkup)  // if link came up.
    {
      // Try to snatch a break-in packet.
      printf("Link up\n");
      if (config_nic_get())
      {
        printf("Checking NIC\n");
        netconsole_check();
      }
    }
    
    // Only try to get a key if the net console failed to run for some reason.
    if (netconsole_ran())
    {
      key = 0;   // just boot
    } else {
      if (t_wait_for_link>200)
      {
        // Already waited enough time, don't wait any more
        key = getkey(0);
      } else {
        // Need to give the user more time to press the key.
        key = getkey( 200 - t_wait_for_link);
      }
    }
  }



  // if button pressed and console disabled, the serial console is activated differently.
  if ((config_button_get()==CONFIG_BUTTON_ON) && config_console_get() == 0)
  {
    int now = button_now();
    switch (now)
    {
       case BUTTON_PRESSED:
         key = 27;
         break;
       case BUTTON_NOT_PRESSED:
         key = 0;
         break;
    }
  }

  
  if (key==27)   // ESC key pressed
  {
    if (ether_mac_unset())
    {
      ether_entermac("");
    }
    uart_console(key);
  } 
  else
  {
    if (button_state == BUTTON_PRESSED)
      udpconsole_run();
  }

  asm volatile("cli");

  boot_now("");
  while(1) {};
  return 0;
}



void boot_now(const char* arg)
{
  u32 kernel_size;
  // Just in case flashing updated the configuration area, re-read
  // the config
  if (netconsole_ran())
  {
    printf("re-reading config\n");
    config_init();
  }

  printf("Booting...\n");
  

  // Load some code
  kernel_size = loader_load();
  if (!kernel_size)
  {
    printf("Invalid kernel size\n");
    reboot_system(NULL_STRING);
  }
  
  switch (config_boottype_get())
  {
    case CONFIG_BOOTTYPE_LINUX26:
      bootlinux_go(kernel_size);
      break;
    case CONFIG_BOOTTYPE_FLATBIN:
      assem_flatbin_boot(config_loadaddress_get());
      break;
    case CONFIG_BOOTTYPE_MULTI:
      bootmulti_go();
      break;
    case CONFIG_BOOTTYPE_COREBOOT:
      bootcoreboot_go();
      break;
  }
  // No idea what state the machine is in now, so just reboot.
  reboot_system(NULL_STRING);
}




static u32 read_pcicfg32(u32 addr)
{
  outl(0x80000000 + addr, 0xcf8);
  return inl(0xcfc);
}


void print_pci_reg(u32 addr)
{
  printf("PCI Address: %x Data: ", unsigned(addr));
  outl(0x80000000 + addr, 0xcf8);
  printf("%x\n", inl(0xcfc));
}

void print_io_reg(u16 addr)
{
  printf("IO Address: %04x Data: %04x\n",unsigned(addr), inw(addr));
}

void print_io_reg8(u16 addr)
{
  printf("IO Address: %04x Data: %02x\n",unsigned(addr), inb(addr));
}

void pcicfg32(u32 addr, u32 data)
{
  outl(0x80000000 & addr, 0xcf8);
  outl(data, 0xcfc);
}

void pcicfg16(u32 addr, u16 data)
{
  outl(0x80000000 & addr, 0xcf8);
  outw(data, 0xcfc);
}


void uart_init()
{
  pcicfg32( 0x3854, 0x000003f8);	// COM1 address

  // Set DLAB (bit7) to get to divisor regs.
  outb(0x83, 0x3fb);
  outb(0x01, 0x3f8);    // low byte 115200
  outb(0x00, 0x3f9);    // high byte 115200
  outb(0x03, 0x3fb);    // unset bit7 (DLAB)
  outb(0x07, 0x3fa);    // enable and clear fifos
}


static void qemu_init()
{
outl(0x80000058, 0xcf8);
outb(0x33, 0xcfe);
outl(0x80000058, 0xcf8);
outb(0x33, 0xcff);
outl(0x8000005c, 0xcf8);
outb(0x33, 0xcfc);
outl(0x8000005c, 0xcf8);
outb(0x33, 0xcfd);
outl(0x8000005c, 0xcf8);
outb(0x33, 0xcfe);
outl(0x8000005c, 0xcf8);
outb(0x33, 0xcff);
outl(0x80000058, 0xcf8);
outb(0x30, 0xcfd);


outl(0x80000010, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000010, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000014, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000014, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000018, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000018, 0xcf8);
outl(0x0, 0xcfc);
outl(0x8000001c, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x8000001c, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000020, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000020, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000024, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000024, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000030, 0xcf8);
outl(0xfffff800, 0xcfc);


outl(0x80000030, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000810, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000810, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000814, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000814, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000818, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000818, 0xcf8);
outl(0x0, 0xcfc);
outl(0x8000081c, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x8000081c, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000820, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000820, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000824, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000824, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000830, 0xcf8);
outl(0xfffff800, 0xcfc);
outl(0x80000830, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000910, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000910, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000914, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000914, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000918, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000918, 0xcf8);
outl(0x0, 0xcfc);
outl(0x8000091c, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x8000091c, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000920, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000920, 0xcf8);
outl(0x1, 0xcfc);
outl(0x80000924, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000924, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000930, 0xcf8);
outl(0xfffff800, 0xcfc);
outl(0x80000930, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000b10, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000b10, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000b14, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000b14, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000b18, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000b18, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000b1c, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000b1c, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000b20, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000b20, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000b24, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80000b24, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000b30, 0xcf8);
outl(0xfffff800, 0xcfc);
outl(0x80000b30, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80004010, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80004010, 0xcf8);
outl(0x1, 0xcfc);
outl(0x80004014, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80004014, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80004018, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80004018, 0xcf8);
outl(0x0, 0xcfc);
outl(0x8000401c, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x8000401c, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80004020, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80004020, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80004024, 0xcf8);
outl(0xffffffff, 0xcfc);
outl(0x80004024, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80004030, 0xcf8);
outl(0xfffff800, 0xcfc);
outl(0x80004030, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000920, 0xcf8);
outl(0x100, 0xcfc);
outl(0x80004010, 0xcf8);
outl(0x0, 0xcfc);

outl(0x80000004, 0xcf8);
outw(0x3, 0xcfc);

outl(0x80000804, 0xcf8);
outw(0x3, 0xcfc);
outl(0x80000940, 0xcf8);
outw(0x8000, 0xcfc);
outl(0x80000940, 0xcf8);
outw(0x8000, 0xcfe);
outl(0x80000904, 0xcf8);
outw(0x3, 0xcfc);
outl(0x80000b04, 0xcf8);
outw(0x3, 0xcfc);
outl(0x80000b3c, 0xcf8);
outb(0xa, 0xcfc);
outl(0x80000b3c, 0xcf8);
outb(0x9, 0xcfc);
outl(0x80000b40, 0xcf8);
outl(0xb001, 0xcfc);
outl(0x80000b80, 0xcf8);
outb(0x1, 0xcfc);
outl(0x80000b90, 0xcf8);
outl(0xb101, 0xcfc);
outl(0x80000bd0, 0xcf8);
outb(0x9, 0xcfe);


outl(0x80004004, 0xcf8);
outw(0x3, 0xcfc);


outl(0x8000403c, 0xcf8);
outb(0xb, 0xcfc);


outl(0x80000860, 0xcf8);
outb(0xb, 0xcfc);  //<------ PIRQA

outl(0x80000860, 0xcf8);
outb(0xb, 0xcfd);  //<------ PIRQB

outl(0x80000860, 0xcf8);
outb(0xb, 0xcfe);  //<--------- r6040
		   //<------ PIRQC

outl(0x80000860, 0xcf8);
outb(0xb, 0xcff);  //<------ PIRQD


outl(0x80000030, 0xcf8);
outl(0xfffffffe, 0xcfc);
outl(0x80000030, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000830, 0xcf8);
outl(0xfffffffe, 0xcfc);
outl(0x80000830, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000930, 0xcf8);
outl(0xfffffffe, 0xcfc);
outl(0x80000930, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80000b30, 0xcf8);
outl(0xfffffffe, 0xcfc);
outl(0x80000b30, 0xcf8);
outl(0x0, 0xcfc);
outl(0x80004030, 0xcf8);
outl(0xfffffffe, 0xcfc);
outl(0x80004030, 0xcf8);
outl(0x0, 0xcfc);

}


static u32 PCIReadConfig(u32 bus, u32 device, u32 function, u32 offset)
{
  u32 addr = bus << 16  |  device << 11  |  function <<  8  |  offset;
  return read_pcicfg32(addr);
}


static void PCIWriteConfig(u32 bus, u32 device, u32 function, u32 offset, u32 val)
{
  u32 addr = bus << 16  |  device << 11  |  function <<  8  |  offset;
  pcicfg32(val, addr);
}





void PCIInit(u32 pv, u32 bus, u32 device, u32 fun)
{
  //printf("PCI device: %d.%d.%d  %x\n", bus, device, fun, pv);
  //u32 tmp = 0;
  switch (pv)
  {
    case 0x602017f3:  // Northbridge
      break;
    case 0x003f106B:  // Apple USB controller
      PCIWriteConfig(bus, device, fun, 0x3c, 0x000b);
      break;
    case 0x604017f3:  // R6040 ethernet
      PCIWriteConfig(bus, device, fun, 0x3c, 0x000b);
      break;

  }
}





void misc_init()
{

  if (DETECTED_QEMU)
  {
       qemu_init();
  }

	u32 bus = 0;
	// Enumerate PCI bus
	for (u32 device=0; device<32; device++)
	{
	   u32 pv = PCIReadConfig(bus, device, 0, 0);
	   if (pv != 0xffffffff)
	   {
	     for (u32 fun=0; fun<8; fun++)
	     {
	       pv = PCIReadConfig(bus, device, fun, 0);
	       if (pv != 0xffffffff)
	       {
		 PCIInit(pv, bus, device, fun);
	       }
	     }
	   }
	}

	// Init GPCS
	pcicfg32(	0x3890, 0x00200002);
	pcicfg32(	0x3898, 0x00300002);
}



#define WDT_TIMEOUT 1

/* Also doubles as 'enable' */
#define WDT_RESET (1<<23)|(1<<20)|(1<<19)|(1<<18)|WDT_TIMEOUT


// Reboot the system
void reboot_system(const char* arg)
{
  unsigned i;
  printf("Rebooting...\n"); 
  outl(0x80003840, 0xCF8);
  i = inl(0xCFC);
  i |= 0x1600;
  outl(i, 0xCFC);

  outl(0x80003844, 0xCF8);
  outl(WDT_RESET, 0xCFC);
  
//  outb(1, 0x92);  
}



// flash via tftp.
static void tftp_command(const char* arg)
{
  u32 src = 0x400000;
  u32 chunk = 0;
  u32 dest = 0;
  // get kernel to 0x400000, which has 8MB available.
  u32 len = loader_copy_from_network(src);
  
  if (len > (FLASHMAP_SIZE - FLASHMAP_BOOT_SIZE))
  {
    printf("Downloaded image is %d bytes with only %d bytes available in flash, image too large!\n",
      len, (FLASHMAP_SIZE - FLASHMAP_BOOT_SIZE));
    return;
  }
  
  
  printf("Next step will overwrite your existing kernel/rootfs,");
  if (!are_you_sure()) return;
  
  // flash from 0x400000 to the on-board flash.
  while (len)
  {
    dest = EON_ChunkToSector(chunk)*FLASHMAP_CHUNK_SIZE;
    if (!chunk)
    {
      printf("Writing: 0x%x", unsigned(dest));
    }
    if (flash_write_chunk((u16*)src, dest, FLASHMAP_CHUNK_SIZE))
    {
      printf("\r\nError writing to flash.");
      break;
    }
    else
    {
      print_erase(8);
      printf("%x", unsigned(dest));
    }
    
    chunk ++;
    src += FLASHMAP_CHUNK_SIZE;
    if (src >= (0xc00000-0x10000)) break;
    if (len<=FLASHMAP_CHUNK_SIZE) {
      len = 0;
    } else {
      len -= FLASHMAP_CHUNK_SIZE;
    }
  }
  printf("\n");
}


void help(const char* arg);

void credits(const char* arg)
{
  printf("Credits:\n");
  printf("\tAndy 'Lurch' Scheller and Ernst J. Oud for Beta testing\n");
  printf("\tThe Nasm developers, community and documentation\n");
  printf("\tSchufti at the Macsat forum\n");
}


void license(const char* arg)
{
  printf("\n This Bootloader is copyright (c) 2014 Bifferos.com.\n\n"
    " Permission is hereby granted, free of charge, to any person obtaining a copy\n"
    " of this Bootloader to deal in the Bootloader without restriction, including\n"
    " the rights to copy, publish, distribute sublicense, and/or sell copies of the\n"
    " Bootloader and to permit persons to whom the Bootloader is furnished to do so,\n"
    " subject to the following conditions:\n\n"
    
    " Modification of the Bootloader binary is not permitted, excepting the 12-byte\n"
    " configuration area found at offset 0xfff4.\n\n"
    
    " THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
    " IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
    " FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
    " AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\n"
    " WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN\n"
    " CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\n"
    
    " Contact sales@bifferos.com if custom versions are required.\n\n"
    );
}


void version(const char* arg)
{
  printf("BIFFBOOT ");
  printf(BIFFBOOT_VERSION);
  printf(" ");
  printf("Community Edition ");
    
  if (DETECTED_QEMU)
    printf("Qemu");
  else
    r6040_print_common_mac();

  printf(" 32-bit Loader by bifferos (c) 2013\n");
  printf("Redistribution prohibited, all rights reserved.\n");
}



static void fill_with_random(void* addr, size_t extent)
{
  unsigned int* start = (unsigned int*)addr;
  unsigned int remain = extent/4;
  unsigned int seed = 0xaaaaaaaa;
  
  while (remain)
  {
    remain--;
    start[remain] = seed;
    seed ^= 0xffffffff;
  }
}


static void memcheck_command(const char* arg)
{
  printf("Buffer strength control register I: 0x%x\n", PCIReadConfig(0, 0x7, 0, 0x5c));
  printf("Starting basic memory check (reset to abort)\n");

  const int extent = 0x800000;
  void* a = malloc(extent);
  void* b = malloc(extent);
  if ((!a) || (!b))
  {
    printf("Unable to allocate memory, rebooting\n");
    reboot_system(NULL_STRING);
  }
  printf("Allocated regions a=0x%x and b=0x%x\n", (unsigned int)a, (unsigned int)b);
  for (int i=0;i<10;i++)
  {
    printf("Pass %d...", i);
    fill_with_random(a, extent);
    memcpy(b, a, extent);
    if (0 == memcmp(a, b, extent))
    {
      printf("OK\n");
    }
    else
    {
      printf("FAILED!\n");
      return;
    }
    memset(a, 0, extent);
    memset(b, 0, extent);
    if (memcmp(a, b, extent))
    {
      printf("Failed to zero memory after test\n");
      return;
    }
  }
}





static struct cmnd g_commands[] = 
{
{"help", help, NULL_COMPLETE, "Print this help"},
{"?", help, NULL_COMPLETE, "Alias for 'help'"},
{"credits", credits, NULL_COMPLETE, "Print credits"},
{"license", license, NULL_COMPLETE, "Print license"},
{"version", version, NULL_COMPLETE, "Print version"},
{"analyser", gpio_logic_analyser, NULL_COMPLETE, "Run a simple logic analyser on JTAG/button GPIO"},
{"showconfig", config_dump, NULL_COMPLETE, "Print config values"},
{"flash", flash_command, flash_complete, "Flash erase or basic test (erase|test)"},
{"memcheck", memcheck_command, NULL_COMPLETE, "Basic memory test"},
{"tftpflash", tftp_command, NULL_COMPLETE, "Obtain and flash kernel via BOOTP/TFTP"},
//{"ubiformat", ubifs_command, NULL_COMPLETE, "Format ubifs partition"},
{"led", led_onoff, led_complete, "Set led state (on|off)"},
//{"phy", r6040_phy_print, NULL, "Print PHY registers"},
{"defaults", config_setdefaults, NULL_COMPLETE, "Set factory default configuration"},
{"revert", config_revert, NULL_COMPLETE, "Revert latest config changes"},
{"save", config_store, NULL_COMPLETE, "Save the config to flash"},
{"reboot", reboot_system, NULL_COMPLETE, "Restart Biffboot"},
{"go", boot_now, NULL_COMPLETE, "Boot kernel/payload"},
{"boot", boot_now, NULL_COMPLETE, "Alias for 'go'"},
{"set", config_set_value, config_complete, "Set config value. 'set help' for options"},
{"mac", ether_entermac, NULL_COMPLETE, "Set the MAC address"}
};



const char* match_command(const char* partial)
{
  u32 i;
  u32 count=0;
  const char* ptr;
  const char* tok;
  for (i=0; i<(sizeof(g_commands)/sizeof(cmnd_t)); i++)
  {
    if (strncmp(partial, g_commands[i].name, strlen(partial)) == 0)
    {
      count++;
      ptr = g_commands[i].name;
    }
    // check for full command match
    if (g_commands[i].complete)
    {
      tok = eat_token(partial, g_commands[i].name);
      if (tok)
      {
        tok = g_commands[i].complete(tok);
        if (tok) return tok;
      }
    }
  }
  if (count==1)
  {
    return ptr;
  }
  
  return (const char*)NULL;
}



void help(const char * arg)
{
  u32 i, len;
  printf("Available commands:\n");
  for (i=0; i< (sizeof(g_commands)/sizeof(cmnd_t)); i++)
  {
    printf(g_commands[i].name);
    len = 11 - strlen(g_commands[i].name);
    while (len--) printf(" ");
    printf(g_commands[i].description);
    printf("\n");
  }
}


// Call relevant function for command
void dispatch_command(const char* cmnd)
{
  u32 i;
  const char* ptr;
  printf("\n");
  // skip any preceding space
  for (i=0; i< (sizeof(g_commands)/sizeof(cmnd_t)); i++)
  {
    ptr = eat_token(cmnd, g_commands[i].name);
    if (ptr)
    {
      history_append(cmnd);
      g_commands[i].function(ptr);
      return;
    }
  }
  if (strlen(cmnd)!=0)
    printf("Invalid command\n");
}


