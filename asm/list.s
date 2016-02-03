.intel_syntax noprefix
.text
	.globl	_init_list
	.globl	_alloc_list
	.globl	_free_list
	.globl	_list_pop
	.globl	_list_push
	.globl	_list_get
	.globl	_list_set

_init_list:
	push r9
	push r8
	push r10
	push rdx
	push rsi
	push rdi
	mov rax, 0x09
	xor rdi, rdi
	mov rsi, 4096
	mov rdx, 3
	mov r10, 0x0022
	mov r8, -1
	xor r9, r9
	syscall
	test ax, 0x0FFF
	jnz _safe_exit
	mov [listmanager], rax
	pop rdi
	pop rsi
	pop rdx
	pop r10
	pop r8
	pop r9
	ret

_free_list:
	mov qword ptr [rax], 0
	add rax, 8
	mov rdx, [listmanager]
	mov [rax], rdx
	sub rax, 8
	mov [listmanager], rax
	xor rax, rax
	ret

_alloc_list:
	mov rax, [listmanager]
	mov rdx, rax
	add rdx, 8
	cmp qword ptr [rdx], 0
	je __alloc_list_nextblock
	mov rdx, [rdx]
	mov [listmanager], rdx
	ret

__alloc_list_nextblock:
	add rdx, 8
	test rdx, 0x0FFF
	jnz __alloc_list_ret
	call _init_list
	mov [listmanager], rax
__alloc_list_ret:
	add qword ptr [listmanager], 16
	ret

_list_get:
	cmp rbx, 0
	je __list_get_ret
	mov rax, [rax+8]
	dec rbx
	jmp _list_get

__list_get_ret:
	mov rax, [rax]
	ret


_list_pop:
	cmp qword ptr [rax+8], 0
	je __list_pop_ret
	mov rax, [rax+8]
	jmp _list_pop
__list_pop_ret:
	mov rax, [rax]
	ret

_list_push:
	push rax
__list_push_traverse:
	add rax, 8
	cmp qword ptr [rax], 0
	je __list_push_ret
	mov rax, [rax]
	jmp __list_push_traverse
__list_push_ret:
	push rbx
	push rax
	call _alloc_list
	pop rbx
	xchg rax, rbx
	mov [rax], rbx
	sub rax, 8
	pop rbx
	mov [rax], rbx
	pop rax
	ret


_list_set:
	push rax
	cmp rbx, 0
	je __list_set_ret
	mov rax, [rax+8]
	dec rbx
	jmp _list_set

__list_set_ret:
	mov [rax], rcx
	pop rax
	ret

.bss
	.comm listmanager 8
