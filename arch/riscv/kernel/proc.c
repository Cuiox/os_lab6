//arch/riscv/kernel/proc.c
#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "stdint.h"
#include "string.h"
#include "elf.h"

extern void __dummy();
extern void __switch_to(struct task_struct* prev, struct task_struct* next);
extern char uapp_start[]; // lab5
extern char uapp_end[]; 
extern unsigned long  swapper_pg_dir[];
extern void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此

void task_init();
void dummy();
void switch_to(struct task_struct* next);
void do_timer(void);
void schedule(void);
static uint64_t load_binary(struct task_struct* task);
static uint64_t load_program(struct task_struct* task);


static uint64_t load_binary(struct task_struct* task)
{
    uint64 pgd_va = (uint64)alloc_page();
    task->pgd = pgd_va - PA2VA_OFFSET; // set pgd pa
    memcpy(pgd_va, swapper_pg_dir, PGSIZE); // copy, in vm.c
    uint64 u_stack_va = (uint64)alloc_page(); // u-mode stack
    create_mapping(pgd_va, USER_END - PGSIZE, u_stack_va - PA2VA_OFFSET, PGSIZE, 0x17);
    uint64 uapp_size = uapp_end - uapp_start;
    uint64_t uapp_PN = uapp_size / PGSIZE + 1;
    uint64 uapp_va = (uint64)alloc_pages(uapp_PN);
    uint64 uapp_pa = uapp_va - PA2VA_OFFSET;
    memcpy(uapp_va, uapp_start, uapp_size);
    create_mapping(pgd_va, USER_START, uapp_pa, uapp_PN * PGSIZE, 0x1F);

    (task->thread).sepc = USER_START;
    (task->thread).sstatus = 0x40020;
    (task->thread).sscratch = USER_END;
}

static uint64_t load_program(struct task_struct* task) 
{
	uint64 pgd_va = (uint64)alloc_page();
	task->pgd = pgd_va - PA2VA_OFFSET;

	memcpy(pgd_va, swapper_pg_dir, PGSIZE);
	
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)uapp_start;
    uint64_t phdr_start = (uint64_t)ehdr + ehdr->e_phoff; // add offset
    int phdr_cnt = ehdr->e_phnum; // Segment Number

    Elf64_Phdr* phdr;
    int load_phdr_cnt = 0;
    //printk("phdr_cnt = %d\n", phdr_cnt);
    for (int i = 0; i < phdr_cnt; i++) {
        //printk("here\n");
        phdr = (Elf64_Phdr*)(phdr_start + sizeof(Elf64_Phdr) * i);
        if (phdr->p_type == PT_LOAD) {
            // printk("PT_LOAD\n");
            //printk("p_filesz = %x \t", phdr->p_filesz);
            //printk("p_memsz = %x \n",  phdr->p_memsz);
            uint64 seg_start = (uint64)ehdr + phdr->p_offset; // p_offset - Segment 在文件中相对于 Ehdr 的偏移量
			uint64 seg_end = seg_start + phdr->p_memsz*8; // p_memsz - Segment 在内存中占的大小
			uint64 seg_pgn = (seg_end-seg_start)/PGSIZE + 1;
			uint64 seg_va = (uint64)alloc_pages(seg_pgn);
			uint64 seg_pa = seg_va - PA2VA_OFFSET;
            
			memcpy(seg_va, seg_start/PGSIZE * PGSIZE, seg_pgn*PGSIZE);
			
            uint64 flags = phdr->p_flags; // p_flags - Segment 的权限（包括了读、写和执行）
			uint64 perm = ((flags & 0x1)<<3) + ((flags & 0x2)<<1) + ((flags & 0x4)>>1) + 0x11;
			create_mapping(pgd_va, ((phdr->p_vaddr)/PGSIZE) * PGSIZE, seg_pa, seg_pgn*PGSIZE, perm);
            //printk("there\n");
        }
    }
    //printk("here done\n");
	//allocate user stack and do mapping
	uint64 Ustack_PA = (uint64)alloc_page() - PA2VA_OFFSET;
	create_mapping(pgd_va, USER_END-PGSIZE, Ustack_PA, PGSIZE, 0x17);
    // following code has been written for you set user stack
    task->thread.sepc = ehdr->e_entry;
    // sstatus bits set
    task->thread.sstatus = 0x40020;
    // user stack for user program
    task->thread.sscratch = USER_END;
}

