#include <linux/linkage.h>

ENTRY(my_pre_handler)

    
    pushq %rbp
    movq %rsp, %rbp

    pushq %r8
    pushq %r9
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi

    callq testfunction_1

    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rax
    popq %r9
    popq %r8

    popq %rbp
    retq

END(my_pre_handler)
