section		.text
	global	_start	; must be declared for linker (ld)
	extern	_write
	extern 	_read
	extern	_deref_var
	extern	_safe_exit
	extern	_init_heap
	extern	_init_list
	extern	_alloc_list
	extern	_free_list
	extern 	stdin
	extern	stdout
	extern	stderr

_echo:
	mov ebx, eax
	mov eax, stdin	; file to read from
	push rbx
	call _read
	pop rbx
	mov dword [ebx], 's'
	add ebx, 4
	mov [ebx], eax
	sub ebx, 4
	mov eax, stdout	; file to write to
	call _write
	ret

_start:			;tell linker entry point
	call _init_heap
	call _init_list
	mov eax, x
	call _echo
	mov eax, stdout	; file to write to
	mov ebx, num	; number to write
	call _write
	mov eax, y
	call _echo
	call _alloc_list
	call _free_list
	call _alloc_list
	mov eax, 0
	call _safe_exit

section	.data
	msg_contents	db "I like turtles", 10, "Look at me!!!", 10, 10, 0 ; our dear string
	msg		dd 's', msg_contents ; value for string
	x		dd '0', 0x00000000
	y		dd '0', 0x00000000
	num		dd 'n', 0x54442D18, 0x400921FB
