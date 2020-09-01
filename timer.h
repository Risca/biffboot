#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

#include "iolib.h"

#define TIMER_10MS 0xd555
#define TIMER_1MS 0xfbbb

// reset timer to known value
void timer_set();
// return true if given interval has expired
// only good for a few mS.
int timer_expired(unsigned short compare);

unsigned short timer_read();

void timer_mdelay10();
void timer_mdelay1();
void timer_sdelay1();
void timer_udelay(unsigned int usecs);

// The value we send to the PIT is the value to divide it's input clock
// (1193180 Hz) by, to get our required frequency. Important to note is
// that the divisor must be small enough to fit into 16-bits.
// u32 divisor = 1041816 / frequency;
#define TIMER_DIVISOR  (1041816/50)  // 50 Hz
#define TIMER_DIV_LOW  ((u8)(TIMER_DIVISOR & 0xFF))
#define TIMER_DIV_HIGH ((u8)((TIMER_DIVISOR>>8) & 0xFF ))


u32 timer_tick();
// Initialise the timer functionality, so we have interrupts.
void timer_init();

#ifdef __cplusplus
}
#endif
