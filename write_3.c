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

#define CPU_NUM 1
#define FUNC_TABLE_SIZE 16
#define ADDR_HEAD_SIZE 100000
#define HASH_INTERVAL 12345
#define SAMPLE_RATIO 1000000000
extern void my_pre_handler(void);
//extern void my_ret_handler(void);


struct func_table{
	unsigned long addr;
	u32 content;
	u16 skb_idx;
	u8	origin;
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
//	{0,0,0,0,1,"udp_send_skb"},
	{0,0,0,0,4,"skb_free_head"},				// skb_free_head(struct sk_buff *skb)
//	{0,0,1,0,1,"ip_queue_xmit"},				// ip_queue_xmit(struct sock *sk, struct sk_buff *skb, struct flowi *fl)
	{0,0,0,0,0,"ip_rcv"},						// ip_rcv(struct sk_buff *skb, struct net_device* dev, struct packet_type *pt, struct net_device *orig_dev)
	{0,0,0,0,0,"__netif_receive_skb_core"},		// __netif_receive_skb_core(struct sk_buff *skb, bool pfmemalloc)
	{0,0,0,0,3,"ip_local_deliver"},				// ip_local_deliver(struct sk_buff *skb)
	{0,0,2,0,0,"ip_local_out"},					// ip_local_out(struct net *net, struct sock *sk, struct sk_buff *skb)
	{0,0,2,0,0,"ip_output"},					// ip_output(struct net *net, struct sock *sk, struct sk_buff *skb)
	{0,0,0,0,0,"__dev_queue_xmit"},				// __dev_queue_xmit(struct sk_buff *skb, void *accel_priv)
	{0,0,1,0,1,"napi_gro_receive"},				// napi_gro_receive(struct napi_struct *napi, struct sk_buff *skb)
	{0,0,0,0,2,"udp_send_skb"},					// udp_send_skb(struct sk_buff *skb, struct flowi4 *fl4)
	{0,0,1,0,2,"tcp_transmit_skb"},				// tcp_transmit_skb(struct sock *sk, struct sk_buff *skb, int clone_it, gfp_t gfp_mask)
	{0,0,2,0,0,"br_handle_frame_finish"},		// br_handle_frame_finish(struct net *net, struct sock *sk, struct sk_buff *skb)
	{0,0,0,0,0,"netif_receive_skb_internal"},	// netif_receive_skb_internal(struct sk_buff *skb)
	{0,0,1,0,0,"ovs_vport_receive"},			// ovs_vport_receive(struct vport *vport, struct sk_buff *skb, const struct ip_tunnel_info *tun_info)
	{0,0,1,0,0,"ovs_execute_actions"},			// ovs_execute_actions(struct datapath *dp, struct sk_buff *skb, const struct sw_flow_actions *acts, struct sw_flow_key *key)
// two driver, e1000/e1000e  ixgbe
	{0,0,0,0,3,"e1000_xmit_frame"},				// e1000_xmit_frame(struct sk_buff *skb, struct net_device *netdev);
	{0,0,0,0,3,"ixgbe_xmit_frame"},				// ixgbe_xmit_frame(struct sk_buff *skb, struct net_device *netdev);
};

//static struct func_delay_record my_delay_record[CPU_NUM][FUNC_TABLE_SIZE];

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
	u64 tstamp;
	tstamp = (time->tv_sec << 32) | time->tv_nsec;

	iph= ip_hdr(skb);
	tcph= tcp_hdr(skb);
	trace_printk("%p %x %x %d %d %d %d %llx\n", skb->head, iph->saddr, iph->daddr, iph->protocol, tcph->source , tcph->dest, funcid, tstamp);
//	return iph->protocol;
}
#if 0
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
			
			if(time_used >= FUNC_RECORD_SIZE)
				time_used = FUNC_RECORD_SIZE-1;

			cpu=smp_processor_id();
			if(cpu >= CPU_NUM){
				printk(KERN_ALERT "module write CPU_NUM is less than core numbers\n");
				goto out;
			}
			//trace_printk("cpu now is %d delay is %d(100ns)\n", cpu, time_used);
			my_delay_record[cpu][i].count[time_used]++;
			
			goto out;
		}
	}
	//printk(KERN_INFO "before is %p.%p after is %p.%p\n", time.tv_sec, time.tv_nsec, time2.tv_sec, time2.tv_nsec);
