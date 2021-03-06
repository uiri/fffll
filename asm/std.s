.intel_syntax noprefix
.text
	.globl	_deref_var
	.globl	_allocvar
	.globl	_allocstr
	.globl	_safe_exit
	.globl	_init_heap
	.globl	_add
	.globl	_cat
	.globl	_die
	.globl	_head
	.globl	_len
	.globl	_mul
	.globl	_open
	.globl	_pop
	.globl 	_push
	.globl	_rcp
	.globl	_read
	.globl	_tail
	.globl	_write
	.globl	var_stdin
	.globl	var_stdout
	.globl	var_stderr
	.globl	brk
	.globl	sbrk
	.globl	zero
	.globl	range_nan

__checknum_range:
	cmp byte ptr [rax], 'r'
	jne __checknum_list
	jne _safe_exit
	mov rax, [rax+4]
	ret

__checknum_list:
	cmp byte ptr [rax], 'l'
	jne _safe_exit
	push rbx
	push 0
	push rax
	call _len
	add rsp, 16
	pop rbx
	ret

__checknum:
	cmp byte ptr [rax], 'n' # check for number
	jne __checknum_range
	add rax, 4
	ret

_allocvar:
	mov rax, [brkvar]
	cmp qword ptr [rax], 0xffffffffffffffff
	jne __allocvar_ret
	call _allocstr
	mov qword ptr [rax+24], 0xffffffffffffffff
__allocvar_ret:
	mov rbx, rax
	add rbx, 12
	mov [brkvar], rbx
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
	add rax, 4
	mov rax, [rax]
_deref_var:
	mov rax, [rax]
	cmp byte ptr [rax], 'v'	# check for variable
	je __deref_var_once
	ret


_safe_exit:
	mov rbx, rax
	mov rdx, [closelist]
_safe_exit_closeloop:
	mov rax, 3
	mov rdi, [rdx]
	mov rdx, [rdx+8]
	test rdi, rdi
	jz _safe_exit_ret
	syscall
	jmp _safe_exit_closeloop
_safe_exit_ret:
	mov rdi, rbx
	mov rax, 60	# sys_exit
	syscall

_add:
	push rbp
	mov rbp, rsp
	mov rax, rbp
	add rax, 16
	mov rbx, rax
	call _deref_var
	call __checknum
	fld qword ptr [rax]
__add_next_arg:
	add rbx, 8
	cmp qword ptr [rbx], 0
	je __add_ret
	push rbx
	push rax
	mov rax, rbx
	call _deref_var
	mov rbx, rax
	pop rax
	xchg rax, rbx
	call __checknum
	xchg rax, rbx
	fadd qword ptr [rbx]
	pop rbx
	jmp __add_next_arg
__add_ret:
	call _allocvar
	mov byte ptr [rax], 'n'
	fstp qword ptr [rax+4]
	mov rsp, rbp
	pop rbp
	ret

_cat:
	push rbp
	mov rbp, rsp
	mov rbx, rbp
	add rbx, 8
	call _allocstr
	mov rcx, 32
	mov r8, rax
	push rbx
__cat_argloop:
	pop rbx
	add rbx, 8
	push rbx
	mov rbx, [rbx]
	test rbx, rbx
	jz __cat_ret
	cmp byte ptr [rbx], 's'
	jne __cat_notstr
	mov rdx, [rbx+4]
	jmp __cat_realloc_loop
__cat_notstr:
#	cmp byte ptr [rbx], 'b'
#	jne __cat_notbool
#__cat_notbool:
#	cmp byte ptr [rbx], 'l'
#	jne __cat_notlist
#__cat_notlist:
	cmp byte ptr [rbx], 'n'
	jne __cat_notnum
	cmp rcx, 32
	jge __cat_num_ge
	xchg r8, rax
	call _reallocstr
	xchg r8, rax
	add rcx, 32
__cat_num_ge:
	push rbx
	push rcx
	push rax
	xchg rax, rbx
	call _numtostr
	pop rax
	mov rdx, rax
	pop rbx
	pop rcx
	jmp __cat_realloc_loop
__cat_notnum:
	call _safe_exit
__cat_realloc_loop:
	mov bl, [rdx]
	test bl, bl
	je __cat_argloop
	inc rdx
	mov [rax], bl
	inc rax
	dec rcx
	test rcx, rcx
	jne __cat_realloc_loop
	xchg r8, rax
	call _reallocstr
	xchg r8, rax
	add rcx, 32
	jmp __cat_realloc_loop
__cat_ret:
	call _allocvar
	mov byte ptr [rax], 's'
	mov [rax+4], r8
	mov rsp, rbp
	pop rbp
	ret

_die:
	push rbp
	mov rbp, rsp
	mov rax, rbp
	add rax, 16
	call _deref_var
	push rax
	mov rax, [jmplist]
	call _list_next
	mov [jmplist], rbx
	mov rbx, rax
	pop rax
	mov rbp, [rbx]
	mov rsp, [rbx+8]
	ret

