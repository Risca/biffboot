#!/usr/bin/env python

# To be included in the final binary
VERSION = "v3.11"


g_defs = {
# Comment this in to get extra debug messages
#"UART_DEBUG" : 1,

# DMA heap should be <1MB according to RDC data sheet
"BIOS_DMA_HEAP_MIN" : 0x50000,
"BIOS_DMA_HEAP_MAX" : 0x70000,

# Where the early decompressing stub loads itself to, after decompression
"LZ_RUN_ADDR" : 0x50000,
# Size of the header for uncompressed biffboot (with the MAC).
"BIOS_HDR_LEN" : 8,
# the stack for biffboot goes here.
"BIOS_STACK_ADDR" : 0xc00000,
# Where the uncompressed biffboot runs from - avoid loading anything to this address.
"BIOS_RUN_ADDR" : 0xd00008,
# Start of heap area for Biffboot goes here.
"BIOS_HEAP_ADDR" : 0xe00000,
}

ld_tmpl = """
OUTPUT_FORMAT("binary")

SECTIONS
{
. = 0x%x;
  .head : { *(.head) }
  .text : 
  { 
    *(.text*)
    *(.gnu.linkonce.t*)
  }
  .rodata :
  {
    *(.rodata*)
    *(.gnu.linkonce.r*)

    start_ctors = .;
    *(.ctor*)
    end_ctors = .;
        
    start_dtors = .;
    *(.dtor*)
    end_dtors = .;

  }  
  . = ALIGN(4096);
  .data : 
  { 
    *(.data*)
    *(.gnu.linkonce.d*)
  }
  . = ALIGN(4096);
  .bss : 
  { 
    *(.COMMON*)
    *(.bss*)
    *(.gnu.linkonce.b*)
  }
  
  /DISCARD/ :
  {
    *(.comment)
    *(.eh_frame)
  }

}
"""

def WriteFile(txt, name):
  file(name, "wb").write(txt)
  print "Written: '%s'" % name
  

def Defines(pref):
  out = ""
  for i in g_defs.keys():
    val = "0x%x" % g_defs[i]
    out += "%s %s %s\n" % (pref, i,val)
  return out




if __name__ == "__main__":

  # generate lz.bin linker script
  WriteFile(ld_tmpl % g_defs["LZ_RUN_ADDR"], "lz_link.ld")

  # generate main.bin linker script
  WriteFile(ld_tmpl % g_defs["BIOS_RUN_ADDR"], "main_link.ld")

  # generate assembler includes
  WriteFile(Defines("%define"), "setup.i")

  # generate gcc includes
  WriteFile(Defines("#define"), "setup.h")

  # Create a version file
  WriteFile('#pragma once\n#define BIFFBOOT_VERSION "%s"\n' % VERSION, "version.h")

  