out:
	;
}
#endif
static inline void remove_hash(int cpu, int idx, struct sk_buff *skb){
	struct addr_list *prev, *temp;	

Again: 
	prev = &(addrHead[cpu][idx].lhead);
	temp=prev->next;
	while(temp->addr!=(unsigned long)(skb->head)){
		prev=temp;
		temp=temp->next;
	}
	local_irq_disable();
	if(prev->next!=temp){
		local_irq_enable();
		goto Again;
	}
	prev->next=temp->next;
	local_irq_enable();
	kmem_cache_free(myslab, temp);
}
static inline void insert_hash(int cpu, int idx, struct sk_buff *skb){
	struct addr_list *prev, *temp;	
	temp = kmem_cache_alloc(myslab, GFP_KERNEL);
	if(temp==NULL){
		printk(KERN_ALERT "kmem_cache_alloc Failed\n");
		goto i_out;
	}
	temp->addr= (unsigned long)skb->head;

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
void testfunction_1(void){
// invisible instruction
//	push $rbp
//	mov $rsp, $rbp

//	unsigned long function_ret;
	unsigned long func;
	struct timespec time;
	struct sk_buff * skb;
	int i;
	unsigned long offset;
	int idx;
	int cpu;
//	u16 protocol;
	//struct addr_list *prev, *temp;
	//add your own code here

//	function_ret=(unsigned long)my_ret_handler;
//	getnstimeofday(&time);
//	cpu = smp_processor_id();	
	
//	trace_printk("cpu is %d\n", get_cpu());
//	put_cpu();
	__asm__ __volatile__(
//		"movq %1, 0x68(%%rbp);"		// timestamp second
//		"movq %2, 0x60(%%rbp);"		// timestamp nanosecond
//		"movq %1, 0x58(%%rbp);"		// set the function ret addr
		"movq 0x50(%%rbp), %0;"		// get the function addr
		: "=a"(func)
//		: "b"(time.tv_sec), "c"(time.tv_nsec),"d"(function_ret)
//		: "d"(function_ret)
		:
		:
	);

	getnstimeofday(&time);
#if 1
	//trace_printk("this func addr is %p\n", func);
	for(i=0;i<FUNC_TABLE_SIZE;i++){
		if(func == my_func_table[i].addr +5){
			
			offset = 0x10 + my_func_table[i].skb_idx *8;

			__asm__ __volatile__(
//			"movq $1, 0x70(%%rbp);"		// set the status to 1
//			"movq %0, 0x68(%%rbp);"		// timestamp second
//			"movq %1, 0x60(%%rbp);"		// timestamp nanosecond
			"addq %%rbp, %1\n"
			"movq (%%rax), %0\n"
			: "=c"(skb)
//			: "b"(time.tv_sec), "c"(time.tv_nsec)
			: "a"(offset)
			:
			);
			cpu = smp_processor_id();
			if(cpu >= CPU_NUM){
				printk(KERN_ALERT "CPU NUM is less than the cpu core numbers\n");
				goto out;
			}
			if(search((unsigned long)(skb->head), &idx, cpu)==0){
				// find 
			//	if(my_func_table[i].flag == 2){
			//		remove_hash(cpu, idx, skb);
			//		if(time.tv_nsec < SAMPLE_RATIO){
			//			output(skb, &time, i);
			//			insert_hash(cpu, idx, skb);
			//		}
			//		goto out;
			//	}
				output(skb, &time, i);

				if(my_func_table[i].flag == 3 || my_func_table[i].flag==4){
					remove_hash(cpu, idx, skb);
				}
	
			}else{
				// not found 
				if(my_func_table[i].flag == 1 ||  my_func_table[i].flag ==2){
					if(time.tv_nsec < SAMPLE_RATIO){
						
						if ( !check_frame(skb) )
							goto out;
						//output
						output(skb, &time, i);

						//insert into hash
						insert_hash(cpu, idx, skb);
					}
				}
			}

			goto out;
		}
	}
	//printk(KERN_INFO "start func is %p function ret is %p timestamp %p.%p\n", func, function_ret, time.tv_sec, time.tv_nsec); 
//	__asm__ __volatile__(
//	"movq $0, 0x70(%%rbp);"		// set the status to 0
//	:::
//	);	
out: 
	;
#endif
}

static void code_modify(struct func_table* func, unsigned long target_addr){

	unsigned long addr = func -> addr;
	u32 offset;
	if(addr==0)
		return;
	offset = target_addr - 5 - addr;
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
		code_modify( &(my_func_table[i]), (unsigned long)my_pre_handler);
	}
		
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
