;--------------
bits 64
section .text
global fib
fib:
	xor rcx, rcx
	xor rdx, rdx
	mov r8, 1
.L2:
	cmp rcx, 5
	jge .L4
	mov rax, rdx
	add rax, r8
	mov r10, rax
	mov rdx, r8
	mov r8, r10
	add rcx, 1
	jmp .L2
.L4:
	mov rax, rdx
	ret
