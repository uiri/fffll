.intel_syntax noprefix
.text
	.globl	_start	# linker entry point

_echo:
	push 0
	push 0
	mov rax, [var_stdin]	# file to read from
	push rax
	call _read
	add rsp, 16
	push rax
	mov rax, [var_stdout]	# file to write to
	push rax
	call _write
	add rsp, 24
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
	call _echo
	push 0
	mov rax, offset num
	push rax
	mov rax, [var_stdout]
	push rax
	call _write
	add rsp, 24
	call _echo
	mov rax, 15
	call _list_test
	mov rax, 0
	call _safe_exit

	.data
msg_contents:	.ascii	"I like turtles\nLookatme!!!\n\n\0"
msg:		.long	0x73, msg_contents
num:		.long	0x6e, 0x54442D18, 0xc00921FB
