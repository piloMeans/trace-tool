#include <linux/linkage.h>

ENTRY(my_pre_handler)
    pushq %rbp

	pushq 0x10(%rsp)
	pushq %rbp
	movq %rsp, %rbp
	pushq 0x18(%rsp)

	pushq %rbp
	movq %rsp, %rbp

	subq $0xa8, %rsp
	movq %rax, 0x50(%rsp)
	movq %rcx, 0x58(%rsp)
	movq %rdx, 0x60(%rsp)
	movq %rsi, 0x68(%rsp)
	movq %rdi, 0x70(%rsp)
	movq %r8, 0x48(%rsp)
	movq %r9, 0x40(%rsp)
	movq 0xc8(%rsp), %rdx
	movq %rdx, 0x20(%rsp)

	movq 0xd8(%rsp), %rsi

	movq 0xd0(%rsp), %rdi
	movq %rdi, 0x80(%rsp)
	subq $5, %rdi

	call testfunction_1

	movq 0x40(%rsp), %r9
	movq 0x48(%rsp), %r8
	movq 0x70(%rsp), %rdi
	movq 0x68(%rsp), %rsi
	movq 0x60(%rsp), %rdx
	movq 0x58(%rsp), %rcx
	movq 0x50(%rsp), %rax
	movq 0x20(%rsp), %rbp
	addq $0xd0, %rsp
	retq
END(my_pre_handler)
