
#include <string.h>
#include "idt.h"
#include "isr.h"
#include "assem.h"  // flush function
#include "timer.h"

idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

// Extern the ISR handler array so we can nullify them on startup.
extern isr_t interrupt_handlers[];

static void idt_set_gate(u8 num, u32 base, u16 sel, u8 flags)
{
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    // We must uncomment the OR below when we get to using user-mode.
    // It sets the interrupt gate's privilege level to 3.
    idt_entries[num].flags   = flags /* | 0x60 */;
//    idt_entries[num].flags   = flags  | 0x60 ;
}


void idt_outb_delay(u8 val, u16 addr)
{
  outb(val, addr);
  timer_udelay(2);
}


static void remap_pic()
{

  asm volatile("cli");

    // mask interrupts from PIC
    idt_outb_delay(0xff, 0x21);
    idt_outb_delay(0xff, 0xA1);

    // Remap the irq table for primary
    idt_outb_delay(0x11, 0x20);
    idt_outb_delay(0x20, 0x21);
    idt_outb_delay(0x04, 0x21);
    idt_outb_delay(0x01, 0x21);
    
    // remap irq for slave
    idt_outb_delay(0x11, 0xA0);
    idt_outb_delay(0x28, 0xA1);
    idt_outb_delay(0x02, 0xA1);
    idt_outb_delay(0x01, 0xA1);    

    // Enable IRQ0 on the master with the mask   
    idt_outb_delay( 0xff, 0xA1);
    idt_outb_delay( 0xfe, 0x21);

  asm volatile("sti");

}

/*
static void linux_remap_pic()
{
    // mask interrupts from PIC
    idt_outb_delay(0xff, 0x21);
    idt_outb_delay(0xff, 0xA1);

    // Remap the irq table for primary
    idt_outb_delay(0x11, 0x20);
    idt_outb_delay(0x30, 0x21);
    idt_outb_delay(0x04, 0x21);
    idt_outb_delay(0x01, 0x21);
    
    // remap irq for slave
    idt_outb_delay(0x11, 0xA0);
    idt_outb_delay(0x38, 0xA1);
    idt_outb_delay(0x02, 0xA1);
    idt_outb_delay(0x01, 0xA1);    

    // Enable IRQ0 on the master with the mask   
    idt_outb_delay( 0xff, 0x21);
    idt_outb_delay( 0xff, 0xA1);
    idt_outb_delay( 0xfb, 0x21);
}
*/


void idt_init()
{
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base  = (u32)&idt_entries;

    memset(&idt_entries, 0, sizeof(idt_entry_t)*256);

    //linux_remap_pic();
    remap_pic();

    idt_set_gate( 0, (u32)isr0 , 0x08, 0x8E);
    idt_set_gate( 1, (u32)isr1 , 0x08, 0x8E);
    idt_set_gate( 2, (u32)isr2 , 0x08, 0x8E);
    idt_set_gate( 3, (u32)isr3 , 0x08, 0x8E);
    idt_set_gate( 4, (u32)isr4 , 0x08, 0x8E);
    idt_set_gate( 5, (u32)isr5 , 0x08, 0x8E);
    idt_set_gate( 6, (u32)isr6 , 0x08, 0x8E);
    idt_set_gate( 7, (u32)isr7 , 0x08, 0x8E);
    idt_set_gate( 8, (u32)isr8 , 0x08, 0x8E);
    idt_set_gate( 9, (u32)isr9 , 0x08, 0x8E);
    idt_set_gate(10, (u32)isr10, 0x08, 0x8E);
    idt_set_gate(11, (u32)isr11, 0x08, 0x8E);
    idt_set_gate(12, (u32)isr12, 0x08, 0x8E);
    idt_set_gate(13, (u32)isr13, 0x08, 0x8E);
    idt_set_gate(14, (u32)isr14, 0x08, 0x8E);
    idt_set_gate(15, (u32)isr15, 0x08, 0x8E);
    idt_set_gate(16, (u32)isr16, 0x08, 0x8E);
    idt_set_gate(17, (u32)isr17, 0x08, 0x8E);
    idt_set_gate(18, (u32)isr18, 0x08, 0x8E);
    idt_set_gate(19, (u32)isr19, 0x08, 0x8E);
    idt_set_gate(20, (u32)isr20, 0x08, 0x8E);
    idt_set_gate(21, (u32)isr21, 0x08, 0x8E);
    idt_set_gate(22, (u32)isr22, 0x08, 0x8E);
    idt_set_gate(23, (u32)isr23, 0x08, 0x8E);
    idt_set_gate(24, (u32)isr24, 0x08, 0x8E);
    idt_set_gate(25, (u32)isr25, 0x08, 0x8E);
    idt_set_gate(26, (u32)isr26, 0x08, 0x8E);
    idt_set_gate(27, (u32)isr27, 0x08, 0x8E);
    idt_set_gate(28, (u32)isr28, 0x08, 0x8E);
    idt_set_gate(29, (u32)isr29, 0x08, 0x8E);
    idt_set_gate(30, (u32)isr30, 0x08, 0x8E);
    idt_set_gate(31, (u32)isr31, 0x08, 0x8E);
    idt_set_gate(32, (u32)irq0, 0x08, 0x8E);
    idt_set_gate(33, (u32)irq1, 0x08, 0x8E);
    idt_set_gate(34, (u32)irq2, 0x08, 0x8E);
    idt_set_gate(35, (u32)irq3, 0x08, 0x8E);
    idt_set_gate(36, (u32)irq4, 0x08, 0x8E);
    idt_set_gate(37, (u32)irq5, 0x08, 0x8E);
    idt_set_gate(38, (u32)irq6, 0x08, 0x8E);
    idt_set_gate(39, (u32)irq7, 0x08, 0x8E);
    idt_set_gate(40, (u32)irq8, 0x08, 0x8E);
    idt_set_gate(41, (u32)irq9, 0x08, 0x8E);
    idt_set_gate(42, (u32)irq10, 0x08, 0x8E);
    idt_set_gate(43, (u32)irq11, 0x08, 0x8E);
    idt_set_gate(44, (u32)irq12, 0x08, 0x8E);
    idt_set_gate(45, (u32)irq13, 0x08, 0x8E);
    idt_set_gate(46, (u32)irq14, 0x08, 0x8E);
    idt_set_gate(47, (u32)irq15, 0x08, 0x8E);

    assem_idt_flush((u32)&idt_ptr);
    
    memset(&interrupt_handlers, 0, sizeof(isr_t)*256);    
}