void task_init() {
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 task[0] 指向 idle
    
    /* YOUR CODE HERE */
    idle = kalloc(); // return 指向物理页的指针
    //idle = (struct task_struct *)idle;
    idle->state = TASK_RUNNING;
    idle->counter = 0;
    idle->priority = 0;
    idle->pid = 0;
    current = idle;
    task[0] = idle;    

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, counter 为 0, priority 使用 rand() 来设置, pid 为该线程在线程数组中的下标。
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`,
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址,  `sp` 设置为 该线程申请的物理页的高地址

    /* YOUR CODE HERE */
    for(uint64 i = 1; i < NR_TASKS; i++){
        task[i] = kalloc();
        task[i]->state = TASK_RUNNING;
        task[i]->counter = 0;
        task[i]->priority = rand();
        task[i]->pid = i;
        (task[i]->thread).ra = (uint64)__dummy;
        (task[i]->thread).sp = (uint64)task[i] + PGSIZE;
        // lab5 start
        // uint64 pgd_va = (uint64)alloc_page();
        // task[i]->pgd = pgd_va - PA2VA_OFFSET; // set pgd pa
        // memcpy(pgd_va, swapper_pg_dir, PGSIZE); // copy, in vm.c
        // uint64 u_stack_va = (uint64)alloc_page(); // u-mode stack
        // create_mapping(pgd_va, USER_END - PGSIZE, u_stack_va - PA2VA_OFFSET, PGSIZE, 0x17);
        // uint64 uapp_size = uapp_end - uapp_start;
        // uint64_t uapp_PN = uapp_size / PGSIZE + 1;
        // uint64 uapp_va = (uint64)alloc_pages(uapp_PN);
        // uint64 uapp_pa = uapp_va - PA2VA_OFFSET;
        // memcpy(uapp_va, uapp_start, uapp_size);
        // create_mapping(pgd_va, USER_START, uapp_pa, uapp_PN * PGSIZE, 0x1F);

        // (task[i]->thread).sepc = USER_START;
        // (task[i]->thread).sstatus = 0x40020;
        // (task[i]->thread).sscratch = USER_END;
        load_program(task[i]);
        // lab5 end
        
    }

    printk("...proc_init done!\n");
    printk("We have [NR_TASKS = %d] tasks\n", NR_TASKS); 
}

// arch/riscv/kernel/proc.c

void dummy() { 
    printk("hello\n");
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. thread space begin at %x%x\n", 
                current->pid, ((uint64)current >> 32), (uint64)current);
        }
    }
}

// arch/riscv/kernel/proc.c

//extern void __switch_to(struct task_struct* prev, struct task_struct* next);

void switch_to(struct task_struct* next) {
    /* YOUR CODE HERE */
    if(current->pid != next->pid){
        struct task_struct * switch_out = current;
        current = next;  
        printk("Switch happen: from [PID = %d] to [PID = %d]\n", switch_out->pid, next->pid);
        __switch_to(switch_out, next);
        
    }else{
        printk("Switch remain: [PID = %d]\n", current->pid);
    }
}

// arch/riscv/kernel/proc.c

void do_timer(void) {
    // 1. 如果当前线程是 idle 线程 直接进行调度
    // 2. 如果当前线程不是 idle 对当前线程的运行剩余时间减1 若剩余时间仍然大于0 则直接返回 否则进行调度

    /* YOUR CODE HERE */
    //printk("---It's time to do something---\n");
    if(current->pid == idle->pid){
        //printk("---   It's first time ha~   ---\n");
        schedule();
    }else{
        // current->counter is unsiged int 64
        if(current->counter == 0){
            //printk("--- counter=0!? schedule it ---\n");        
            schedule();
        }else{
            //printk("--- counter=%d   fuck it up! ---\n", current->counter); 
            current->counter--;
        }
    }
    return;
}

// arch/riscv/kernel/proc.c

void schedule(void) {
    /* YOUR CODE HERE */
#ifdef SJF 
// SJF
    uint64 min_counter = 2147483647;
    uint64 next_task_index = 0;
    for(uint64 i = 1; i < NR_TASKS; i++){
        if(task[i]->counter > 0 && task[i]->counter < min_counter){
            min_counter = task[i]->counter;
            next_task_index = i;
        }
    }
    if(next_task_index == 0){
        //printk("! omg no positive counters, rand it !\n");
        for(uint64 i = 1; i < NR_TASKS; i++){
            task[i]->counter = rand();
        }
        for(uint64 i = 1; i < NR_TASKS; i++){
            printk("[PID = %d] has [COUNTER = %d]\n", 
                task[i]->pid, task[i]->counter);
        }
        schedule();
    }else{
        printk("schedule choose [PID = %d] with min [COUNTER = %d]\n", 
                task[next_task_index]->pid, task[next_task_index]->counter);
        switch_to(task[next_task_index]);
    }
    return;
#else 
//PRIORITY
    int c = 0; 
    int next = 0;
    int i = 0;
    struct task_struct ** p;
    while(1){
        c = -1;
        next = 0;
        i = NR_TASKS;
        p = &task[NR_TASKS];
        while(--i){
            if(!*--p){
                continue;
            }
            if((*p)->state == TASK_RUNNING && (int)(*p)->counter > c){
                c = (*p)->counter;
                next = i; // counter 越大 越优先
            }
        }
        if(c){
            break;
        }
        for(uint64 j = 1; j < NR_TASKS; j++){
            if(task[j]){
                task[j]->counter = (task[j]->counter >> 1) + task[j]->priority;
            }
        }
    }
    printk("schedule choose [PID = %d] [PRIORITY = %d] [COUNTER = %d]\n", 
            task[next]->pid, task[next]->priority, task[next]->counter);
    switch_to(task[next]);
    return;
#endif

}

