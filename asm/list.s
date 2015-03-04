	#section .text
.intel_syntax noprefix
.text
	.globl	_init_list
	.globl	_alloc_list
	.globl	_free_list
	#extern	_safe_exit

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
	test ax, 0x00FF
	jnz _safe_exit
	mov [listmanager], rax
	mov rbx, rax
	add rbx, 8
	mov [rax], rbx
	mov rax, rbx
	pop rdi
	pop rsi
	pop rdx
	pop r10
	pop r8
	pop r9
	ret

_free_list:
	mov rcx, [listmanager]
	add rax, 24
	mov rdx, [rcx]
	sub rdx, rax
	cmp rdx, 24
	je __free_end_list
	add rdx, rax
	mov [rax], rdx
__free_end_list:
	mov [rcx], rax
	xor rax, rax
	ret

_alloc_list:
	mov rcx, [listmanager]
	mov rax, [rcx]
	mov rdx, [rcx]
	add rdx, 24
	cmp qword ptr [rdx], 0
	je __alloc_list_nextblock
	mov rdx, [rdx]
	mov [rcx], rdx
	jmp __alloc_list_ret

__alloc_list_nextblock:
	add rdx, 8
	sub rdx, rcx
	cmp rdx, 0x1000
	je __alloc_list_newpage
	add qword ptr [rcx], 24
	jmp __alloc_list_ret

__alloc_list_newpage:
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
	mov [listmanager], rax
	mov rbx, rax
	add rax, 8
	mov [rbx], rax
	pop rdi
	pop rsi
	pop rdx
	pop r10
	pop r8
	pop r9

__alloc_list_ret:
	ret

#section .bss
#	listmanager	resb 8

.bss
	.comm listmanager 8
