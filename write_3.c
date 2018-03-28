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
#include <linux/smp.h>
#include <uapi/linux/time.h>

struct data_trans{
	unsigned long ret_addr;
};

extern void my_pre_handler(void);
extern void my_ret_handler(void);

//static struct data_trans mydata_trans[NR_CPUS];

unsigned long addr;
u32 content;
atomic_t * addr3;
unsigned char brk = 0xcc;
unsigned char call= 0xe8;
unsigned char origin;


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

void testfunction_ret_1(void){

	//int cpu;	

	//ret function
	//trace_printk("hello ret\n");
	unsigned long func;
	struct timespec time;
	struct timespec time2;

	//cpu=smp_processor_id();

	__asm__ __volatile__(
		"movq  0xa8(%%rbp), %0;"		// get the function addr
		"movq  0x98(%%rbp), %1;"
		"movq  0x90(%%rbp), %2;"
		: "=a"(func), "=b"(time.tv_sec), "=c"(time.tv_nsec)
		:
		:
	);

	getnstimeofday(&time2);
	printk(KERN_INFO "before is %p.%p after is %p.%p\n", time.tv_sec, time.tv_nsec, time2.tv_sec, time2.tv_nsec);


}
void testfunction_1(void){
// invisible instruction
//	push $rbp
//	mov $rsp, $rbp

	unsigned long function_ret;
	unsigned long func;
	struct timespec time;

//	int cpu;
	//add your own code here

	function_ret=(unsigned long)my_ret_handler;
	getnstimeofday(&time);
//	cpu = smp_processor_id();	

	__asm__ __volatile__(
		"movq %1, 0x68(%%rbp);"		// timestamp second
		"movq %2, 0x60(%%rbp);"		// timestamp nanosecond
		"movq %3, 0x58(%%rbp);"		// get the function ret addr
		"movq 0x50(%%rbp), %0;"		// get the function addr
		: "=a"(func)
		: "b"(time.tv_sec), "c"(time.tv_nsec),"d"(function_ret)
		:
	);
	printk(KERN_INFO "start func is %p function ret is %p timestamp %p.%p\n", func, function_ret, time.tv_sec, time.tv_nsec); 


//	mydata_trans[cpu].ret_addr=trace_ret;
}

static int __init my_write_init(void)
{
	//printk(KERN_INFO "f addr is %p\n", my_run_sync);

#if 1
	//unsigned long addr2 = kallsyms_lookup_name("testfunction_1");
	//unsigned long addr2 = (unsigned long)testfunction_1;
	unsigned long addr2 = (unsigned long)my_pre_handler;
	u32 offset;

	addr = kallsyms_lookup_name("udp_send_skb");
//	addr = kallsyms_lookup_name("ip_rcv");
	offset = addr2 - 5 - addr;
	content = *((u32 *)(addr+1));
	origin = *((u8*)addr);
	//printk(KERN_INFO "ip rcv addr is %p testfunction is %p content %08x\n", addr, addr2, content);

	addr3= (atomic_t *)kallsyms_lookup_name("modifying_ftrace_code");
	// also need to midify the ftrace_update_func ???
	// printk(KERN_INFO "modifying ftrace code is %p\n", addr3);
	
	set_page_rw(addr);
	//*((u8*)addr) = 0xe8;
		
	smp_wmb();
	atomic_inc(addr3);
	//*((u8*)addr) = 0xcc;
	probe_kernel_write((void*)addr, &brk, 1);
	my_run_sync();

	//*((u32*)(addr+1))= offset;
	probe_kernel_write((void*)(addr+1), &offset, 4);
	
	my_run_sync();	

	//*((u8*)addr) = 0xe8;
	probe_kernel_write((void*)addr, &call, 1);

	my_run_sync();	

	atomic_dec(addr3);
	set_page_ro(addr);

#endif
		
	printk(KERN_INFO "write init\n");
    return 0;
}

static void __exit my_write_exit(void)
{
#if 1
	set_page_rw(addr);

	smp_wmb();
	atomic_inc(addr3);
	//*((u8*)addr) = 0xcc;
	probe_kernel_write((void*)addr, &brk, 1);

	my_run_sync();

	//*((u32*)(addr+1))= content;
	probe_kernel_write((void*)(addr+1), &content, 4);
	
	my_run_sync();	

	//*((u8*)addr) = origin;
	probe_kernel_write((void*)addr, &origin, 1);

	my_run_sync();	

	atomic_dec(addr3);
	set_page_ro(addr);

#endif
	printk(KERN_INFO "write exit\n");
}

module_init(my_write_init)
module_exit(my_write_exit)
MODULE_LICENSE("GPL");
