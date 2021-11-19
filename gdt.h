#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

#include "io.h"
#include "iolib.h"

void gdt_init();

struct gdt_entry_struct
{
    u16 limit_low;           // The lower 16 bits of the limit.
    u16 base_low;            // The lower 16 bits of the base.
    u8  base_middle;         // The next 8 bits of the base.
    u8  access;              // Access flags, determine what ring this segment can be used in.
    u8  granularity;
    u8  base_high;           // The last 8 bits of the base.
} __attribute__((packed));

typedef struct gdt_entry_struct gdt_entry_t;

// This struct describes a GDT pointer. It points to the start of
// our array of GDT entries, and is in the format required by the
// lgdt instruction.
struct gdt_ptr_struct
{
    u16 limit;               // The upper 16 bits of all selector limits.
    u32 base;                // The address of the first gdt_entry_t struct.
} __attribute__((packed));

typedef struct gdt_ptr_struct gdt_ptr_t;

#ifdef __cplusplus
}
#endif
