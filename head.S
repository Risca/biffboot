; Copyright (c) http://www.bifferos.com

%include "setup.i"


; Some defines for sdram drive current constants, 0x5f - 0x5c page 157.

%include "buffer_control.i"



; Detect if running under qemu, and jump if so.
%macro	qemu_check 0
	io_outl 0x0cf8, 0x80000800    ; select PCI device known to be intel under Qemu.
	mov	dx, 0xcfc
	in	eax, dx		; value 0x70008086 in EAX if qemu.
	cmp	eax, 0x70008086
%endmacro



; Output a byte to a IO port.
%macro	io_outb 2
	mov	dx, %1    ; param1 = address, param2 = data.
	mov	al, %2
	out	dx, al
%endmacro



%macro  trace 1
%ifdef UART_DEBUG
	io_outb 0x3f8, %1	;  diagnostic character
%endif
%endmacro



; Output a word to a IO port.
%macro	io_outw 2
	mov	dx, %1    ; param1 = address, param2 = data.
	mov	ax, %2
	out	dx, ax
%endmacro

; Output a long to a IO port.
%macro	io_outl 2
	mov	dx, %1    ; param1 = address, param2 = data.
	mov	eax, %2
	out	dx, eax
%endmacro


; Macros make the northbridge config easier to follow.

; Write PCI config register.  
; 	@arg1 = register
; 	@arg2 = value (16-bit)
%macro	pcicfg16 2
	mov	dx, 0xcf8
	mov	eax, 0x80000000 + %1
	out	dx, eax	
	mov	dl, 0xfc
	mov	ax, %2
	out	dx, ax
%endmacro

; Write PCI config register.  
; 	@arg1 = register
; 	@arg2 = value (32-bit)
%macro	pcicfg32 2
	mov	dx, 0xcf8
	mov	eax, 0x80000000 + %1
	out	dx, eax	
	mov	dl, 0xfc
	mov	eax, %2
	out	dx, eax
%endmacro

; Write PCI config register.  
; 	@arg1 = register
; 	@arg2 = value to and with
;	@arg3 = value to or with
; arg1 = (arg1 & arg2) | arg3
%macro	pcicfg32 3
	mov	dx, 0xcf8
	mov	eax, 0x80000000 + %1
	out	dx, eax
	mov	dl, 0xfc
	in	eax, dx
	and	eax, %2     ; and param 2.
	or	eax, %3     ; or param 3
	out	dx, eax
%endmacro


; Write PCI config register
; 	@arg1 = device
; 	@arg2 = address
;	@arg3 = value
%macro	PCIWriteConfig16 3
	mov	dx, 0xcf8
	mov	eax, 0x80000000 | (%1 << 11) | %2
	out	dx, eax	
	mov	dl, 0xfc
	mov	ax, %3
	out	dx, ax
%endmacro


%macro	clear_up 2    ; start_addr, count
	mov	ecx, %2/4	; count
	mov	eax, %1		; start
	mov	edi, eax
	mov	eax, 0
	cld			; count up
	rep	stosd		
%endmacro


%macro	clear_down 2    ; start_addr, count
	mov	ecx, %2/4	; count
	mov	eax, %1		; start
	mov	edi, eax
	mov	eax, 0
	std			; count down
	rep	stosd		
%endmacro


ORG 0xffff0000 ; 64k BIOS, top of memory. 

main:

TIMES 0xf000-($-$$) DB 0xff 


	BITS 32 
	ALIGN 8 
gdt: 
	dw (gdtEnd-gdt)-1 
	dd gdt 
	dw 0x0000 ; padding 
	dd 0, 0   ; extra segment (keeps linux happy)
	dw 0xffff, 0x0000 
	db 0x00, 0x9a, 0xcf, 0x00 ; code segment 9b, cf 
	dw 0xffff, 0x0000 
	db 0x00, 0x92, 0xcf, 0x00 ; data segment 93, cf 
