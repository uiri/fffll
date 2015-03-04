.intel_syntax noprefix
.text
#section		.text
	.globl	_start	# must be declared for linker (ld)
	#extern	_write
	#extern 	_read
	#extern	_deref_var
	#extern	_safe_exit
	#extern	_init_heap
	#extern 	stdin
	#extern	stdout
	#extern	stderr

_echo:
	mov rbx, rax
	mov rax, offset stdin	# file to read from
	push rbx
	call _read
	pop rbx
	mov byte ptr [rbx], 's'
	add rbx, 4
	mov [rbx], rax
	sub rbx, 4
	mov rax, offset stdout	# file to write to
	call _write
	ret

_start:			#tell linker entry point
	call _init_heap
	call _init_list
	mov rax, offset x
	call _echo
	mov rax, offset stdout
	mov rbx, offset num
	call _write
	mov rax, offset y
	call _echo
	call _alloc_list
	call _free_list
	call _alloc_list
	mov eax, 0
	call _safe_exit

#section	.data
#	msg_contents	db "I like turtles", 10, "Look at me!!!", 10, 10, 0 ; our dear string
#	msg		dd 's', msg_contents ; value for string
#	x		dd '0', 0x00000000
#	y		dd '0', 0x00000000
#	num		dd 'n', 0x54442D18, 0x400921FB

	.data
msg_contents:	.ascii	"I like turtles\nLookatme!!!\n\n\0"
msg:		.long	0x73, msg_contents
x:		.long	0x30, 0x0, 0x0
y:		.long	0x30, 0x0, 0x0
num:		.long	0x6e, 0x54442D18, 0x400921FB
