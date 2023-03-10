#include "types.h"
#include "sbi.h"


struct sbiret sbi_ecall(int ext, int fid, uint64 arg0,
			            uint64 arg1, uint64 arg2,
			            uint64 arg3, uint64 arg4,
			            uint64 arg5) 
{
    // unimplemented           
    struct sbiret ret;
	__asm__ volatile (
		"lw a7, %[ext]\n"
		"lw a6, %[fid]\n"
		"ld a5, %[arg5]\n"
		"ld a4, %[arg4]\n"
		"ld a3, %[arg3]\n"
		"ld a2, %[arg2]\n"
		"ld a1, %[arg1]\n"
		"ld a0, %[arg0]\n"
		"ecall\n"
		"mv %[ret_error], a0\n"
		"mv %[ret_value], a1\n"
		: [ret_error] "=r" (ret.error), [ret_value] "=r" (ret.value)
		: [ext] "m" (ext), [fid] "m" (fid), 
		  [arg0] "m" (arg0), [arg1] "m" (arg1), 
		  [arg2] "m" (arg2), [arg3] "m" (arg3), 
		  [arg4] "m" (arg4), [arg5] "m" (arg5)
		: "memory" 
	);

	return ret;
}
