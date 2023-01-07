.extern start_kernel
.extern mm_init
.extern task_init
.extern setup_vm # lab4
.extern early_pgtbl # lab4
#.extern PA2VA_OFFSET # lab4
.extern setup_vm_final # lab4
    .section .text.init
    .globl _start
_start:
    # YOUR CODE HERE

        # push
    la sp, boot_stack_top 
    ld t0, offset # lab4
    sub sp, sp, t0 # lab4

    # ------------------
    # --- lab4 code ----
    call setup_vm
    call relocate
    # ------------------  

    # ------------------
    # --- lab3 code ----
    #jal ra, mm_init
    call mm_init
    call setup_vm_final # lab4
    jal ra, task_init
    # ------------------  

    addi sp, sp, -64
    sd a0, 0(sp)
    sd a1, 8(sp)
    sd a2, 16(sp)
    sd a3, 24(sp)
    sd a4, 32(sp)
    sd a5, 40(sp)
    sd a6, 48(sp)
    sd a7, 56(sp)
    # ------------------
    
        # set stvec = _traps
    la a0, _traps # _traps is globl tag
    csrw stvec, a0
    # ------------------
    
        # set sie[STIE] = 1
    csrr a0, sie
    ori a0, a0, 32 # sie[5] = 1
    csrw sie, a0
    # ------------------

        # set first time interrupt
    lui a0, 0x989
    addi a0, a0, 0x680 # a0 = 10000000 = 0x989680
    rdtime a1
    add a0, a0, a1 # a0 = next = get_cycles() + TIMECLOCK;
    andi a1, a1, 0 # a1 ~ a7 = 0
    andi a2, a2, 0
    andi a3, a3, 0
    andi a4, a4, 0
    andi a5, a5, 0
    andi a6, a6, 0
    andi a7, a7, 0
    ecall # sbi_ecall(0, 0, a0, 0, 0, 0, 0, 0);
    
    # ------------------

        # set sstatus[SIE] = 1
    # csrr a0, sstatus
    # ori a0, a0, 2 # sstatus[1] = 1
    # csrw sstatus, a0
    # ------------------

        # pop
    ld a0, 0(sp)
    ld a1, 8(sp)
    ld a2, 16(sp)
    ld a3, 24(sp)
    ld a4, 32(sp)
    ld a5, 40(sp)
    ld a6, 48(sp)
    ld a7, 56(sp)
    addi sp, sp, 64

     

    # ------------------
    # - your lab1 code here -
    jal x0, start_kernel
    # ------------------

# ------------------
# head.S lab4
relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)

    ld t0, offset
    add ra, ra, t0
    add sp, sp, t0

    # set satp with early_pgtbl

    la t1, early_pgtbl  # va(virtual address)
    sub t1, t1, t0      # pa(physical address)
    slli t1, t1, 8      # erase 39-55 # seems can be remove
    srli t1, t1, 20     # satp[43:0] = pa[12:38]
    addi t0, x0, 1      # set MODE
    slli t0, t0, 63
    add t1, t1, t0      # satp[63] = 1
    csrw satp, t1

    # flush tlb
    sfence.vma zero, zero

    # flush icache
    fence.i

    ret
# ------------------
    .section .data.constant
offset:
    .dword 0xffffffdf80000000
# ------------------
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top:
