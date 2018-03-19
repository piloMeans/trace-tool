/*
 * NOTE: This example is works on x86 and powerpc.
 * Here's a sample kernel module showing the use of kprobes to dump a
 * stack trace and selected registers when do_fork() is called.
 *
 * For more information on theory of operation of kprobes, see
 * Documentation/kprobes.txt
 *
 * You will see the trace data in /var/log/messages and on the console
 * whenever do_fork() is invoked to create a new process.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/skbuff.h>
#include <linux/inet.h>
#include <uapi/asm/ptrace-abi.h>

int testfunction_1(void){

__asm__(
	"pushq %%rbp;\n"

	"pushq 0x10(%%rsp);\n"
	"pushq %%rbp;\n"
	"movq %%rsp, %%rbp;\n"
	"pushq 0x18(%%rsp);\n"

	"pushq %%rbp;\n"
	"movq %%rsp, %%rbp;\n"

	"subq $0xa8, %%rsp;\n"
	"movq %%rax, 0x50(%%rsp);\n"
	"movq %%rcx, 0x58(%%rsp);\n"
	"movq %%rdx, 0x60(%%rsp);\n"
	"movq %%rsi, 0x68(%%rsp);\n"
	"movq %%rdi, 0x70(%%rsp);\n"
	"movq %%r8, 0x48(%%rsp);\n"
	"movq %%r9, 0x40(%%rsp);\n"
	"movq 0xc8(%%rsp), %%rdx;\n"
	"movq %%rdx, 0x20(%%rsp);\n"

	"movq 0xd8(%%rsp), %%rsi;\n"

	"movq 0xd0(%%rsp), %%rdi;\n"
	"movq %%rdi, 0x80(%%rsp);\n"
	"subq $5, %%rdi;\n"
	:::
	);
	
	//add your own code here
    trace_printk("hello modify\n");

	__asm__(
	"movq 0x40(%%rsp), %%r9;\n"
	"movq 0x48(%%rsp), %%r8;\n"
	"movq 0x70(%%rsp), %%rdi;\n"
	"movq 0x68(%%rsp), %%rsi;\n"
	"movq 0x60(%%rsp), %%rdx;\n"
	"movq 0x58(%%rsp), %%rcx;\n"
	"movq 0x50(%%rsp), %%rax;\n"
	"movq 0x20(%%rsp), %%rbp;\n"
	"addq $0xd0, %%rsp;\n"
	:::
	);
    return 0;
}

static int __init function_init(void)
{                                                                                                                         
    printk(KERN_INFO "function init\n");
    return 0;
}

static void __exit function_exit(void)
{
    printk(KERN_INFO "function exit\n");
}

module_init(function_init)
module_exit(function_exit)
MODULE_LICENSE("GPL");
