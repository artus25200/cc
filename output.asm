global _main
section .text
extern _printf
string:
	db "%d\n"
print:
	push rbp
	mov rbp, rsp
	sub rsp, 16
	mov [rbp-4], edi
	mov [rbp-16], rsi
	mov rsi, rdi
	mov rdi, string
	mov eax, 0
	call _printf
	mov eax, 0
	leave
	ret
_main:
	push rbp
	mov rbp, rsp
	mov r8, 175
	mov r9, 1011
	imul r8, r9
	mov r9, 17986
	mov r10, 181
	sub r9, r10
	mov r10, 1
	mov rdx, 0
	mov rax, r9
	mov rcx, r10
	div rcx
	mov r9, rax
	add r8, r9
	mov r8, 1
	mov r9, 2
	add r8, r9
	mov r8, 7
	pop rbp
	mov eax, 0
	ret
