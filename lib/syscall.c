#include "syscall.h"
#include "printk.h"
#include "proc.h"

extern struct task_struct * current;

uint64_t sys_write(unsigned int fd, const char* buf, size_t count) {
    uint64_t right_cnt = 0;
    for( ; right_cnt < count; right_cnt++ ) {
        putc(buf[right_cnt]);
    }
    return right_cnt;
}

uint64_t sys_getpid(void) {
    return (uint64_t)(current->pid);
}