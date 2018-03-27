#include <linux/linkage.h>

ENTRY(my_ret_handler)
	pushq %rsp
	pushfq
	subq $0x18, %rsp
	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %rcx
	pushq %rax
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %rbx
	pushq %rbp
	movq %rsp, %rbp
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	movq %rsp, %rdi
	
	call testfunction_ret_1

#	movq %rax, 0x98(%rsp)
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbp
	popq %rbx
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rax
	popq %rcx
	popq %rdx
	popq %rsi
	popq %rdi
	addq $0x18, %rsp
	popfq
	retq
END(my_ret_handler)
