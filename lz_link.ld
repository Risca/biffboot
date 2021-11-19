
OUTPUT_FORMAT("binary")

SECTIONS
{
. = 0x50000;
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
