section		.text
	global	_start	; must be declared for linker (ld)
	extern	_write
	extern	_deref_var
	extern	_safe_exit
	extern 	stdin
	extern	stdout
	extern	stderr

_start:			;tell linker entry point
	mov eax, stdout	; file to write to
	mov ebx, msg	; string to write
	call _write
	mov eax, stdout	; file to write to
	mov ebx, num	; number to write
	call _write
	mov eax, 0
	call _safe_exit

section	.data
	msg_contents	db "I like turtles", 10, "Look at me!!!", 10, 10, 0 ; our dear string
	msg		dd 's', msg_contents ; value for string
	num		dd 'n', 0x00000000, 0x44080000
