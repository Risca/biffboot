#pragma once
#ifdef __cplusplus
extern "C" {
#endif 


#include "types.h"

#define GPIO_JTAG2  (1 << 11)
#define GPIO_JTAG3  (1 << 13)
#define GPIO_JTAG4  (1 <<  9)
#define GPIO_JTAG5  (1 << 12)

#define GPIO_BUTTON (1 << 15)     // button
#define GPIO_LED    (1 << 16)      // RED LED

// Set port directions for LED and button.
void gpio_init();

// Set red LED on/off
void gpio_led_on();
void gpio_led_off();

void gpio_input(unsigned long mask);
void gpio_output(unsigned long mask, int value);
int gpio_get_value(unsigned long mask);
void gpio_set_value(unsigned long mask, int value);

unsigned long gpio_proxy(unsigned char* buffer, size_t buffer_length);

void gpio_logic_analyser(const char* arg);

#ifdef __cplusplus
}
#endif
