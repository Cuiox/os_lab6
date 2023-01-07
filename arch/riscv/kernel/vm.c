// arch/riscv/kernel/vm.c
#include "vm.h"

extern _stext, _etext, _srodata, _erodata, _sdata, _edata, _sbss, _ebss;

void setup_vm_final(void);
//void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);

/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
unsigned long  early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void setup_vm(void) {
    /* 
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表 
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */
    // PHY_START = 0x0000000080000000
    // >> 30 : 1GB
    // >> 12 : 4KB page size
    // << 10 : 10bit flags
    // 0xf : V | R | W | X = 1 1 1 1
    early_pgtbl[(PHY_START >> 30)%512] = ((PHY_START >> 12) << 10) + 0xf;
    early_pgtbl[((PHY_START+PA2VA_OFFSET) >> 30)%512] = ((PHY_START >> 12) << 10) + 0xf;

    return;
}

// arch/riscv/kernel/vm.c 

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

void setup_vm_final(void) {
    memset(swapper_pg_dir, 0x0, PGSIZE);

    // No OpenSBI mapping required

    // mapping kernel text X|-|R|V
    create_mapping(swapper_pg_dir, (uint64)&_stext, (uint64)&_stext - PA2VA_OFFSET, (uint64)&_etext - (uint64)&_stext, 0b1011);

    // mapping kernel rodata -|-|R|V
    create_mapping(swapper_pg_dir, (uint64)&_srodata, (uint64)&_srodata - PA2VA_OFFSET, (uint64)&_erodata - (uint64)&_srodata, 0b0011);

    // mapping other memory -|W|R|V
    //create_mapping(swapper_pg_dir, (uint64)&_sdata, (uint64)&_sdata - PA2VA_OFFSET, VM_START + PHY_START - (uint64)&_sdata, 0b0111);
    //create_mapping(swapper_pg_dir, (uint64)&_sdata, (uint64)&_sdata - PA2VA_OFFSET, (uint64)_ebss - (uint64)_sbss + (uint64)_edata - (uint64)&_sdata, 0b0111);
    create_mapping(swapper_pg_dir, (uint64)&_sdata, (uint64)&_sdata - PA2VA_OFFSET, VM_START + PHY_SIZE - (uint64)&_sdata, 0b0111);
    
    // set satp with swapper_pg_dir
    
    __asm__ volatile (
        "mv t0, %[swapper_pg_dir]\n"
        
        "lui t1, 0xfffff\n"
        "srli t1, t1, 4\n"
        "addi t1, t1, 0xfd\n"
        "slli t1, t1, 8\n"
        "addi t1, t1, 0xf8\n"
        "slli t1, t1, 28\n"

        "sub t0, t0, t1\n"
        "slli t0, t0, 8\n"
        "srli t0, t0, 20\n"
        "addi t1, x0, 1\n"
        "slli t1, t1, 63\n"
        "add t0, t0, t1\n"
        "csrw satp, t0\n"
        :: [swapper_pg_dir] "r" (swapper_pg_dir)
        : "memory" 
    );

    // flush TLB
    asm volatile("sfence.vma zero, zero");

    // flush icache
    asm volatile("fence.i");

    printk("...setup_vm done!\n");

    return;
}


