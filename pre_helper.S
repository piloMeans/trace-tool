#include <linux/linkage.h>

ENTRY(my_pre_handler)

    
    pushq %rbp
    movq %rsp, %rbp

    pushq %r9
    pushq %r8
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
	movq %rsp, %rdi

    callq testfunction_1

    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rax
    popq %r8
    popq %r9

    popq %rbp
    retq

END(my_pre_handler)

ENTRY(my_pre_handler_2)

	subq $0x18, %rsp
	pushq 0x20(%rsp)
	pushq 0x20(%rsp) 

    pushq %rbp
    movq %rsp, %rbp

    pushq %r9
    pushq %r8
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
	movq %rsp, %rdi

    callq testfunction_2

    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rax
    popq %r8
    popq %r9

    popq %rbp
    retq

END(my_pre_handler_2)

ENTRY(my_ret_handler)
	pushfq
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
	popfq
    addq $0x20, %rsp
	retq
END(my_ret_handler)
