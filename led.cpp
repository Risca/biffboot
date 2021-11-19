
#include "led.h"
#include "unistd.h"
#include "gpio.h"
#include "iolib.h"
#include <string.h>
#include <stdio.h>


// Set red LED on/off
void led_on()
{
  gpio_output(GPIO_LED, 0);
}

void led_off()
{
  gpio_output(GPIO_LED, 1);
}


typedef struct _entry
{
  const char* name;
  int value;
} _entry_t; 


static struct _entry g_entries[] = 
{
  {"on", 0},
  {"off", 1},
  {NULL_STRING, 0}
};


void led_onoff(const char* arg)
{
_entry_t* ptr = g_entries;
const char* out;

  while (ptr->name)
  {
    if ((out = eat_token(arg, ptr->name)))
    {
      gpio_output(GPIO_LED, ptr->value);
      return;
    }
    ptr++;
  }    
  printf("led: Unrecognised option ('on' or 'off')\n");
}


const char* led_complete(const char* partial)
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
  return NULL_STRING;
}