_head:
	push rbp
	mov rbp, rsp
	mov rax, rbp
	add rax, 16
	call _deref_var
	mov rax, [rax+4]
	mov rax, [rax]
	mov rsp, rbp
	pop rbp
	ret

_len:
	push rbp
	mov rbp, rsp
	mov rax, rbp
	add rax, 8
	xor rcx, rcx
	push rax
__len_argloop:
	pop rax
	add rax, 8
	cmp qword ptr [rax], 0
	je __len_end
	push rax
	call _deref_var
	cmp byte ptr [rax], 's'
	je __len_str
	mov rax, [rax+4]
__len_list:
	call _list_next
	test rax, rax
	jz __len_argloop
	inc rcx
	mov rax, rbx
	jmp __len_list
__len_str:
	mov rax, [rax+4]
__len_strloop:
	cmp byte ptr [rax], 0
	je __len_argloop
	inc rax
	inc rcx
	jmp __len_strloop
__len_end:
	mov rax, offset lennumbuf
	mov qword ptr [rax], rcx
	fild qword ptr [rax]
	call _allocvar
	mov byte ptr [rax], 'n'
	fstp qword ptr [rax+4]
__len_ret:
	mov rsp, rbp
	pop rbp
	ret

_mul:
	push rbp
	mov rbp, rsp
	mov rax, rbp
	add rax, 16
	mov rbx, rax
	call _deref_var
	call __checknum
	fld qword ptr [rax]
__mul_next_arg:
	add rbx, 8
	cmp qword ptr [rbx], 0
	je __mul_ret
	push rbx
	push rax
	mov rax, rbx
	call _deref_var
	mov rbx, rax
	pop rax
	xchg rax, rbx
	call __checknum
	xchg rax, rbx
	fmul qword ptr [rbx]
	pop rbx
	jmp __mul_next_arg
__mul_ret:
	call _allocvar
	mov byte ptr [rax], 'n'
	fstp qword ptr [rax+4]
	mov rsp, rbp
	pop rbp
	ret

_open:
	push rbp
	mov rbp, rsp
	mov rax, rbp
	add rax, 16
	mov rax, [rax]
	mov rdi, [rax+4]
	mov rsi, 02102
	mov rdx, 0666
	mov rax, 2
	syscall
	push rax
	call _allocvar
	pop rbx
	mov [rax+4], rbx
	mov dword ptr [rax], 0x66
	push rax
	call _alloc_list
	mov rdx, [closelist]
	mov [rax+8], rdx
	mov [rax], rbx
	mov [closelist], rax
	pop rax
	mov rsp, rbp
	pop rbp
	ret

_pop:
	push rbp
	mov rbp, rsp
	mov rax, rbp
	add rax, 16
	cmp byte ptr [rax], 'l'
	jne _safe_exit
	call _list_pop
	mov rsp, rbp
	pop rbp
	ret


_push:
	push rbp
	mov rbp, rsp
	mov rax, rbp
	add rax, 16
	push rax
	add rax, 8
	call _deref_var
	mov rbx, rax
	pop rax
	call _deref_var
	cmp byte ptr [rax], 'l'
	jne _safe_exit
	mov rax, [rax+4]
	call _list_push
	mov rsp, rbp
	pop rbp
	ret


_rcp:
	push rbp
	mov rbp, rsp
	mov rax, rbp
	add rax, 16
	call _deref_var
	call __checknum
	mov rbx, rax
	mov rax, offset one
	fld qword ptr [rax]
	fdiv qword ptr [rbx]
	call _allocvar
	mov byte ptr [rax], 'n'
	fstp qword ptr [rax+4]
	mov rsp, rbp
	pop rbp
	ret


_read:
	push rbp
	mov rbp, rsp
	mov rax, rbp
	add rax, 16
	mov rbx, rax
	mov rax, [rax]
	add rbx, 8
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
	mov rdx, rax
	call _allocvar
	mov byte ptr [rax], 's'
	add rax, 4
	mov [rax], rdx
	sub rax, 4
	mov rsp, rbp
	pop rbp
	ret
_tail:
	push rbp
	mov rbp, rsp
	mov rax, rbp
	add rax, 16
	call _deref_var
	mov rax, [rax+4]
	mov rax, [rax+8]
	mov rdx, rax
	mov rbx, rax
	test rax, rax
	jz __tail_ret
	call _allocvar
	mov byte ptr [rax], 'l'
	mov rbx, rax
	call _alloc_list
	mov [rbx+4], rax
__tail_alloc_loop:
	push rdx
	mov rdx, [rdx]
	mov [rax], rdx
	pop rdx
	mov rdx, [rdx+8]
	mov [rax+8], rdx
	test rdx, rdx
	jz __tail_ret
	call _alloc_list
	jmp __tail_alloc_loop
