
#include "io.h"
#include "timer.h"
#include "isr.h"
#include "idt.h"

// Whether we're in the interrupt or other mode of operation.
static u32 timer_interrupting = 0;

#define PIT_MODE 0x43
#define PIT_CH0  0x40


// Non-interrupt mode of using the i8253 timer.

void timer_set()
{
  outb(0x34, PIT_MODE);
  outb(0xff, PIT_CH0);   // LSB
  outb(0xff, PIT_CH0);   // MSB
}

int timer_expired(unsigned short value)
{
  if (timer_read()<=value)
    return 1;
  return 0;
}

unsigned short timer_read()
{
  unsigned short out;
  // latch timer 2
  outb(0x04, PIT_MODE);
  // Read LSB
  out = inb(PIT_CH0);
  out |= (inb(PIT_CH0) << 8);
  return out;
}


// Interrupt-driven timer functions.  Will eventually move over to using
// only these.

static volatile u32 tick = 0;


u32 timer_tick()
{
  asm volatile("cli");
  u32 out = tick;
  asm volatile("sti");
  return out;
}


static void timer_callback(registers_t regs)
{
  tick++;
}


// Start interrupts
void timer_init()
{
    tick = 0;
    
    asm volatile("cli");
    
    timer_interrupting = 1;
    // Firstly, register our timer callback.
    register_interrupt_handler(IRQ0, &timer_callback);

//    printf("low/high: ");
//    print_hex8(TIMER_DIV_LOW);
//    printf(", ");
//    print_hex8(TIMER_DIV_HIGH);
//    printf("\n");

    // Send the command byte.
    idt_outb_delay(0x36, PIT_MODE);

    // Send the frequency divisor.
    idt_outb_delay(TIMER_DIV_LOW, PIT_CH0);
    idt_outb_delay(TIMER_DIV_HIGH, PIT_CH0);
//    idt_outb_delay(0xf0, PIT_CH0);
//    idt_outb_delay(0xf0, PIT_CH0);
    
    asm volatile("sti");
    // Linux does this... Activate??
    //idt_outb_delay(0xFe, 0x21);
}


// wait 1/50th of second
static void timer_wait_one_tick()
{
    u32 val_start = timer_tick();
    while (val_start == timer_tick())
    {
    }
}

// User functions for time delays.

// No dma operation should interrupt this call for more than 60mS
// But this is unlikely!
void timer_mdelay10()
{
  if (timer_interrupting)
  {
    timer_wait_one_tick();  // actually 20mS, but never mind.
  } 
  else
  {
    timer_set();
    while(!timer_expired(TIMER_10MS));
  }
}

void timer_mdelay1()
{
  if (timer_interrupting)
  {
    timer_wait_one_tick(); // we never use this anyhow.  
  } 
  else
  {
    timer_set();
    while(!timer_expired(TIMER_1MS));
  }
}

void timer_delay1S()
{
  int i;
  for(i=0;i<100;i++)
  {
    timer_mdelay10();
  }
}

void timer_udelay(unsigned int usecs)
{
  unsigned int i;
  for (i=0;i<usecs;i++)
  {
    inb(0x80);
  }
}



