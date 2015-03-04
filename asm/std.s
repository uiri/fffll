	#section		.text
.intel_syntax noprefix
.text
	.globl	_deref_var
	.globl	_safe_exit
	.globl	_init_heap
	.globl	_read
	.globl	_write
	.globl	stdin
	.globl	stdout
	.globl	stderr
	.globl	brk
	.globl	sbrk
	#extern	_numtostr

_init_heap:
	mov rax, 0x0c
	xor rdx, rdx
	mov rdi, rbx
	mov rsi, rcx
	syscall
	mov [sbrk], rax
	mov rbx, rax
	add rbx, 2048
	mov rax, 0x0c
	mov rdi, rbx
	mov rsi, rcx
	syscall
	cmp rax, rbx
	jb _safe_exit
	mov [brk], rax
	ret

_freestr:
	push rax

__freestr_overwrite:
	mov byte ptr [rax], 0
	inc rax
	cmp byte ptr [rax], 0
	jne __freestr_overwrite

__freestr_clearblock:
	inc rax
	cmp rax, sbrk
	jae __freestr_shrink
	cmp byte ptr [rax], 0
	je __freestr_clearblock
__freestr_ret:
	pop rax
	ret
__freestr_shrink:
	pop rax
	mov [sbrk], rax
	xor rax, rax
	ret

_allocstr:
	push rbx
	mov rbx, [sbrk]
	add rbx, 32
	cmp rbx, [brk]
	jbe __allocstr_room
	add rbx, 2016
	mov rax, 0x0c
	mov rdi, rbx
	mov rsi, rcx
	syscall
	sub rbx, 2016
	cmp rax, [brk]
	jbe _safe_exit		# Out of Memory
	mov [brk], rax
__allocstr_room:
	mov rax, [sbrk]
	mov [sbrk], rbx
	pop rbx
	ret

_reallocstr:
	push rbx
	push rax
	push rax
	mov rcx, 32

__reallocstr_readstr:
	inc rax
	cmp byte ptr [rax], 0
	jne __reallocstr_readstr
	mov rbx, [sbrk]

__reallocstr_clearblock:
	cmp rax, rbx
	je __reallocstr_extend
	inc rax
	cmp byte ptr [rax], 0
	je __reallocstr_clearblock
__reallocstr_move:
	add rcx, rax
	pop rax
	sub rcx, rax
	push rbx
__reallocstr_extend:
	add rbx, rcx
	cmp rbx, [brk]
	jbe __reallocstr_room
	mov rax, 0x0c
	add rbx, 2048
	mov rdi, rbx
	mov rsi, rcx
	syscall
	sub rbx, 2048
	cmp rax, [brk]
	jbe _safe_exit		# Out of Memory
	mov [brk], rax
__reallocstr_room:
	pop rax
	pop rcx
	push rax
	cmp rcx, rax
	je __reallocstr_ret
__reallocstr_copyloop:
	cmp rax, [brk]
	je __reallocstr_ret
	cmp byte ptr [rcx], 0
	je __reallocstr_ret
	mov dl, [rcx]
	mov [rax], dl
	mov byte ptr [rcx], 0
	inc rax
	inc rcx
	jmp __reallocstr_copyloop

__reallocstr_ret:
	pop rax
	mov [sbrk], rbx
	pop rbx
	ret

__deref_var_once:
	inc rax
	mov rax, [rax]

_deref_var:
	cmp byte ptr [rax], 'v'	# check for variable
	je __deref_var_once
	ret

_safe_exit:
	mov rdi, rax 	# move return value
	mov rax,60		# sys_exit
	syscall

_write:
	mov rdx, rax
	mov rax, rbx
	## Check for 'f' or 'h'
	cmp byte ptr [rdx], 'f'
	jne _safe_exit
	add rdx, 4
	xor rbx, rbx
	mov [rnb], rbx
	mov bl, [rdx]
	xor rdx, rdx
	call _deref_var
	cmp byte ptr [rax], 'n'	# check for number
	jne __write_not_num
	push rbx
	push rax
	call _allocstr
	mov rbx, rax
	mov [rnb], rax
	pop rax
	push rbx
	call _numtostr		# go to number routine
	pop rax
	pop rbx
	jmp __write_syscall

__write_not_num:
	cmp byte ptr [rax], 's'
	jne _safe_exit
	add rax, 4
	mov rax, [rax]

__write_syscall:
	mov rcx, rax

__write_str:
	inc rdx			# increase length
	inc rax			# look at next character
	cmp byte ptr [rax], 0	# compare with null
	jne __write_str		# write it if it isn't null
	mov rsi, rcx
	mov rdi, rbx
	mov rax, 1		# sys_write
	syscall
	mov rcx, rsi
	dec rdx
	add rcx, rdx
	cmp byte ptr [rcx], 10
	je __write_ret

__write_stdout_nl:
	cmp rbx, 0x0001
	je __write_newline
	cmp rbx, 0x0002
	je __write_newline
	mov rax, 1
	ret

__write_newline:
	mov rcx, offset rnb	# use write buf
	cmp byte ptr [rcx], 0
	jne __write_freestr

__write_freestr_ret:
	mov byte ptr [rcx], 10	# store newline
	mov rdx, 1	# len 1
	mov rdi, rbx
	mov rsi, rcx
	mov rax, 1	# sys_write
	syscall
__write_ret:
	ret

__write_freestr:
	mov rax, [rcx]
	call _freestr
	jmp __write_freestr_ret

_read:
	## cmp byte ptr [rax], 'h'
	## je __read_http		# check for http stream
	cmp byte ptr [rax], 'f'
	jne _safe_exit		# check for file stream
	xor rbx, rbx
	mov [rnb], rbx
	mov rbx, rax
	add rbx, 4
	mov rbx, [rbx]
	mov rdx, 32
	call _allocstr
	mov rcx, rax
	push rcx
__read_fillbuf:
	mov rax, 0
	mov rdx, 32
	mov rdi, rbx
	mov rsi, rcx
	syscall
	mov rcx, rsi
	push rcx
	cmp rax, 0
	jl _safe_exit
	je __read_ret
	mov rdx, rax
__read_nl:
	cmp byte ptr [rcx], 10
	je __read_clearbuf
	inc rcx
	dec rdx
	cmp rdx, 0
	jne __read_nl
	pop rax
	push rcx
	call _reallocstr
	pop rcx
	jmp __read_fillbuf
__read_clearbuf:
	cmp rdx, 0
	je __read_ret
	mov byte ptr [rcx], 0
	inc rcx
	dec rdx
	jmp __read_clearbuf
__read_ret:
	pop rax
	pop rax
	ret
#section .bss
#	rnb	resb 4
#	brk	resb 4
#	sbrk	resb 4

#section	.data
#	stdin		dd 'f', 0 	; stdin
#	stdout		dd 'f', 1	; stdout
#	stderr		dd 'f', 2	; stderr

.bss
	.lcomm	rnb	4
	.lcomm	brk	8
	.lcomm	sbrk	8

.data
stdin:	.long 0x66, 0x0, 0x0
stdout:	.long 0x66, 0x1, 0x0
stderr:	.long 0x66, 0x2, 0x0
