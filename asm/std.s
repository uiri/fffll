section		.text
	global	_deref_var
	global	_safe_exit
	global	_init_heap
	global	_read
	global	_write
	global	stdin
	global	stdout
	global	stderr
	global	brk
	global	sbrk
	extern	_numtostr

_init_heap:
	mov eax, 0x2d
	xor rdx, rdx
	int 0x80
	mov dword [sbrk], eax
	mov ebx, eax
	add ebx, 2048
	mov eax, 0x2d
	int 0x80
	cmp eax, ebx
	jb _safe_exit
	mov dword [brk], eax
	ret

_freestr:
	push rax

__freestr_overwrite:
	mov byte [eax], 0
	inc eax
	cmp byte [eax], 0
	jne __freestr_overwrite

__freestr_clearblock:
	inc eax
	cmp eax, sbrk
	jae __freestr_shrink
	cmp byte [eax], 0
	je __freestr_clearblock
__freestr_ret:
	pop rax
	ret
__freestr_shrink:
	pop rax
	mov [sbrk], eax
	xor eax, eax
	ret

_allocstr:
	push rbx
	mov ebx, [sbrk]
	add ebx, 32
	cmp ebx, [brk]
	jbe __allocstr_room
	add ebx, 2016
	mov eax, 0x2d
	int 0x80
	sub ebx, 2016
	cmp eax, [brk]
	jbe _safe_exit		; Out of Memory
	mov dword [brk], eax
__allocstr_room:
	mov eax, [sbrk]
	mov dword [sbrk], ebx
	pop rbx
	ret

_reallocstr:
	push rbx
	push rax
	push rax
	mov ecx, 32

__reallocstr_readstr:
	inc eax
	cmp byte [eax], 0
	jne __reallocstr_readstr
	mov ebx, [sbrk]

__reallocstr_clearblock:
	cmp eax, ebx
	je __reallocstr_extend
	inc eax
	cmp byte [eax], 0
	je __reallocstr_clearblock
__reallocstr_move:
	add ecx, eax
	pop rax
	sub ecx, eax
	push rbx
__reallocstr_extend:
	add ebx, ecx
	cmp ebx, [brk]
	jbe __reallocstr_room
	mov eax, 0x2d
	add ebx, 2048
	int 0x80
	sub ebx, 2048
	cmp eax, [brk]
	jbe _safe_exit		; Out of Memory
	mov dword [brk], eax
__reallocstr_room:
	pop rax
	pop rcx
	push rax
	cmp ecx, eax
	je __reallocstr_ret
__reallocstr_copyloop:
	cmp eax, [brk]
	je __reallocstr_ret
	cmp byte [ecx], 0
	je __reallocstr_ret
	mov dl, [ecx]
	mov [eax], dl
	mov byte [ecx], 0
	inc eax
	inc ecx
	jmp __reallocstr_copyloop

__reallocstr_ret:
	pop rax
	mov dword [sbrk], ebx
	pop rbx
	ret

__deref_var_once:
	inc eax
	mov eax, [eax]

_deref_var:
	cmp byte [eax], 'v'	; check for variable
	je __deref_var_once
	ret

_safe_exit:
	mov	ebx, eax 	; move return value
	mov	eax,1		; sys_exit
	int	0x80

_write:
	mov edx, eax
	mov eax, ebx
	;; Check for 'f' or 'h'
	cmp byte [edx], 'f'
	jne _safe_exit
	add edx, 4
	xor ebx, ebx
	mov [rnb], ebx
	mov bl, [edx]
	xor edx, edx
	call _deref_var
	cmp byte [eax], 'n'	; check for number
	jne __write_not_num
	push rbx
	push rax
	call _allocstr
	mov ebx, eax
	mov [rnb], eax
	pop rax
	push rbx
	call _numtostr		; go to number routine
	pop rax
	pop rbx
	jmp __write_syscall

__write_not_num:
	cmp byte [eax], 's'
	jne _safe_exit
	add eax, 4
	mov eax, [eax]

__write_syscall:
	mov ecx, eax

__write_str:
	inc edx			; increase length
	inc eax			; look at next character
	cmp byte [eax], 0	; compare with null
	jne __write_str		; write it if it isn't null
	mov eax, 4		; sys_write
	int 0x80
	dec edx
	add ecx, edx
	cmp byte [ecx], 10
	je __write_ret

__write_stdout_nl:
	cmp ebx, 0x0001
	je __write_newline
	cmp ebx, 0x0002
	je __write_newline
	mov eax, 1
	ret

__write_newline:
	mov ecx, rnb	; use write buf
	cmp byte [ecx], 0
	jne __write_freestr

__write_freestr_ret:
	mov byte [ecx], 10	; store newline
	mov edx, 1	; len 1
	mov eax, 4	; sys_write
	int 0x80
__write_ret:
	ret

__write_freestr:
	mov eax, [ecx]
	call _freestr
	jmp __write_freestr_ret

_read:
	;; cmp byte [eax], 'h'
	;; je __read_http		; check for http stream
	cmp byte [eax], 'f'
	jne _safe_exit		; check for file stream
	xor ebx, ebx
	mov [rnb], ebx
	mov ebx, eax
	add ebx, 4
	mov ebx, [ebx]
	mov edx, 32
	call _allocstr
	mov ecx, eax
	push rcx
__read_fillbuf:
	mov eax, 3
	mov edx, 32
	int 0x80
	push rcx
	cmp eax, 0
	jl _safe_exit
	je __read_ret
	mov edx, eax
__read_nl:
	cmp byte [ecx], 10
	je __read_clearbuf
	inc ecx
	dec edx
	cmp edx, 0
	jne __read_nl
	pop rax
	push rcx
	call _reallocstr
	pop rcx
	jmp __read_fillbuf
__read_clearbuf:
	cmp edx, 0
	je __read_ret
	mov byte [ecx], 0
	inc ecx
	dec edx
	jmp __read_clearbuf
__read_ret:
	pop rax
	pop rax
	ret

section .bss
	rnb	resb 4
	brk	resb 4
	sbrk	resb 4

section	.data
	stdin		dd 'f', 0 	; stdin
	stdout		dd 'f', 1	; stdout
	stderr		dd 'f', 2	; stderr