gdtEnd: 
pm_start: 
	mov eax, 0x18
	mov ds, eax  
	mov es, eax  
	mov ss, eax
	mov fs, eax 
	mov gs, eax

	trace 'i'   ;  diagnostic

	; defensive programming to ensure consistency of bootloader
	; operation
	clear_up	0x0, 0x100000	; clear the bottom 1MB DRAM
	clear_up	LZ_RUN_ADDR, 0x10000   ; lz loader run address.
	clear_down	BIOS_STACK_ADDR, 0x100000   ; 1MB stack
	clear_up	BIOS_RUN_ADDR, 0x100000     ; 1MB of program space

	trace 'f'   ;  diagnostic

	; Setup stack pointer somewhere
	mov esp, BIOS_STACK_ADDR

	mov	ecx, 0x1000     ; Copy the decompressor.  This many bytes max

	cld			; count up.
	mov	eax, 0xffff0000 ; source
	mov	esi, eax
	mov	eax, LZ_RUN_ADDR	; destination for bootloader (compressed ver.)
	mov	edi, eax
	rep	movsb

	pcicfg32	0x83c, 0x0000000c
	pcicfg32	0x810, 0x0000ea00  ; IO base address for MAC2
	pcicfg32	0x814, 0xd3900000
	pcicfg16	0x804, 0x0007

	; verify that the LZ portion was copied correctly (serves as a DRAM
	; check)

	mov	eax, 0xffff0000
	mov	esi, eax
	mov	eax, LZ_RUN_ADDR
	mov	edi, eax

	mov	eax, [esi]
	mov	ebx, [edi]
	cmp	eax, ebx
	je 	skip
	trace 'e'   ; Faulty DRAM?
skip:

	trace 'f'   ;  diagnostic

	jmp	LZ_RUN_ADDR   ; jump to destination
; ============================================================ 
; 16-bit below this line 
; ============================================================ 
	BITS 16
real_mode:
	cli 
	cld
	mov	ax, cs ; ds = cs 
	mov	ds, ax 
	mov	ax, 0  ; ES = 0
	mov	es, ax


	qemu_check
	je skip_qemu_1

	; master interrupt (0-7) irq5 level, others edge
	io_outb	0x4d0, 0x20   ;  f8 for LOADER32
;	io_outb	0x4d0, 0xf8   ;  f8 for LOADER32

	; slave interrupt (8-15) irq15, 14, 12, 11, 10, 9 level
	io_outb	0x4d1, 0xde   

	; init DRAM refresh and misc. chipset settings.
	io_outb	0x43, 0x54   ; command reg.
	io_outb	0x41, 0x12   ; set DRAM refresh setting (prob. not used)

	; LOADER32
	io_outb	0x08, 0x04
	io_outb	0x10, 0x04
	io_outb	0x61, 0xfc

