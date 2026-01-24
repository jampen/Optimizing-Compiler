# Cyrex Optimizing Compiler
#### Fun peephole optimizing compiler

## Usage
```cyrexc [*.cyrex] [--optimized] [--ir]```
#### Flags:
```--optimized: enable optimization```

```--ir: output intermediate representation```

## Optimization Example
Input:
```
program.cyrex

function main() : int {
	var sum : int = 0
	var x : int = 0
	
	while x < 5 {
		x = x + 1
		var y : int = 0
		while y < 5 {
			y = y + 1
			sum = sum + 1
		}		
	}
	
	return sum
}
```

`cyrexc program.cyrex`

Unoptimized output
```

bits 64
section .text
global main
main:
	push rbx
	push r12
.L0:
	mov rcx, 0
	mov rdx, 0
	jmp .L2
.L2:
	mov rax, rdx
	cmp rax, 5
	setl al
	movzx rax, al
	mov r8, rax
	mov rax, r8
	test rax, rax
	jnz .L3
	jz .L4
.L3:
	mov rax, rdx
	add rax, 1
	mov r9, rax
	mov rdx, r9
	mov r10, 0
	jmp .L5
.L5:
	mov rax, r10
	cmp rax, 5
	setl al
	movzx rax, al
	mov r11, rax
	mov rax, r11
	test rax, rax
	jnz .L6
	jz .L7
.L6:
	mov rax, r10
	add rax, 1
	mov rbx, rax
	mov r10, rbx
	mov rax, rcx
	add rax, 1
	mov r12, rax
	mov rcx, r12
	jmp .L5
.L7:
	jmp .L2
.L4:
	mov rax, rcx
	jmp .L1
.L1:
	pop r12
	pop rbx
	ret
```



`cyrexc program.cyrex --optimized`

Optimized output
```
bits 64
section .text
global main
main:
	xor rcx, rcx
	xor rdx, rdx
.L2:
	cmp rdx, 5
	jge .L4
	add rdx, 1
	xor r10, r10
.L5:
	cmp r10, 5
	jge .L7
	add r10, 1
	add rcx, 1
	jmp .L5
.L7:
	jmp .L2
.L4:
	mov rax, rcx
	ret
```

