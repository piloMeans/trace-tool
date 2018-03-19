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
#include <asm/ftrace.h>


static void set_page_rw(unsigned long addr){
	unsigned int level;
	pte_t *pte = lookup_address(addr, &level);

	if(pte->pte & (~_PAGE_RW))
		pte->pte |= _PAGE_RW;
}

static void set_page_ro(unsigned long addr){
	unsigned int level;
	pte_t *pte = lookup_address(addr, &level);

	pte->pte = pte->pte & (~_PAGE_RW);
}

static void my_do_sync_core(void *data){
	sync_core();
}

static void my_run_sync(void){
	int enable_irqs;

	if(num_online_cpus() ==1)
		return;

	enable_irqs = irqs_disabled();

	if(enable_irqs)
		local_irq_enable();
	on_each_cpu(my_do_sync_core, NULL, 1);
	if(enable_irqs)
		local_irq_disable();
}

unsigned long addr;
u32 content;
atomic_t * addr3;

static int __init my_write_init(void)
{
	unsigned long addr2 = kallsyms_lookup_name("testfunction_1");
	u32 offset;

	addr = kallsyms_lookup_name("udp_send_skb");
	offset = addr2 - 5 - addr;
	content = *((u32 *)(addr+1));
	//printk(KERN_INFO "ip rcv addr is %p testfunction is %p content %08x\n", addr, addr2, content);

	addr3= (atomic_t *)kallsyms_lookup_name("modifying_ftrace_code");
	// also need to midify the ftrace_update_func ???
	// printk(KERN_INFO "modifying ftrace code is %p\n", addr3);
	
#if 1
	set_page_rw(addr);
	//*((u8*)addr) = 0xe8;
		
	smp_wmb();
	atomic_inc(addr3);
	*((u8*)addr) = 0xcc;
	my_run_sync();

	*((u32*)(addr+1))= offset;
	
	my_run_sync();	

	*((u8*)addr) = 0xe8;

	my_run_sync();	

	atomic_dec(addr3);
	set_page_ro(addr);

#endif
	
    return 0;
}

static void __exit my_write_exit(void)
{
#if 1
	set_page_rw(addr);

	smp_wmb();
	atomic_inc(addr3);
	*((u8*)addr) = 0xcc;

	my_run_sync();

	*((u32*)(addr+1))= content;
	
	my_run_sync();	

	*((u8*)addr) = 0x0f;

	my_run_sync();	

	atomic_dec(addr3);
	set_page_ro(addr);

#endif
	printk(KERN_INFO "write exit\n");
}

module_init(my_write_init)
module_exit(my_write_exit)
MODULE_LICENSE("GPL");