/* 创建多级页表映射关系 */
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小
    perm 为映射的读写权限

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
    uint64 VPN_START = (va / PGSIZE) % (0x1 << 27);     // first VPN  
    uint64 VPN_END = ((va+sz-1) / PGSIZE) % (0x1 << 27);// last VPN
    uint64 PPN_START = (pa / PGSIZE);
    for(uint64 i = VPN_START; i <= VPN_END; i++){
        uint64 VPN[3]; // 三级VPN
        VPN[0] = i % (0x1 << 9);
        VPN[1] = (i % (0x1 << 18)) / (0x1 << 9);
        VPN[2] = i / (0x1 << 18);

        uint64 * pgtbl_temp = pgtbl; // pgtbl_temp: 当前的page table
        uint64 * pgtbl_next;         // pgtbl_next: 下一级page table
        uint64 entry;                // 从 page table 中拿出的 entry

        for(int j = 2; j > 0; j--){
            entry = pgtbl_temp[VPN[j]];
            if((entry % 2) == 1){ // V = 1
                //                               10bit flags | PPN*PGSIZE
                pgtbl_next = (uint64 *)((entry / (0x1 << 10)) * PGSIZE + PA2VA_OFFSET);
            }else{ // page fault
                pgtbl_next = (uint64 *)kalloc();  // PPN                                flag  V = 1
                pgtbl_temp[VPN[j]] = ((( (uint64)pgtbl_next - PA2VA_OFFSET ) / PGSIZE) << 10) + 1;
            }
            pgtbl_temp = pgtbl_next;
        }                   // PPN_START + Offset                  flag
        pgtbl_temp[VPN[0]] = ((PPN_START + i - VPN_START) << 10) + perm;
    }

    return;
}

/*
ffffffe000000000 VM_START
0000000080000000 PHY_START
ffffffdf80000000 PA2VA_OFFSET
    "addi t1, x0, 0xff\n"
    "slli t1, t1, 8\n"
    "addi t1, t1, 0xff\n"
    "slli t1, t1, 8\n"
    "addi t1, t1, 0xff\n"
    "slli t1, t1, 8\n"
    "addi t1, t1, 0xdf\n"
    "slli t1, t1, 8\n"
    "addi t1, t1, 0x80\n"
    "slli t1, t1, 24\n"

    "lui t1, 0xfffff\n"
    "srli t1, t1, 4\n"
    "addi t1, t1, 0xfd\n"
    "slli t1, t1, 8\n"
    "addi t1, t1, 0xf8\n"
    "slli t1, t1, 28\n"
*/

/*
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
   
    uint64 VPN_START = (va / PGSIZE) % 0x8000000;
    uint64 VPN_END = ((va+sz-1) / PGSIZE) % 0x8000000;
    uint64 PPN_START = (pa / PGSIZE);
    for(uint64 i = VPN_START; i <= VPN_END; i++){
        uint64 VPN[3];
        //VPN[0] = (i % (1 << 9) );
        //VPN[1] = (i % (1 << 18)) / (1 << 9);
        //VPN[2] = (i % (1 << 27)) / (1 << 18);
        VPN[0] = i % 0x200;
        VPN[1] = (i % 0x40000) / 0x200;
        VPN[2] = i / 0x40000;

        uint64 * pgtbl_temp = pgtbl;
        uint64 * pgtbl_next;
        uint64 entry = 0;

        for(int j = 2; j > 0; j--){
            entry = pgtbl_temp[VPN[j]];
            if((entry % 2) == 1){ // V = 1
                //                               10bit flags | PPN*PGSIZE
                pgtbl_next = (uint64 *)((entry / (1 << 10)) * PGSIZE);
            }else{ // page fault
                pgtbl_next = (uint64 *)kalloc();
                pgtbl_temp[VPN[j]] = (( (uint64)pgtbl_next - PA2VA_OFFSET ) / (PGSIZE)) * (1 << 9) + 1;
            }
            pgtbl_temp = pgtbl_next;
        }
        pgtbl_temp[VPN[0]] = ((PPN_START + i - VPN_START) << 10) + perm;
    }

    return;
}
void *memcpy(void *dst, void *src, unsigned long n) {
    char *cdst = (char *)dst;
    char *csrc = (char *)src;
    for (unsigned long i = 0; i < n; ++i) 
        cdst[i] = csrc[i];

    return dst;
}
*/

void * memcpy(void *dst, void *src, uint64 n)
{
    char *cdst = (char *)dst;
    char *csrc = (char *)src;
    if((dst>(src+n)) || (dst<src)){
        while(n--){
            *cdst++ = *csrc++;
        }
    }else{
        cdst = (char *)(dst+n-1);
        csrc = (char *)(src+n-1);
        while(n--){
            *cdst-- = *csrc--;
        }
    }

    return dst;
}
