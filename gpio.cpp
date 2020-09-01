
#include "io.h"
#include <string.h>
#include <stdio.h>
#include "unistd.h"
#include "gpio.h"
#include "r6040.h"
#include "stdlib.h"
#include "iolib.h"

#define RDC_CONTROL 0x80003848
#define RDC_DATA    0x8000384c


//#define GPIO0	1 << 11,
//	1 << 13,
//	1 << 9,
//	1 << 12,


static unsigned long g_last_value = 0xffffffff;



static inline void set_control(unsigned long mask)
{
	unsigned long tmp;
	outl(RDC_CONTROL, 0xcf8);   // Select control register.
	tmp = inl(0xcfc);         
	tmp |= mask;                // raise to set GPIO function
	outl(tmp, 0xcfc);
}

static inline void set_data(unsigned long mask)
{
	outl(RDC_DATA, 0xcf8);   // Select data register.
	g_last_value |= mask;        
	outl(g_last_value, 0xcfc);
}

static inline void clear_data(unsigned long mask)
{
	outl(RDC_DATA, 0xcf8);   // Select data register.
	g_last_value &= ~mask;               
	outl(g_last_value, 0xcfc);
}


int gpio_get_value(unsigned long mask)
{
	unsigned long tmp;
	outl(RDC_DATA, 0xcf8);   // Select data register.
	tmp = inl(0xcfc);         
	return (tmp & mask)?1:0; 
}


void gpio_input(unsigned long mask)
{	
	set_data(mask);      // raise logic level to turn off pull-down.
	set_control(mask);   // select as GPIO function.
}


void gpio_output(unsigned long mask, int value)
{		
	if (value) {
		set_data(mask);
	} else {
		clear_data(mask);
	}
	set_control(mask);
}


void gpio_set_value(unsigned long mask, int value)
{
	if (value) {
		set_data(mask);
	} else {
		clear_data(mask);
	}	
}


#define OP_READ 0
#define OP_WRITE_HIGH 1
#define OP_WRITE_LOW 2
#define OP_DIRN_IN 3
#define OP_DIRN_OUT_HIGH 4
#define OP_DIRN_OUT_LOW 5


typedef struct _gpio_proxy
{ 
  unsigned long operation;  // see above
  unsigned long mask;      // gpio affected
  unsigned int result;   // only if needed
} gpio_proxy_t;


unsigned long gpio_proxy(unsigned char* buffer, size_t buffer_length)
{
  gpio_proxy_t* pkt = (gpio_proxy_t*)buffer;

  pkt->result = 0;
  switch (pkt->operation)
  {
    case OP_READ :
      gpio_input(pkt->mask);
      pkt->result = gpio_get_value(pkt->mask);
      break;
    case OP_WRITE_HIGH :
      gpio_set_value(pkt->mask, 1);
      break;
    case OP_WRITE_LOW :
      gpio_set_value(pkt->mask, 0);
      break;
    case OP_DIRN_IN :
      gpio_input(pkt->mask);
      break;
    case OP_DIRN_OUT_HIGH :
      gpio_output(pkt->mask, 1);
      break;
    case OP_DIRN_OUT_LOW :
      gpio_output(pkt->mask, 0);
      break;
  }  
  return sizeof(gpio_proxy_t);
}




static u32 read_all()
{
#define ALL_LA_PINS (GPIO_JTAG2|GPIO_JTAG3|GPIO_JTAG4|GPIO_JTAG5|GPIO_BUTTON)
  outl(RDC_DATA, 0xcf8);   // Select data register for future operations
  return inl(0xcfc) & ALL_LA_PINS;
}



// Sample LEDs in a loop.
void gpio_logic_analyser(const char* arg)
{
  unsigned long tmp, prev, count;
  printf("Starting logic analyser on JTAG2-5 and button input, power off to reboot!\n");
  gpio_input(GPIO_JTAG2);
  gpio_input(GPIO_JTAG3);
  gpio_input(GPIO_JTAG4);
  gpio_input(GPIO_JTAG5);
  gpio_input(GPIO_BUTTON);
    
  tmp = prev = read_all();
  count = 0;
  while (1) {
    tmp = read_all();
    if (tmp != prev)
    {
      printf("%x",unsigned(count));
      printf("-");
      printf((tmp & GPIO_JTAG2)? "1":"0");
      printf((tmp & GPIO_JTAG3)? "1":"0");
      printf((tmp & GPIO_JTAG4)? "1":"0");
      printf((tmp & GPIO_JTAG5)? "1":"0");
      printf((tmp & GPIO_BUTTON)? "1":"0");
      printf("\n");
      count++;
      prev = tmp;
    }
  }
}

