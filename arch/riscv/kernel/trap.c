// trap.c 
//#include"printk.h"
//#include"clock.c"
#include "stdint.h"
#include "syscall.h"

struct pt_regs {
    uint64_t x[32];
    uint64_t sepc;
    uint64_t sstatus;
    uint64_t stval;
    uint64_t sscratch;
    uint64_t scause;
};

    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.5 节
    // 其他interrupt / exception 可以直接忽略

void trap_handler(unsigned long scause, unsigned long sepc, struct pt_regs *regs) 
{
    if(scause >= 0x8000000000000000){
        if(scause == 0x8000000000000005){
            //printk("[S] Superviosr Timer Interrupt\n");
            clock_set_next_event();
            do_timer();
        }else{
            printk("Not Timer Interrupt\n");
        }
    }else{
        // exception
        if(scause == 0x8){
            // ECALL_FROM_U_MODE
            uint64_t sys_call_num = regs->x[17];
            if(sys_call_num == SYS_WRITE){
                uint64_t fd     = regs->x[10];
                uint64_t buf    = regs->x[11];
                uint64_t count  = regs->x[12];
                sys_write((unsigned int)fd, (const char *)buf, (size_t)count);
            }else if(sys_call_num == SYS_GETPID){
                regs->x[10]     = sys_getpid();
            }else{
                printk("[S] Unhandled syscall: %lx", sys_call_num);
                while (1);
            }
            regs->sepc += 4;
        }else{
            printk("[S] Unhandled trap, ");
            printk("scause: %lx, ", scause);
            printk("stval: %lx, ", regs->stval);
            printk("sepc: %lx\n", regs->sepc);
            while (1);
        }
    }
 
    return;
    // YOUR CODE HERE
}
