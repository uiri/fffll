#section	.text
.intel_syntax noprefix
.text
	.globl _numtostr

__num_negsign:
	test cx, 0x0800
	jz __num_negsign_ret
	push rcx
	push rax
	push rdx
	mov ecx, offset neg_sign
	mov al, [rcx]
	mov [rbx], al
	inc ebx
	pop rdx
	pop rax
	pop rcx

__num_negsign_ret:
	ret

__num_inf:
	cmp eax, 0
	jne __num_nan
	sub edx, 0x00100000
	cmp edx, 0
	jne __num_nan
	pop rbx
	mov ecx, offset inf
	mov edx, 8
__num_inf_loop:
	mov al, [rcx]
	mov [rbx], al
	inc ebx
	inc ecx
	dec edx
	cmp edx, 0
	jne __num_inf_loop
	ret

__num_nan:
	pop rbx
	mov ecx, offset nan
	mov edx, 3
__num_nan_loop:
	mov al, [rcx]
	mov [rbx], al
	inc rbx
	inc rcx
	dec rdx
	cmp rdx, 0
	jne __num_nan_loop
	ret

__num_exp_negsign:
	cmp ax, 1023
	jge __num_exp_negsign_ret
	inc ecx
	mov byte ptr [rcx], '-'
	inc ecx

__num_exp_negsign_ret:
	ret

__num_digit_loop:
	dec ecx
	div ebx		# divide by 10
	add dl, 48
	mov [rcx], dl	# write div remainder
	xor rdx, rdx
	cmp rax, 0
	jne __num_digit_loop
	ret

__num_tinyint:
	test eax, 1
	jnz __num_mulint
	shr eax, 1
	test edx, 1
	jz __num_nocarry
	add eax, 0x80000000

__num_nocarry:
	shr edx, 1
	inc cx
	inc word ptr [eexp]
	cmp cx, 1075
	je __num_fin
	jmp __num_tinyint

__num_mulint:
	push rcx
	push rax
	mov ebx, 5
	mov rax, rdx
	xor rdx, rdx
	mul ebx
	mov rdx, rax
	pop rax
	push rdx
	mul ebx
	mov rcx, rdx
	pop rdx
	add rdx, rcx

__num_tinyround:
	pop rcx
	inc cx
	push rcx
	test rdx, 0xFFFFFFFFFFF00000
	jz __num_tinytail
	mov rcx, rdx
	shr edx, 1
	shl ecx, 31
	shr eax, 1
	add eax, ecx
	inc word ptr [eexp]
	jmp __num_tinyround

__num_tinytail:
	pop rcx

__num_tinycond:
	cmp cx, 1075
	jl __num_tinyint
	je __num_fin
	push rcx
	mov ecx, eax
	shl edx, 1
	shr ecx, 31
	add edx, ecx
	shl eax, 1
	pop rcx
	dec cx
	dec word ptr [eexp]
	jmp __num_tinycond

__num_get_range_addr:
	mov rax, [rax+4]
	jmp __num_get_range_addr_ret

_numtostr:
	cmp byte ptr [rax], 'r'
	je __num_get_range_addr
	add rax, 4

__num_get_range_addr_ret:
	push rbx
	xor rcx, rcx
	mov rdx, [rax]
	add rax, 4
	mov rbx, [rax]
	and rbx, 0x000FFFFF
	add rbx, 0x00100000
	add rax, 2
	mov cx, [rax]
	shr cx, 4
	pop rax
	xchg rbx, rax
	call __num_negsign
	xchg rdx, rax
	cmp byte ptr [rbx], '-'
	jne __num_no_equals
	inc ebx
__num_no_equals:
	push rbx
	cmp cx, 0x07FF
	je __num_inf
	cmp cx, 0x0FFF
	je __num_inf
	test cx, 0x0800
	jz __num_zero
	sub cx, 0x0800

