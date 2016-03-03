.intel_syntax noprefix
.text
	.globl	_init_list
	.globl	_alloc_list
	.globl	_free_list
	.globl	_list_next
	.globl	_list_pop
	.globl	_list_push
	.globl	_list_get
	.globl	_list_set
	.globl	jmplist

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
	call _alloc_list
	mov [jmplist], rax
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


__list_next_zero:
	pop rbx
	ret
__list_next_end_down:
	mov rbx, [rax+8]
	shr rbx, 48
	test rbx, 0x8000
	jnz __list_next_end_down_neg
	mov rbx, [rax+24]
	shr rbx, 48
	test rbx, 0x8000
	jnz __list_next_end_valid
	jmp __list_next_end_down_same_sign
__list_next_end_down_neg:
	mov rbx, [rax+24]
	shr rbx, 48
	test rbx, 0x8000
	jz __list_next_invalid_range
__list_next_end_down_same_sign:
	mov rbx, [rax+8]
	cmp rbx, [rax+24]
	jle __list_next_invalid_range
	jmp __list_next_end_valid
__list_next_compute:
	pop rcx
	mov [rax], rbx
	pop rax
	pop rbx
	ret
_list_next:
	push rax
	mov rax, [rax]
	test rax, rax
	jz __list_next_zero
	cmp byte ptr [rax], 0x72
	jne __list_next_next
	push rax
	mov rax, [rax+4]
	mov rbx, [rax]
	cmp rbx, [range_nan]
	jnz __list_next_incr
	mov rbx, [rax+16]
	cmp rbx, 0
	jne __list_next_init
	mov rbx, [rax+8]
	xor rbx, [rax+24]
	shr rbx, 48
	test rbx, 0x8000
	jz __list_next_init_same_sign
	mov rbx, [rax+24]
	shr rbx, 48
	and rbx, 0x8000
	or rbx, 0x3ff0
	shl rbx, 48
	jmp __list_next_init_set
__list_next_init_same_sign:
	mov rbx, [rax+8]
	cmp rbx, [rax+24]
	jg __list_next_init_neg
	mov rbx, 0x3ff0
	shl rbx, 48
	jmp __list_next_init_set
__list_next_init_neg:
	mov rbx, 0xbff0
	shl rbx, 48
__list_next_init_set:
	mov [rax+16], rbx
__list_next_init:
	mov rbx, [rax+8]
	jmp __list_next_end
__list_next_incr:
	fld qword ptr [rax]
	fadd qword ptr [rax+16]
	fstp qword ptr [rax]
	mov rbx, [rax]
__list_next_end:
	push rbx
	mov rbx, [rax+16]
	cmp rbx, 0
	jl __list_next_end_down
	mov rbx, [rax+8]
	shr rbx, 48
	test rbx, 0x8000
	jz __list_next_end_up_pos
	mov rbx, [rax+24]
	shr rbx, 48
	test rbx, 0x8000
	jz __list_next_end_valid
	jmp __list_next_end_up_same_sign
__list_next_end_up_pos:
	mov rbx, [rax+24]
	shr rbx, 48
	test rbx, 0x8000
	jnz __list_next_invalid_range
__list_next_end_up_same_sign:
	mov rbx, [rax+8]
	cmp rbx, [rax+24]
	jge __list_next_invalid_range
__list_next_end_valid:
	pop rbx
	push rcx
	mov rcx, rbx
	xor rcx, [rax+24]
	shr rcx, 48
	test rcx, 0x8000
	jnz __list_next_compute
	cmp rbx, [rax+24]
	jl __list_next_compute
__list_next_end_up:
	pop rcx
	mov rbx, [range_nan]
	mov [rax], rbx
	pop rax
	pop rbx
	mov rax, [rbx+8]
	jmp _list_next
__list_next_invalid_range:
	pop rbx
	pop rax
	xor rax, rax
__list_next_next:
	pop rbx
	mov rbx, [rbx+8]
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
	.comm jmplist 8
