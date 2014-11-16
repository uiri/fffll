section		.text
	global _deref_var
	global _safe_exit
	global stdin
	global stdout
	global stderr
	
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

section	.data
	stdin		dd 'f', 0 	; stdin
	stdout		dd 'f', 1	; stdout
	stderr		dd 'f', 2	; stderr