__num_zero:
	cmp cx, 0
	jne __num_nonzero
	cmp eax, 0
	jne __num_nonzero
	cmp edx, 0x00100000
	jne __num_nonzero
	mov [eexp], cx
	add cx, 1075
	xor rdx, rdx
	mov dl, 48
	pop rbx
	mov [rbx], dl
	ret

__num_nonzero:
	push rcx
	push rax
	push rdx
	xor rax, rax
	mov ax, cx
	mov ecx, offset wnb+28
	mov byte ptr [rcx], 'e'
	call __num_exp_negsign
	pop rdx
	pop rax
	pop rcx
	push rcx
	sub rcx, 1076
	mov [eexp], cx
	pop rcx
	cmp cx, 1075
	je __num_fin
	jl __num_tinyint

__num_divint:
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
	jne __num_round
	add rbx, 5

__num_round:
	pop rcx
	cmp cx, 1075
	je __num_fin
	push rcx
	test rdx, 0xFFFFFFFFFFE00000
	jnz __num_full
	shr ebx, 1
	add eax, ebx
	mov ecx, eax
	shl eax, 1
	shl edx, 1
	shr ecx, 31
	add edx, ecx
	pop rcx
	dec cx
	push rcx
	dec word ptr [eexp]
	cmp cx, 1075
	jg __num_round

__num_full:
	pop rcx
	cmp cx, 1075
	jg __num_divint

__num_fin:
	mov ecx, offset wnb+27
	mov rbx, 10000000

__num_digit:
	div ebx		# divide by 10 000 000
	push rax
	mov rax, rdx
	mov rbx, 10
	xor rdx, rdx
	call __num_digit_loop
	pop rax
	xor rdx, rdx
	cmp rax, 0
	jne __num_digit
	pop rbx
	dec ecx

__num_digit_skip:
	inc ecx
	cmp byte ptr [rcx], 48
	je __num_digit_skip
	mov edx, offset wnb+27
	sub edx, ecx
	add dx, [eexp]
	push rdx
	xor rdx, rdx
	mov dl, [rcx]
	mov byte ptr [rcx], '.'
	dec ecx
	mov [rcx], dl
	cmp ecx, offset wnb+25
	jne __num_digit_notone
	inc ecx
	mov byte ptr [rcx], '0'
	dec ecx
	mov byte ptr [rcx], '.'
	dec ecx
	mov [rcx], dl

__num_digit_notone:
	mov edx, offset wnb+27
	sub edx, ecx
__num_digit_notone_loop:
	mov al, [rcx]
	mov [rbx], al
	inc ebx
	inc ecx
	dec edx
	cmp edx, 0
	jne __num_digit_notone_loop
	pop rax
	add ecx, edx
	inc ecx
	push rbx
	mov rbx, 10
	push rdx
	push rax
	mov rax, 1

__num_digit_exp_len:
	xor rdx, rdx
	mul rbx
	pop rdx
	push rdx
	inc ecx
	cmp byte ptr [rcx], '-'
	jne __num_digit_exp_len_possign
	inc ecx
	pop rdx
	not dx
	add dx, 1
	push rdx

__num_digit_exp_len_possign:
	cmp rdx, rax
	jge __num_digit_exp_len
	pop rax
	pop rdx
	mov edx, ecx
	inc rdx
	push rdx

__num_digit_exp:
	xor rdx, rdx
	div ebx
	add dl, 48
	mov [rcx], dl
	dec ecx
	cmp eax, 0
	jne __num_digit_exp
	cmp byte ptr [rcx], '-'
	jne __num_digit_exp_possign
	dec ecx

__num_digit_exp_possign:
	pop rdx
	sub edx, ecx
	pop rbx
__num_digit_exp_loop:
	mov al, [rcx]
	mov [rbx], al
	inc ebx
	inc ecx
	dec edx
	cmp edx, 0
	jne __num_digit_exp_loop
	mov byte ptr [rbx], 0
	ret

.bss
	.lcomm eexp	4
	.lcomm wnb	32

.data
neg_sign:	.byte '-'
inf:		.ascii "infinity"
nan:		.ascii "NaN"