__tail_ret:
	mov rax, rbx
	mov rsp, rbp
	pop rbp
	ret

_write:
	push rbp
	mov rbp, rsp
	mov rdx, rbp
	add rdx, 16
	mov rax, rdx
	mov rdx, [rdx]
__write_arg_loop:
	add rax, 8
	cmp qword ptr [rax], 0
	je __write_outnl
	push rdx
	push rax
	call __write_arg
	pop rax
	pop rdx
	jmp __write_arg_loop
__write_outnl:
	cmp byte ptr [rcx], 10
	je __write_outret
	cmp rbx, 0x0001
	je __write_newline
	cmp rbx, 0x0002
	je __write_newline
	mov rax, 1
	jmp __write_outret
__write_list:
	push rax
	push rdx
	mov rcx, offset rnb
	mov byte ptr [rcx], '['
	mov rdx, 1
	mov rdi, rbx
	mov rsi, rcx
	mov rax, 1
	syscall
	pop rdx
	pop rax
	push rax
	push rbx
	mov rax, [rax+4]
	call _list_next
	test rax, rax
	jz __write_list_end
	mov rcx, rbx
	pop rbx
	push rbx
	push rcx
	push rax
	push rdx
	call __write_bare_arg
	pop rdx
	pop rax
	jmp __write_list_loop_init
__write_list_loop:
	cmp rdi, 0x0001
	je __write_list_skip_comma
	cmp rdi, 0x0002
	je __write_list_skip_comma
	push rax
	push rdx
	mov rcx, offset rnb
	mov byte ptr [rcx], ','
	mov byte ptr [rcx+1], ' '
	mov rdx, 2
	mov rsi, rcx
	mov rax, 1
	syscall
	pop rdx
	pop rax
__write_list_skip_comma:
	call __write_bare_arg
__write_list_loop_init:
	pop rax
	call _list_next
	test rax, rax
	jz __write_list_end
	mov rcx, rbx
	pop rbx
	push rbx
	push rcx
	jmp __write_list_loop
__write_list_end:
	push rdx
	mov rcx, offset rnb
	mov byte ptr [rcx], ']'
	mov rdx, 1
	mov rsi, rcx
	mov rax, 1
	syscall
	pop rdx
	pop rbx
	pop rax
	ret
__write_newline:
	mov rcx, offset rnb	# use write buf
	mov byte ptr [rcx], 10
	mov rdx, 1	# len 1
	mov rdi, rbx
	mov rsi, rcx
	mov rax, 1	# sys_write
	syscall
__write_outret:
	mov rsp, rbp
	pop rbp
	xor rax, rax
	ret
__write_arg:
	## Check for 'f' or 'h'
	# cmp byte ptr [rdx], 'h'
	# je _write_http
	push rdx
	cmp byte ptr [rdx], 'f'
	jne _safe_exit
	add rdx, 4
	xor rbx, rbx
	mov [rnb], rbx
	mov bl, [rdx]
	pop rdx
	call _deref_var
__write_bare_arg:
	cmp byte ptr [rax], 'l'
	je __write_list
	xor rdx, rdx
	cmp byte ptr [rax], 'n'	# check for number
	je __write_num
	cmp byte ptr [rax], 'r' # check for range
	jne __write_bool
__write_num:
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
__write_bool:
	cmp byte ptr [rax], 'b'
	jne __write_not_num
	add rax, 4
	cmp qword ptr [rax], 0
	jne __write_true
	mov rax, offset false
	jmp __write_false
__write_true:
	mov rax, offset true
__write_false:
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
	cmp byte ptr [rcx], 32
	je __write_ret
	cmp rbx, 0x0001
	je __write_space
	cmp rbx, 0x0002
	je __write_space
	mov rax, 1
	ret
__write_space:
	mov rcx, offset rnb	# use write buf
	cmp byte ptr [rcx], 0
	jne __write_freestr
__write_freestr_ret:
	mov byte ptr [rcx], 32	# store newline
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

.bss
	.lcomm	lennumbuf	8
	.lcomm	rnb		4
	.lcomm	brk		8
	.lcomm	sbrk		8

.data
stdin:		.long	0x66, 0x0, 0x0
stdout:		.long	0x66, 0x1, 0x0
stderr:		.long	0x66, 0x2, 0x0
var_stdin:	.quad	offset stdin
var_stdout:	.quad	offset stdout
var_stderr:	.quad	offset stderr
false:		.asciz	"false"
true:		.asciz	"true"
__brkvar_init:	.long	0xffffffff, 0xffffffff
brkvar:		.long	__brkvar_init
one:		.long	0x00000000, 0x3ff00000
zero:		.long	0x00000000, 0x00000000
range_nan:	.long	0xffffffff, 0x7fffffff
