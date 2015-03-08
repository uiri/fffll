.intel_syntax noprefix
.text
	.globl	_start	# linker entry point

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

_list_test:
	push rax
	mov rbx, rax

__list_test_iter_loop:
	push rbx
	call __list_test_iter
	pop rbx
	dec rbx
	cmp rbx, 0
	jne __list_test_iter_loop

	pop rax
__list_test_rev_iter_loop:
	push rbx
	call __list_test_iter
	pop rbx
	inc rbx
	cmp rbx, rax
	jne __list_test_rev_iter_loop
	ret

__list_test_iter:
	mov rcx, 1

__list_shl_loop:
	shl rcx, 1
	cmp rbx, 0
	je __list_shl_break
	dec rbx
	jmp __list_shl_loop

__list_shl_break:
	mov rdx, rcx
__list_test_alloc_loop:
	push rax
	push rdx
	push rcx
	call _alloc_list
	pop rcx
	pop rdx
	dec rcx
	cmp rcx, 0
	jne __list_test_alloc_loop

	mov rcx, rdx
__list_test_free_loop:
	call _free_list
	pop rax
	dec rcx
	cmp rcx, 0
	jne __list_test_free_loop
	ret

_start:
	call _init_heap
	call _init_list
	mov rax, offset x
	call _echo
	mov rax, offset stdout
	mov rbx, offset num
	call _write
	mov rax, offset y
	call _echo
	mov rax, 15
	call _list_test
	mov rax, 0
	call _safe_exit

	.data
msg_contents:	.ascii	"I like turtles\nLookatme!!!\n\n\0"
msg:		.long	0x73, msg_contents
x:		.long	0x30, 0x0, 0x0
y:		.long	0x30, 0x0, 0x0
num:		.long	0x6e, 0x54442D18, 0x400921FB
