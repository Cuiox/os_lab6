#include "defs.h"

/* 目标架构 */
OUTPUT_ARCH( "riscv" )

/* 程序入口 */
ENTRY( _start )

MEMORY {
    /* ram 代表 LMA ( Load Memory Address ), 即我们 OS image 被 load 的地址，可以理解为物理地址 */
    ram  (wxa!ri): ORIGIN = PHY_START + OPENSBI_SIZE, LENGTH = PHY_SIZE - OPENSBI_SIZE
    /* ramv 代表 VMA ( Virtual Memory Address ) 即虚拟地址 */
    ramv (wxa!ri): ORIGIN = VM_START + OPENSBI_SIZE, LENGTH = VM_SIZE - OPENSBI_SIZE
}

/* kernel代码起始位置 */
BASE_ADDR = VM_START + OPENSBI_SIZE;

SECTIONS
{
    /* . 代表当前地址 */
    . = BASE_ADDR;

    /* 记录kernel代码的起始地址 */
    _skernel = .;

    /* ALIGN(0x1000) 表示4KB对齐 */
    /* _stext, _etext 分别记录了text段的起始与结束地址 */
    .text : ALIGN(0x1000){
        _stext = .;

        *(.text.init)
        *(.text.entry)
        *(.text .text.*)
        
        _etext = .;
    } >ramv AT>ram

    .rodata : ALIGN(0x1000){
        _srodata = .;

        *(.srodata .srodata.*)
        *(.rodata .rodata.*)
        
        _erodata = .;
    } >ramv AT>ram

    /* .data : ALIGN(0x1000){
        _sdata = .;

        *(.sdata .sdata*)
        *(.data .data.*)
        
        _edata = .;
    } >ramv AT>ram */

    .data : ALIGN(0x1000){
        _sdata = .;

        *(.sdata .sdata*)
        *(.data .data.*)

        _edata = .;

        . = ALIGN(0x1000);
        uapp_start = .; /* added in lab05 */
        *(.uapp .uapp*)
        uapp_end = .;
        . = ALIGN(0x1000);

    } >ramv AT>ram

    .bss : ALIGN(0x1000){
        _sbss = .;

        *(.bss.stack)
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        
        _ebss = .;
    } >ramv AT>ram

    /* 记录kernel代码的结束地址 */
    _ekernel = .;
}
