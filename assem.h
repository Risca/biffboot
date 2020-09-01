#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

void fast_outl(unsigned long);
unsigned long fast_inl(void);
void fast_clock(unsigned long);
unsigned char fast_rcvr_spi(unsigned long);
void assem_flatbin_boot(unsigned long);
void assem_linux_boot(unsigned long, unsigned long);
void assem_multi_boot(unsigned long info, unsigned long entry);
void assem_core_boot(unsigned long entry);

void assem_gdt_flush(u32 gdt_ptr);
void assem_idt_flush(u32 idt_ptr);

// Prepare to run Linux code (switch back to old gdt).
void assem_linux_gdt();

#ifdef __cplusplus
}
#endif

