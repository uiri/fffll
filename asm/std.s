section		.text
	global _deref_var
	global _safe_exit
	global _write
	global stdin
	global stdout
	global stderr
	extern _numtostr
	extern brk

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
	mov bl, [edx]
	xor edx, edx
	call _deref_var
	cmp byte [eax], 'n'	; check for number
	jne __write_not_num
	push rbx
	mov ebx, rnb
	call _numtostr		; go to number routine
	mov eax, rnb
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
	mov byte [ecx], 10	; store newline
	mov edx, 1	; len 1
	mov eax, 4	; sys_write
	int 0x80
__write_ret:
	ret

section .bss
	rnb		resb 32

section	.data
	stdin		dd 'f', 0 	; stdin
	stdout		dd 'f', 1	; stdout
	stderr		dd 'f', 2	; stderr
