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
#include <linux/cpumask.h>

#define CPU_NUM 4
#define FUNC_TABLE_SIZE 6
#define FUNC_RECORD_SIZE 1000

//struct data_trans{
//	unsigned long ret_addr;
//};

extern void my_pre_handler(void);
extern void my_ret_handler(void);

//static struct data_trans mydata_trans[NR_CPUS];

struct func_table{
	unsigned long addr;
	int skb_idx;
	u32 content;
	u8	origin;
	char name[30];
};

struct func_delay_record{
	unsigned long count[FUNC_RECORD_SIZE];
};
static struct func_table my_func_table[FUNC_TABLE_SIZE]={{0,0,0,0,"udp_send_skb"},{0,0,0,0,"ip_send_skb"},{0,0,0,0,"ip_finish_output2"},{0,0,0,0,"ip_rcv"},{0,0,0,0,"__netif_receive_skb_core"},{0,0,0,0,"ip_local_deliver"}};

static struct func_delay_record my_delay_record[CPU_NUM][FUNC_TABLE_SIZE];

//unsigned long addr;
//u32 content;
atomic_t * addr3;
const unsigned char brk = 0xcc;
const unsigned char call= 0xe8;
//unsigned char origin;


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

	int cpu;	

	//ret function
	//trace_printk("hello ret\n");
	unsigned long func;
	struct timespec time;
	struct timespec time2;
	int i;
	unsigned long status=0;
	unsigned long time_used;

	__asm__ __volatile__(
		"movq 0xa0(%%rbp), %0;"		// get the status
		: "=a"(status)
		::
	);

	if(status==0)
		goto out;

	__asm__ __volatile__(
		"movq  0xa8(%%rbp), %0;"		// get the function addr
		"movq  0x98(%%rbp), %1;"		// get the start timestamp, second
		"movq  0x90(%%rbp), %2;"		// nanosecond
		: "=a"(func), "=b"(time.tv_sec), "=c"(time.tv_nsec)
		:
		:
	);

	for(i=0;i<FUNC_TABLE_SIZE;i++){
		if(func == my_func_table[i].addr +5 ){
			
			// do something statistic here
			getnstimeofday(&time2);
			time_used = ((time2.tv_sec - time.tv_sec))*10000000 + (time2.tv_nsec - time.tv_nsec)/100;
			
			if(time_used >= FUNC_TABLE_SIZE)
				time_used = FUNC_TABLE_SIZE-1;

			cpu=smp_processor_id();
			//trace_printk("cpu now is %d delay is %d(100ns)\n", cpu, time_used);
			my_delay_record[cpu][i].count[time_used]++;
			
			goto out;
		}
	}
	//printk(KERN_INFO "before is %p.%p after is %p.%p\n", time.tv_sec, time.tv_nsec, time2.tv_sec, time2.tv_nsec);
out:
	;
}
void testfunction_1(void){
// invisible instruction
//	push $rbp
//	mov $rsp, $rbp

	unsigned long function_ret;
	unsigned long func;
	struct timespec time;
	int i;

//	int cpu;
	//add your own code here

	function_ret=(unsigned long)my_ret_handler;
//	getnstimeofday(&time);
//	cpu = smp_processor_id();	

	__asm__ __volatile__(
//		"movq %1, 0x68(%%rbp);"		// timestamp second
//		"movq %2, 0x60(%%rbp);"		// timestamp nanosecond
		"movq %1, 0x58(%%rbp);"		// set the function ret addr
		"movq 0x50(%%rbp), %0;"		// get the function addr
		: "=a"(func)
//		: "b"(time.tv_sec), "c"(time.tv_nsec),"d"(function_ret)
		: "d"(function_ret)
		:
	);


	//trace_printk("this func addr is %p\n", func);
	for(i=0;i<FUNC_TABLE_SIZE;i++){
		if(func == my_func_table[i].addr +5){

			//trace_printk("func matched addr is %p\n", my_func_table[i].addr);
			// do some filter here
			
			getnstimeofday(&time);
			__asm__ __volatile__(
			"movq $1, 0x70(%%rbp);"		// set the status to 1
			"movq %0, 0x68(%%rbp);"		// timestamp second
			"movq %1, 0x60(%%rbp);"		// timestamp nanosecond
			:
			: "b"(time.tv_sec), "c"(time.tv_nsec)
			:
			);
			goto out;
		}
	}
	//printk(KERN_INFO "start func is %p function ret is %p timestamp %p.%p\n", func, function_ret, time.tv_sec, time.tv_nsec); 
	__asm__ __volatile__(
	"movq $0, 0x70(%%rbp);"		// set the status to 0
	:::
	);	
out: 
	;
}

static void code_modify(struct func_table* func, unsigned long target_addr){

	unsigned long addr = func -> addr;
	u32 offset = target_addr - 5 - addr;
	func->content = *((u32 *)(addr+1));
	func->origin = *((u8*)addr);
	
	set_page_rw(addr);
		
	smp_wmb();
	atomic_inc(addr3);

	probe_kernel_write((void*)addr, &brk, 1);
	my_run_sync();

	probe_kernel_write((void*)(addr+1), &offset, 4);
	
	my_run_sync();	

	probe_kernel_write((void*)addr, &call, 1);

	my_run_sync();	

	atomic_dec(addr3);
	set_page_ro(addr);

	
}
static void code_restore(struct func_table* func){
	unsigned long addr = func->addr;
	set_page_rw(addr);

	smp_wmb();
	atomic_inc(addr3);
	probe_kernel_write((void*)addr, &brk, 1);

	my_run_sync();

	probe_kernel_write((void*)(addr+1), &(func->content), 4);
	
	my_run_sync();	

	probe_kernel_write((void*)addr, &(func->origin), 1);

	my_run_sync();	

	atomic_dec(addr3);
	set_page_ro(addr);

}
static int __init my_write_init(void)
{
	//printk(KERN_INFO "f addr is %p\n", my_run_sync);
	
	int i,j;
	
	addr3= (atomic_t *)kallsyms_lookup_name("modifying_ftrace_code");

	for(i=0;i<CPU_NUM;i++){
		for(j=0;j<FUNC_TABLE_SIZE;j++){
			memset(my_delay_record[i][j].count, 0, sizeof(unsigned long)*FUNC_TABLE_SIZE);
		}
	}
	
	for(i=0;i<FUNC_TABLE_SIZE;i++){
		my_func_table[i].addr = kallsyms_lookup_name(my_func_table[i].name);
		code_modify( &(my_func_table[i]), (unsigned long)my_pre_handler);
	}
		
	printk(KERN_INFO "write init\n");
    return 0;
}

static void __exit my_write_exit(void)
{
	int i,j,k;
	//output the result
	for(i=0;i<FUNC_TABLE_SIZE;i++){
		code_restore( &(my_func_table[i]));
	}
	//trace_printk("NR_CPUS is %d\n", NR_CPUS);
	//trace_printk("NR_CPUS is %d\n", num_online_cpus());
#if 1
	for(j=0;j<FUNC_TABLE_SIZE;j++){
		trace_printk("function: %s\n", my_func_table[j].name);
		for(i=0;i<CPU_NUM;i++){
			trace_printk("CPU: %d\n", i);
			for(k=0;k<FUNC_RECORD_SIZE;k++){
				trace_printk("%ld ", my_delay_record[i][j].count[k]);
			}
			trace_printk("\n");
		}
	}
#endif
	printk(KERN_INFO "write exit\n");
}

module_init(my_write_init)
module_exit(my_write_exit)
MODULE_LICENSE("GPL");
