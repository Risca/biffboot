


SECTION .text



global fast_outl
global fast_inl
global fast_clock
global fast_rcvr_spi
global assem_flatbin_boot
global assem_linux_boot
global assem_multi_boot
global assem_core_boot
global assem_cpuid
global assem_gdt_flush
global assem_idt_flush
global assem_linux_gdt

fast_outl:
;	push ebp
;	mov ebp, esp
;	push ecx

	; IO address
	mov	dx, 0xcfc
	mov	eax, [esp+4]
	out	dx, eax

;	pop ecx
;	pop ebp
	ret


fast_inl:
;	push ebp
;	mov ebp, esp
;	push ecx

	; IO address
	mov	dx, 0xcfc
	in	eax, dx

;	pop ecx
;	pop ebp
	ret


fast_clock:

	; Raise clock line
	mov	dx, 0xcfc
	mov	eax, [esp+4]
	or	eax, (1 << 11)
	out	dx, eax
	; lower clock line
	mov	eax, [esp+4]
	and	eax, ~(1 << 11)
	out	dx, eax

	ret



%macro	cycle_clock 0
	; Raise the clock line
	mov	eax, [esp+4]
	or	eax, (1 << 11)
	out	dx, eax
	; lower clock line
	mov	eax, [esp+4]
	and	eax, ~(1 << 11)
	out	dx, eax
%endmacro

; Sample to ebx
%macro	mmdo_sample 0
	in	eax, dx
	shr	ax, 13    ; JTAG3
	and	al, 1
	or	bl, al	
	cycle_clock
%endmacro


fast_rcvr_spi:

	; Store the IO address
	mov	dx, 0xcfc

	xor	ebx, ebx    ; zero the ebx
	mmdo_sample
	shl	ebx, 1
	mmdo_sample
	shl	ebx, 1
	mmdo_sample
	shl	ebx, 1
	mmdo_sample
	shl	ebx, 1
	mmdo_sample
	shl	ebx, 1
	mmdo_sample
	shl	ebx, 1
	mmdo_sample
	shl	ebx, 1
	mmdo_sample
	mov	eax, ebx

	ret


;#define GPIO_JTAG2  (1 << 11)
;#define GPIO_JTAG3  (1 << 13)
;#define GPIO_JTAG4  (1 <<  9)
;#define GPIO_JTAG5  (1 << 12)



assem_flatbin_boot:
	; load eax with first argument
	mov	eax, [esp+4]
	jmp	eax
	ret   ; never returns


assem_linux_boot:
	mov	al, 0x80    ; disable NMI
	out	0x70, al
	mov	eax, 0

	mov	ebp, eax	
	mov 	edi, eax
	mov	ebx, eax

	mov	eax, [esp+4]
	mov	esi, [esp+8];
	jmp	eax
	
	ret   ; never returns


assem_multi_boot:
	mov	al, 0x80    ; disable NMI
	out	0x70, al
	mov	eax, 0

	; PG clearer (bit31)  PE set (bit 0) all others underfined
	mov	eax, cr0
	and	eax, 0x7fffffff
	or	al, 1
	mov	cr0, eax

	; set the flags register
	lahf
	and	eax, ~((1>>17) | (1<<9))
	sahf

	mov	ebx, [esp+4]
	mov	ecx, [esp+8]
	mov	eax, 0x2BADB002   ; magic

	jmp	ecx
	
	ret   ; never returns



assem_core_boot:
	mov	al, 0x80    ; disable NMI
	out	0x70, al
	mov	eax, 0

	; PG clearer (bit31)  PE set (bit 0) all others underfined
	mov	eax, cr0
	and	eax, 0x7fffffff
	or	al, 1
	mov	cr0, eax

	; set the flags register
	lahf
	and	eax, ~((1>>17) | (1<<9))
	sahf

	mov	ecx, [esp+4]

	call	ecx
	
	ret


; We need a 'standard' GDT to boot Linux, like the one in flash

assem_linux_gdt:
    lgdt [0xfffff000]        ; Load the original 'ROM-based' pointer

    mov ax, 0x18      ; 0x10 is the offset in the GDT to our data segment
    mov ds, ax        ; Load all data segment selectors
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x10:.flush   ; 0x08 is the offset to our code segment: Far jump!
.flush:
    ret


; general version of the above function
assem_gdt_flush:
    mov eax, [esp+4]  ; Get the pointer to the GDT, passed as a parameter.
    lgdt [eax]        ; Load the new GDT pointer

    mov ax, 0x10      ; 0x10 is the offset in the GDT to our data segment
    mov ds, ax        ; Load all data segment selectors
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.gflush   ; 0x08 is the offset to our code segment: Far jump!
.gflush:
    ret



assem_idt_flush:
    mov eax, [esp+4]  ; Get the pointer to the IDT, passed as a parameter. 
    lidt [eax]        ; Load the IDT pointer.
    ret
