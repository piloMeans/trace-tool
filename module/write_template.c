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
#include <asm/current.h>
#include <uapi/linux/ip.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/jhash.h>
#include <asm/atomic.h>
#include <asm/cmpxchg.h>

#define CPU_NUM cpu_num
#define FUNC_TABLE_SIZE func_table_size
#define SPEC_FUNC_TABLE_SIZE spec_func_table_size
#define ADDR_HEAD_SIZE 100000
#define HASH_INTERVAL 12345
#define SAMPLE_RATIO sample_ratio
#define DEBUG debug

extern void my_pre_handler(void);
extern void my_pre_handler_2(void);
extern void my_ret_handler(void);

struct func_table{
	unsigned long addr;
	u32 content;
	u8	origin;
	u8 skb_idx;
	u8 sk_idx;
	u8 flag;	
	char name[30];
};


struct addr_list{
	struct addr_list *next;
	u64 addr;
};
struct addr_head{
	struct addr_list lhead;
};

struct addr_head addrHead[CPU_NUM][ADDR_HEAD_SIZE];
static struct kmem_cache *myslab;

//flag meaning:
//	0 means normal function ,mid.
//	1 means recv start function
//	2 means send start function
//	3 means recv/send end function
//	4 means certainly end function


static struct func_table my_func_table[FUNC_TABLE_SIZE]={
	FUNCTABLE
};

// spec func table is for function which change the skb->head
// and this kind of function need the ret
static struct func_table my_spec_func_table[SPEC_FUNC_TABLE_SIZE]={
	SPEC_FUNCTABLE
};

// addr 3 is for atomic_modifying_code
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

