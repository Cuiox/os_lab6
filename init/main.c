#include "printk.h"
#include "sbi.h"
#include "defs.h"

extern void test();

extern _stext, _etext, _srodata, _erodata;

int start_kernel() {
    printk("[S-MODE] 2022");
    printk(" Hello RISC-V\n");

    schedule(); // lab5
    test(); // DO NOT DELETE !!!
    
	return 0;
}

    //printk(".text start at \t= 0x%x%x\n", ((uint64)&_stext >> 32), (uint64)&_stext);
    //printk(".text end at \t= 0x%x%x\n", ((uint64)&_etext >> 32), (uint64)&_etext);
    //printk(".rodata start at= 0x%x%x\n", ((uint64)&_srodata >> 32), (uint64)&_srodata);
    //printk(".rodata end at \t= 0x%x%x\n", ((uint64)&_erodata >> 32), (uint64)&_erodata);

    //printk("now .text[0] = %ld\n", (&_stext)[0]);
    //*(&_stext) = 0x0;
    //printk("then .text[0] = %ld\n", (&_stext)[0]);

    //printk("now .rodata[0] = %ld\n", (&_srodata)[0]);
    //*(&_srodata) = 0x00;
    //printk("then .rodata[0] = %ld\n", (&_srodata)[0]);
