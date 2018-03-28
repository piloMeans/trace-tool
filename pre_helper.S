#include <linux/linkage.h>

ENTRY(my_pre_handler)

    subq $0x18, %rsp
    
    pushq 0x20(%rsp)
    pushq 0x20(%rsp)
    pushq %rbp
    movq %rsp, %rbp
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9

    callq testfunction_1

    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rax
    popq %rbp
    retq

END(my_pre_handler)