skip_qemu_1:

	;invd			; enable L1 cache
	mov	eax, cr0
	and	eax, 0x9fffffff
	mov	cr0, eax

	qemu_check
	je skip_qemu_2

	; Disable the PIC?
	io_outb	0x21, 0xFF      ; all master IRQ enabled.
	io_outb	0xA1, 0xFF      ; all slave IRQ enabled (8-15)
	
	; Northbridge
	pcicfg32	0x68, 0x0006c99f   ; mem timing
	pcicfg32	0x6c, 0x00800451  ; mem bank  16MByte   000451 == 32MB
	pcicfg32	0x70, 0x04000003  ; is this needed at all?
	pcicfg32	0x64, 0xff00ffff, 0x00790000
	; pcicfg32	0x84, 0x00000000  ; memory attribute redboot

	; South bridge, bootstrapping register
	pcicfg32	0x3840, 0xffffffff, 0x87ff0600

	; incorrect Serial IRQ control register makes system not boot :-).
	; 50h - 12h default.
	; 51h - 3.3v/5.0v PAD pull up/down control reg Pull-up. must be 10h
	pcicfg32	0x3850, 0x84101012  

	; value prior to supporting etron dram.
	;pcicfg32	0x385c, 0x00002315  	; DRAM buffer strength

	PCIWriteConfig16   0x7, 0x5c, [buffer_strength_control_1]

	; PCI interrupt routing table register
	;/ 31-28 - 0xD gives IRQ14 for EHCI
	;/ 27-24 - 0xf gives IRQ15 for OHCI
	;/ 23-20 - 0x0 gives disable for MAC1
	;/ 19-16 - 0x3 gives IRQ10 for MAC0
	;/ 15-12 - 0x1 gives IRQ9  for INTD_n
	;/ 11-8  - 0x1 gives IRQ9  for INTC_n
	;/ 7-4   - 0xb gives IRQ12 for INTB_n
	;/ 3-0   - 0x7 gives IRQ6 for INTA_n  
	;/ 3-0   - 0x3 gives IRQ10 for INTA_n  
	pcicfg32	0x3858, 0xDF0311B3   ; // PCI IRQ routing

	; EHCI
	pcicfg32	0x513c, 0x0000020e   ; //IRQ 14
	pcicfg32	0x5110, 0xd4900000    ; //mem addr.
	pcicfg32	0x510c, 0x00002008    ; //latency timer
	pcicfg16	0x5104, 0x0006    ; //enable

	; OHCI
	pcicfg32	0x503c, 0x5000010f    ; //IRQ 15
	pcicfg32	0x5010, 0xd5900000    ; //mem addr
	pcicfg32	0x500c, 0x00002008    ; //latency timer
	pcicfg16	0x5004, 0x0006    ; //enable

	; USB Phy control
	pcicfg32	0xc0, 0xffffffff, 0x00040000  ; //enable

	; MAC 1
	pcicfg32	0x403c, 0x0000000A ;    // IRQ 10 assigned to MAC0
	pcicfg32	0x4010, 0x0000E800 ;    // IO Base address
	pcicfg32	0x4014, 0xD1900000 ;    // Memory mapped IO addr.
	pcicfg16	0x4004, 0x0007 ;

	pcicfg32	0x84, 0x3ff00000	; memory attribute REDBOOT

skip_qemu_2:

	qemu_check
	jne skip_qemu_3

	; OHCI IO mem
	io_outl 0x0cf8, 0x80000a10
	io_outl 0x0cfc, 0xf0000000

	; Enable the OHCI device
	io_outl 0x0cf8, 0x80000a04
	io_outw 0x0cfc, 0x0002

	; PCI routing for OHCI (IRQ 11)
	io_outl 0x0cf8, 0x80000a3c
	io_outb 0x0cfc, 0x0b

skip_qemu_3:


%ifdef UART_DEBUG
  	pcicfg32 0x3854, 0x000003f8	; COM1 address

  	; Set DLAB (bit7) to get to divisor regs.
  	io_outb 0x3fb, 0x83
  	io_outb 0x3f8, 0x01  ;  low byte 115200
  	io_outb 0x3f9, 0x00  ;  high byte 115200
  	io_outb 0x3fb, 0x03  ;  unset bit7 (DLAB)
  	io_outb 0x3fa, 0x07  ;  enable and clear fifos
%endif

	trace 'B'   ;  diagnostic

	mov eax, cr0 ; Set protected mode 
	or al, byte 0x01 
	mov cr0, eax 
	o32 lgdt [cs:gdt] 
	jmp long 0x10:pm_start 

 
TIMES 0xfff0-($-$$) DB 0xff 
 
reset_entry: ; power on 
	jmp real_mode 
TIMES 0xfff4-($-$$) DB 0xff  ; pad after the 3-byte jmp instruction

buffer_strength_control_1:
	DW DRAM_BUFFER_STRENGTH     ; setting for etron SDRAM chip, use 2315 for old DRAM.
reserved1:
	DW 0xffff

mac_address:
	DW 0xffff
	DW 0xffff
	DW 0xffff
memory_size:
	DB 0x00    ; this will auto-detect memory size
reserved2:
	DB 0xff    ; unused, for future expansion

