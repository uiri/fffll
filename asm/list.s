section .text
	global	_init_list
	global	_alloc_list
	global	_free_list
	extern	_safe_exit

_init_list:
	push rbp
	push rdi
	push rsi
	push rdx
	push rcx
	push rbx
	mov eax, 0xc0
	xor ebx, ebx
	mov ecx, 4096
	mov edx, 0x0003
	mov esi, 0x0022
	mov edi, -1
	xor ebp, ebp
	int 0x80
	push rax
	not ax
	inc ax
	cmp ax, 256
	jl _safe_exit
	pop rax
	mov qword [listmanager], rax
	mov rbx, rax
	add rbx, 8
	mov [rax], rbx
	pop rbx
	pop rcx
	pop rdx
	pop rsi
	pop rdi
	pop rbp
	ret

_free_list:
	mov rcx, [listmanager]
	add rax, 16
	mov rdx, [rcx]
	sub rdx, rax
	cmp rdx, 8
	je __free_end_list
	add rdx, rax
	mov qword [rax], rdx
__free_end_list:
	mov qword [rcx], rax
	xor rax, rax
	ret

_alloc_list:
	mov rcx, [listmanager]
	mov rax, [rcx]
	mov rdx, [rcx]
	add rdx, 16
	cmp qword [rdx], 0
	je __alloc_list_nextblock
	mov rdx, [rdx]
	mov qword [rcx], rdx
	jmp __alloc_list_ret

__alloc_list_nextblock:
	add rdx, 8
	sub rdx, rcx
	cmp rdx, 0x1000
	je __alloc_list_newpage
	add qword [rcx], 24
	jmp __alloc_list_ret

__alloc_list_newpage:
	push rbp
	push rdi
	push rsi
	push rdx
	push rcx
	push rbx
	mov eax, 0xc0
	xor ebx, ebx
	mov ecx, 4096
	mov edx, 3
	mov esi, 0x0022
	mov edi, -1
	xor ebp, ebp
	int 0x80
	mov qword [listmanager], rax
	mov rbx, rax
	add rax, 8
	mov [rbx], rax
	pop rbx
	pop rcx
	pop rdx
	pop rsi
	pop rdi
	pop rbp

__alloc_list_ret:
	ret

section .bss
	listmanager	resb 8
