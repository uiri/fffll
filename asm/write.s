section	.text
	global _write
	extern _safe_exit
	extern _deref_var

__write_negsign:
	test cx, 0x0800
	jz __write_negsign_ret
	push rcx
	push rax
	push rdx
	mov ecx, neg_sign
	mov eax, 4
	mov edx, 1
	int 0x80
	pop rdx
	pop rax
	pop rcx

__write_negsign_ret:
	ret

__write_inf:
	cmp eax, 0
	jne __write_nan
	sub edx, 0x00100000
	cmp edx, 0
	jne __write_nan
	pop rbx
	mov ecx, inf
	mov edx, 8
	mov eax, 4
	int 0x80
	jmp __write_stdout_nl
	
__write_nan:
	pop rbx
	mov ecx, nan
	mov edx, 3
	mov eax, 4
	int 0x80
	jmp __write_stdout_nl

__write_exp_negsign:
	cmp ax, 1023
	jg __write_exp_negsign_ret
	inc ecx
	mov byte [ecx], '-'
	inc ecx

__write_exp_negsign_ret:
	ret

__write_digit_loop:
	dec ecx
	div ebx		; divide by 10
	add dl, 48
	mov [ecx], dl	; write div remainder
	xor rdx, rdx
	cmp rax, 0
	jne __write_digit_loop
	ret

__write_num_frac:
	cmp cx, 0
	je __write_num_fin
	dec cx
	shl rbx, 1
	jmp __write_num_frac

__write_num:
	add eax, 4
	push rbx
	mov edx, [eax]
	add eax, 4
	mov ebx, [eax]
	and ebx, 0x000FFFFF
	add ebx, 0x00100000
	add eax, 2
	mov cx, [eax]
	shr cx, 4
	pop rax
	xchg rbx, rax
	call __write_negsign
	xchg rdx, rax
	push rbx
	cmp cx, 0x07FF
	je __write_inf
	cmp cx, 0x0FFF
	je __write_inf
	test cx, 0x0800
	jz __write_num_pos
	sub cx, 0x0800

__write_num_pos:
	push rcx
	push rax
	push rdx
	xor rax, rax
	mov ax, cx
	mov ecx, wnb+28
	mov byte [ecx], 'e'
	call __write_exp_negsign
	pop rdx
	pop rax
	pop rcx
	cmp cx, 1023
	jl __write_num_frac
	je __write_num_fin
	sub cx, 1023
	cmp cx, 21
	jl __write_num_tinyint
	cmp cx, 52
	jl __write_num_smallint
	push rcx
	sub rcx, 52
	mov [eexp], cx
	pop rcx

__write_num_divint:
	dec cx
	mov ebx, 5
	push rcx
	push rax
	mov rax, rdx
	xor rdx, rdx
	div ebx
	mov rcx, rax
	pop rax
	push rcx
	div ebx
	mov rbx, rdx
	pop rdx
	cmp rbx, 0
	jne __write_num_round
	add rbx, 5

__write_num_round:
	test rdx, 0xFFE00000
	jnz __write_num_full
	shr ebx, 1
	add eax, ebx
	mov ecx, eax
	shl eax, 1
	shl edx, 1
	shr ecx, 31
	add edx, ecx

__write_num_nocarry:
	pop rcx
	dec cx
	push rcx
	dec dword [eexp]
	cmp cx, 52
	jg __write_num_round

__write_num_full:	
	pop rcx
	cmp cx, 52
	jg __write_num_divint

__write_num_smallint:
	mov bx, 52
	sub bx, cx
	mov cx, bx
	shr rax, cl
	push rax
	mov rax, rdx
	shr rdx, cl
	mov bx, 32
	sub bx, cx
	mov cx, bx
	shl rax, cl
	pop rbx
	add rax, rbx
	jmp __write_num_fin

__write_num_tinyint:
	mov rax, rdx
	xor rdx, rdx
	mov bx, 20
	sub bx, cx
	mov cx, bx
	shr rax, cl

__write_num_fin:
	mov ecx, wnb+27
	mov rbx, 10000000

__write_digit:
	div ebx		; divide by 10 000 000
	push rax
	mov rax, rdx
	mov rbx, 10
	xor rdx, rdx
	call __write_digit_loop
	pop rax
	xor rdx, rdx
	cmp rax, 0
	jne __write_digit
	pop rbx
	dec ecx

__write_digit_skip:
	inc ecx
	cmp byte [ecx], 48
	je __write_digit_skip
	mov edx, wnb+27
	sub edx, ecx
	dec rdx
	add rdx, [eexp]
	push rdx
	xor rdx, rdx
	mov dl, [ecx]
	mov byte [ecx], '.'
	dec ecx
	mov [ecx], dl
	cmp ecx, wnb+25
	jne __write_digit_notone
	inc ecx
	mov byte [ecx], '0'
	dec ecx
	mov byte [ecx], '.'
	dec ecx
	mov [ecx], dl

__write_digit_notone:	
	mov edx, wnb+27
	sub edx, ecx
	mov eax, 4
	int 0x80
	pop rax
	add ecx, edx
	inc ecx
	push rbx
	mov rbx, 10
	push rdx
	push rax
	mov rax, 1

__write_digit_exp_len:
	xor rdx, rdx
	mul rbx
	pop rdx
	push rdx
	inc ecx
	cmp rdx, rax
	jg __write_digit_exp_len
	pop rax
	pop rdx
	mov edx, ecx
	inc rdx
	push rdx

__write_digit_exp:
	xor rdx, rdx
	div ebx
	add dl, 48
	mov [ecx], dl
	dec ecx
	cmp eax, 0
	jne __write_digit_exp
	pop rdx
	sub edx, ecx
	pop rbx
	mov eax, 4
	int 0x80

__write_stdout_nl:
	cmp ebx, 0x0001
	je __write_newline
	cmp ebx, 0x0002
	je __write_newline
	mov eax, 1
	ret

__write_newline:
	mov ecx, wnb	; use write buf
	mov byte [ecx], 10	; store newline
	mov edx, 1	; len 1
	mov eax, 4	; sys_write
	int 0x80
	ret

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
	jne __write_stdout_nl
	ret

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
	je __write_num		; go to number routine
	cmp byte [eax], 's'
	jne _safe_exit
	add eax, 4
	mov ecx, [eax]
	mov eax, ecx
	jmp __write_str		; go to string routine

section	.data
	neg_sign	db '-'
	inf		db "infinity"
	nan		db "NaN"

section .bss
	eexp		resb 4
	wnb		resb 32
