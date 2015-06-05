	opt r-
	
; entry point code for N64

; macro to zero a section for initialising bss areas
; assumes the section is a multiple of 4 bytes long
; the parameter (sec) must have been declared in a section statement

zerosect	macro	sec
	local	l1,l2

	la	t0,sect(\sec)
	la	t1,sectend(\sec)
	beq	t0,t1,l2	; in case section is empty
	nop
l1:
	addiu	t0,t0,4
	sltu	at,t0,t1
	bne	at,zero,l1
	sw	zero,-4(t0)
l2:
	endm

; 4k header area for front of cartridge
	section	.header
;	dcb	4096,0

	incbin "header.bin"

; declaration of sections we're going to initialise to 0
	section	.bss
	
; start up code
	xref    boot
	section	.start
		
	zerosect .bss   ; clear out .bss section
	jal     boot    ; go execute users program
	nop
	break	$402	; just in case it returns

