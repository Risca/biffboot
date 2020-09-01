

ORG 0xffff0000 ; 64k BIOS

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

	; Setup stack pointer somewhere  this gives 3840 bytes of stack
	mov esp, 0xf00

	mov	ecx, 0x1000     ; Copy the bootloader stub

	cld			; count up.
	mov	eax, 0xffff0000 ; source
	mov	esi, eax
	mov	eax, LZ_RUN_ADDR	; destination for bootloader (compressed ver.)
	mov	edi, eax
	rep	movsb

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

	mov	eax, cr0
	and	eax, 0x9fffffff
	mov	cr0, eax

	mov eax, cr0 ; Set protected mode 
	or al, byte 0x01 
	mov cr0, eax 
	o32 lgdt [cs:gdt] 
	jmp long 0x10:pm_start 
 
TIMES 0xfff0-($-$$) DB 0xff 
 
reset_entry: ; power on 
	jmp real_mode 
TIMES 0x10000-($-$$) DB 0xff

