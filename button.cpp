
#include "types.h"
#include "timer.h"
#include "button.h"
#include "unistd.h"
#include "gpio.h"
#include "config.h"
#include "iolib.h"

static int g_initialised=0;

static void button_init()
{
  if (config_button_get()==CONFIG_BUTTON_ON)
    gpio_input(GPIO_BUTTON);
  g_initialised = 1;
}


// Sample button state
int button_now()
{
  if (!g_initialised)
  {
    button_init();
  }  

  switch (config_button_get())
  {
    case CONFIG_BUTTON_FORCEDOFF:
      return BUTTON_NOT_PRESSED;
    case CONFIG_BUTTON_ON:
      timer_udelay(10);
      return gpio_get_value(GPIO_BUTTON);
    case CONFIG_BUTTON_FORCEDON:
      return BUTTON_PRESSED;
    default:
      return BUTTON_NOT_PRESSED;
  }    
}


