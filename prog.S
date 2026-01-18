bits 64
section .text
global main
main:
	xor rcx, rcx
	xor rdx, rdx
.L2:
	cmp rdx, 5
	jge .L4
	xor r9, r9
.L5:
	cmp r9, 5
	jge .L7
	add r9, 1
	add rcx, 1
	jmp .L5
.L7:
	add rdx, 1
	jmp .L2
.L4:
	mov rax, rcx
	ret