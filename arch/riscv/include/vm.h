#include "defs.h"
#include "types.h"
#include "mm.h"
#include "string.h"


void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);
void * memcpy(void *dst, void *src, uint64 n);
