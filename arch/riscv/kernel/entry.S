    .extern trap_handler # extern the trap_handler function
    .section .text.entry
    .align 2
    .globl _traps 
_traps:
    # YOUR CODE HERE
    # -----------
    csrr t0, sscratch
    beq t0, x0, push # S-mode stack
    csrw sscratch, sp # switch
    add sp, t0, x0   # U-mode stack

        # 1. save 32 registers and sepc to stack
push:
    addi sp, sp, -272
    sd x0, 0(sp)
    sd x1, 8(sp)
    sd x2, 16(sp)
    sd x3, 24(sp)
    sd x4, 32(sp)
    sd x5, 40(sp)
    sd x6, 48(sp)
    sd x7, 56(sp)
    sd x8, 64(sp)
    sd x9, 72(sp)
    sd x10, 80(sp)
    sd x11, 88(sp)
    sd x12, 96(sp)
    sd x13, 104(sp)
    sd x14, 112(sp)
    sd x15, 120(sp)
    sd x16, 128(sp)
    sd x17, 136(sp)
    sd x18, 144(sp)
    sd x19, 152(sp)
    sd x20, 160(sp)
    sd x21, 168(sp)
    sd x22, 176(sp)
    sd x23, 184(sp)
    sd x24, 192(sp)
    sd x25, 200(sp)
    sd x26, 208(sp)
    sd x27, 216(sp)
    sd x28, 224(sp)
    sd x29, 232(sp)
    sd x30, 240(sp)
    sd x31, 248(sp)
    csrr t1, sepc
    sd t1, 256(sp)    # save sepc last
    # lab5
    csrr t0, sstatus
    sd t0, 264(sp)
    
    # -----------

        # 2. call trap_handler
    csrr a0, scause # a0 = scause
    mv a1, t1       # a1 = sepc
    add a2, sp, x0
    jal ra, trap_handler
    # -----------

        # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack
    ld t0, 264(sp)
    csrw sstatus, t0 # lab5
    ld t0, 256(sp)
    csrw sepc, t0 # restore sepc first 
    ld x0, 0(sp)
    ld x1, 8(sp)
    ld x2, 16(sp)
    ld x3, 24(sp)
    ld x4, 32(sp)
    ld x5, 40(sp)
    ld x6, 48(sp)
    ld x7, 56(sp)
    ld x8, 64(sp)
    ld x9, 72(sp)
    ld x10, 80(sp)
    ld x11, 88(sp)
    ld x12, 96(sp)
    ld x13, 104(sp)
    ld x14, 112(sp)
    ld x15, 120(sp)
    ld x16, 128(sp)
    ld x17, 136(sp)
    ld x18, 144(sp)
    ld x19, 152(sp)
    ld x20, 160(sp)
    ld x21, 168(sp)
    ld x22, 176(sp)
    ld x23, 184(sp)
    ld x24, 192(sp)
    ld x25, 200(sp)
    ld x26, 208(sp)
    ld x27, 216(sp)
    ld x28, 224(sp)
    ld x29, 232(sp)
    ld x30, 240(sp)
    ld x31, 248(sp)
    addi sp, sp, 272
        
    # -----------
    # sp switch
    csrr t0, sscratch
    beq t0, x0, return
    csrw sscratch, sp
    add sp, t0, x0

        # 4. return from trap
return:
    sret # return to sepc
    # -----------

#-------------------------
# --- lab3 ---
    .extern dummy
    .globl __dummy
__dummy:
    # lab5 start
    add t0, sp, x0
    csrr sp, sscratch
    csrw sscratch, t0
    # lab5 end
    #la t0, dummy
    #csrw sepc, t0
    
    sret

# arch/riscv/kernel/entry.S

    .globl __switch_to
__switch_to:
    # save state to prev process
    # YOUR CODE HERE
    # a0 : struct task_struct * prev
    # 0(a0)  : struct thread_info* thread_info
    # 8(a0)  : uint64 state;
    # 16(a0) : uint64 counter;
    # 24(a0) : uint64 priority;
    # 32(a0) : uint64 pid;
    # 40~(a0) : struct thread_struct thread;
    sd ra ,  40(a0)
    sd sp ,  48(a0)
    sd s0 ,  56(a0)
    sd s1 ,  64(a0)
    sd s2 ,  72(a0)
    sd s3 ,  80(a0)
    sd s4 ,  88(a0)
    sd s5 ,  96(a0)
    sd s6 , 104(a0)
    sd s7 , 112(a0)
    sd s8 , 120(a0)
    sd s9 , 128(a0)
    sd s10, 136(a0)
    sd s11, 144(a0)
    # lab5 start
    csrr t0, sepc
    sd t0, 152(a0)
    csrr t0, sstatus
    sd t0, 160(a0)
    csrr t0, sscratch
    sd t0, 168(a0)
    # lab5 end


    # restore state from next process
    # YOUR CODE HERE
    # a1 : struct task_struct * next
    ld ra ,  40(a1)
    ld sp ,  48(a1)
    ld s0 ,  56(a1)
    ld s1 ,  64(a1)
    ld s2 ,  72(a1)
    ld s3 ,  80(a1)
    ld s4 ,  88(a1)
    ld s5 ,  96(a1)
    ld s6 , 104(a1)
    ld s7 , 112(a1)
    ld s8 , 120(a1)
    ld s9 , 128(a1)
    ld s10, 136(a1)
    ld s11, 144(a1)
    # lab5 start
    ld t0, 152(a1)
    csrw sepc, t0
    ld t0, 160(a1)
    csrw sstatus, t0
    ld t0, 168(a1)
    csrw sscratch, t0
    ld t1, 176(a1)              # 176
    slli t1, t1, 8              # erase 39-55 # seems can be remove
    srli t1, t1, 20             # satp[43:0] = pa[12:38]
    addi t0, x0, 1              # set MODE
    slli t0, t0, 63     
    add t1, t1, t0              # satp[63] = 1
    csrw satp, t1
    # flush tlb
    sfence.vma zero, zero
    # flush icache
    fence.i

    # lab5 end

    ret



#-------------------------