static int search(unsigned long addr, int *idx, int cpu){
	u32 high, low;
	struct addr_list *temp;
	high = ((addr) >> 32);
	low = addr;
	*idx = (jhash_2words(high, low, HASH_INTERVAL)) % ADDR_HEAD_SIZE;

	temp=&(addrHead[cpu][*idx].lhead);
	while(temp!=NULL){
		if(temp->addr == addr){
			return 0;		// find it
		}
		temp=temp->next;
	}
	return -1;		// not found
}
static inline void output(struct sk_buff *skb, struct timespec *time, int funcid){

	struct iphdr *iph;
	struct tcphdr *tcph;
	u16 data;
	u64 tstamp;
	tstamp = (time->tv_sec << 32) | time->tv_nsec;

	iph= ip_hdr(skb);
	tcph= tcp_hdr(skb);
	data = *((u16*)(skb->head + skb->network_header + 4));


	trace_printk("%p %p %x %x %x %d %d %d %d %llx\n",skb, skb->head, data, iph->saddr, iph->daddr, iph->protocol, tcph->source , tcph->dest, funcid, tstamp);
//	return iph->protocol;
}
static inline void remove_hash(int cpu, int idx, unsigned long addr){
	struct addr_list *prev, *temp;	

Again: 
	prev = &(addrHead[cpu][idx].lhead);
	temp=prev->next;
	while(temp!=NULL && temp->addr!= addr){
		prev=temp;
		temp=temp->next;
	}
	if(temp==NULL)
		goto rm_out;	
	local_irq_disable();
	if(prev->next!=temp){
		local_irq_enable();
		goto Again;
	}
	prev->next=temp->next;
	local_irq_enable();
	kmem_cache_free(myslab, temp);

rm_out:
	;
}
static inline void insert_hash(int cpu, int idx, unsigned long addr){
	struct addr_list *prev, *temp;	
	temp = kmem_cache_alloc(myslab, GFP_KERNEL);
	if(temp==NULL){
		printk(KERN_ALERT "kmem_cache_alloc Failed\n");
		goto i_out;
	}
	temp->addr= addr;

	local_irq_disable();
	prev = &(addrHead[cpu][idx].lhead);
	temp->next = prev->next;
	prev->next = temp;
	local_irq_enable();
i_out:
	;
}
static inline int check_frame(struct sk_buff *skb){
	u16 frame_type = *((u16*)(skb->head + skb->mac_header +12));
	if(frame_type == 0xdd86 || frame_type ==0x0608 || frame_type == 0x3508)
		return 0;
	return 1;
}
void testfunction_ret_1(void){

	int cpu;	

	//ret function
	//trace_printk("hello ret\n");
	//unsigned long func;
	//struct timespec time;
	//struct timespec time2;
	//int i;
	unsigned long status;
	//unsigned long time_used;
	struct sk_buff *skb;
	unsigned long head;
	unsigned long base=1;
	int idx;

	__asm__ __volatile__(
		"movq %%rdi, %0\n"
		"movq 0x80(%0), %1\n"		// get the status
		: "+a"(base), "=b"(status)
		::
	);
	if(status==0)
		goto out;

	__asm__ __volatile__(
		"movq  0x90(%2), %0;"		// get the skb
		"movq  0x88(%2), %1;"		// get the origin skb->head
		: "=b"(skb), "=d"(head)
		: "a"(base)
		:
	);
	//if( (unsigned long)(skb->head) == head)	// impossible
	//	goto out;
	cpu = smp_processor_id();
	if(cpu >= CPU_NUM){
		printk(KERN_ALERT "CPU_NUM is less than the cpu core numbers\n");
		goto out;
	}
#if 1
	if(search(head, &idx, cpu)== 0){
		remove_hash(cpu, idx, head);
		trace_printk(" %p == %p\n", (void*)head, skb->head);
		search((unsigned long)(skb->head), &idx, cpu); //find the idx
		insert_hash(cpu, idx, (unsigned long)(skb->head));
		skb_shinfo(skb)->__unused |= 0x0c;
	}
#else
		trace_printk(" %p == %p\n", (void*)head, skb->head);
#endif

out:
	;
}
void testfunction_1(void){
// invisible instruction
//	push $rbp
//	mov $rsp, $rbp

	unsigned long func;
	struct timespec time;
	struct sk_buff * skb;
	unsigned long offset;
	unsigned long base=1;
	int i;
	int idx;
	int cpu;

	__asm__ __volatile__(
		"movq %%rdi, %0\n"
		"movq 0x40(%0), %1\n"		// get the function addr
		: "+a"(base), "=b"(func)
		:
		:
	);

	getnstimeofday(&time);
#if 1
	for(i=0;i<FUNC_TABLE_SIZE;i++){
		if(func == my_func_table[i].addr +5){
			
			offset = my_func_table[i].skb_idx *8;

			__asm__ __volatile__(
		//	"addq %0, %1\n"
			"movq (%1, %2), %0\n"
			: "=d"(skb)
			: "a"(base), "b"(offset)
			:
			);
			cpu = smp_processor_id();
			if(cpu >= CPU_NUM){
				printk(KERN_ALERT "CPU NUM is less than the cpu core numbers\n");
				goto out1;
			}
#if DEBUG
			output(skb, &time, i);
#else
			if(my_func_table[i].flag != 1 &&  my_func_table[i].flag !=2){
				if((skb_shinfo(skb)->__unused & 0x0c) != 0x0c)
					goto out1;
				if(search( (unsigned long)(skb->head), &idx, cpu)==0){
					// find 

					output(skb, &time, i);

					if(my_func_table[i].flag == 3 || my_func_table[i].flag==4){
						remove_hash(cpu, idx, (unsigned long)(skb->head));
					}
				}
	
			}else{
				// not found 
				if(search((unsigned long)(skb->head), &idx, cpu)==0){
					// find 
					output(skb, &time, i);
					goto out1;
				}

				//if(my_func_table[i].flag == 1 ||  my_func_table[i].flag ==2){
					if(time.tv_nsec < SAMPLE_RATIO){
						
						if ( !check_frame(skb) )
							goto out1;
						//output
						output(skb, &time, i);

						//insert into hash
						insert_hash(cpu, idx, (unsigned long)(skb->head));
					}
				//}
			}
#endif

			goto out1;
		}
	}
out1: 
	;
#endif
}
void testfunction_2(void){
// invisible instruction
//	push $rbp
//	mov $rsp, $rbp

	unsigned long function_ret;
	unsigned long func;
	//struct timespec time;
	struct sk_buff * skb;
	unsigned long offset;
	unsigned long base=1;
	int i;
	//int idx;

	function_ret = (unsigned long)my_ret_handler;
	__asm__ __volatile__(
		"movq %%rdi, %0\n"
		"movq %2, 0x48(%0)\n"		// change the return addr
		"movq 0x40(%0), %1\n"		// get the function addr
		: "+a"(base),"=b"(func)
		: "d"(function_ret)
		:
	);
	for(i=0;i<SPEC_FUNC_TABLE_SIZE;i++){
		if(func == my_spec_func_table[i].addr + 5){
			offset = my_spec_func_table[i].skb_idx *8;
			__asm__ __volatile__(
			//	"addq %%rbp, %1\n"
				"movq (%1, %2), %0\n"
				: "=d"(skb)
				: "a"(base), "b"(offset)
				:
			);
			__asm__ __volatile__(
				"movq %1, 0x60(%0)\n"		// store the skb
				"movq %2, 0x58(%0)\n"		// store the skb->head
				"movq $1, 0x50(%0)\n"		// status
				: 
				: "a"(base), "d"(skb), "b"(skb->head)
				:
			);
			goto out2;
		}
	}
	// if not found one, then we should set status=0
	__asm__ __volatile__(
		"movq $0, 0x50(%0)\n"
		:
		: "a"(base)
		:
	);
out2:
	;
}

static void code_modify(struct func_table* func, unsigned long target_addr){

	unsigned long addr = func -> addr;
	u32 offset;
	if(addr==0)
		return;
	offset = target_addr - 5 - addr;
	func->content = *((u32 *)(addr+1));
	func->origin = *((u8*)addr);

	// the origin is likely to be (0f 1f 44 00 00)(low addr -> high addr)
	if(func->origin != 0x0f){	// not support reenter
		//legacy prefixes in https://wiki.osdev.org/X86-64_Instruction_Encoding
		//special situation for legacy is (66 66 66 66 90)(low addr -> high addr)
		if(func -> origin != 0x66 || func-> content != 0x90666666){		
			printk(KERN_ALERT "not support reenter function %s\n", func->name);
			func->addr=0;
			return ;
		}
	}
	
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
	if(addr==0)
		return;
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
	
	int i;
	int j;
	
	//init slab
	//printk(KERN_INFO "sizeof skb is %d\n", sizeof(struct sk_buff));
	myslab = kmem_cache_create("myslab", sizeof(struct addr_list), 0, 
				SLAB_HWCACHE_ALIGN | SLAB_POISON | SLAB_RED_ZONE, NULL);

	for(i=0;i<CPU_NUM;i++){
		for(j=0;j<ADDR_HEAD_SIZE;j++){
			addrHead[i][j].lhead.next=NULL;
			addrHead[i][j].lhead.addr=0;
		}
	}

	addr3= (atomic_t *)kallsyms_lookup_name("modifying_ftrace_code");


	for(i=0;i<FUNC_TABLE_SIZE;i++){
		my_func_table[i].addr = kallsyms_lookup_name(my_func_table[i].name);
		if(my_func_table[i].addr==0){
			printk(KERN_INFO "function %s not found in the kallsyms\n", my_func_table[i].name);
		}
		code_modify( &(my_func_table[i]), (unsigned long)my_pre_handler);
	}
#if DEBUG
	for(i=0;i<FUNC_TABLE_SIZE;i++){
		printk(KERN_INFO "function %s origin is %02x and content is %08x \n", my_func_table[i].name,
			my_func_table[i].origin, my_func_table[i].content);
	}
#else
	for(i=0;i<SPEC_FUNC_TABLE_SIZE;i++){
		my_spec_func_table[i].addr = kallsyms_lookup_name(my_spec_func_table[i].name);
		code_modify( &(my_spec_func_table[i]), (unsigned long)my_pre_handler_2);
	}
#endif	
	printk(KERN_INFO "write init\n");
    return 0;
}

static void __exit my_write_exit(void)
{
	int i,j;
	
	//restore the code
	for(i=0;i<FUNC_TABLE_SIZE;i++){
		code_restore( &(my_func_table[i]));
	}
#if DEBUG
#else
	for(i=0;i<SPEC_FUNC_TABLE_SIZE;i++){
		code_restore( &(my_spec_func_table[i]));
	}
#endif
	//free the slab
	for(i=0;i<CPU_NUM;i++){
		for(j=0;j<ADDR_HEAD_SIZE;j++){
			struct addr_list *temp= (addrHead[i][j].lhead).next;
			struct addr_list *temp2;
			while(temp!=NULL){
				temp2=temp->next;
				kmem_cache_free(myslab, temp);
				temp=temp2;
			}
		}
	}
	kmem_cache_destroy(myslab);

	printk(KERN_INFO "write exit\n");
}

module_init(my_write_init)
module_exit(my_write_exit)
MODULE_LICENSE("GPL");
