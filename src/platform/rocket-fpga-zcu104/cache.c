
#include <bao.h>
#include <cache.h>
#include <arch/sbi.h>

#define SBI_EXTID_ROCKET    (0x09000000)
#define SBI_EXT_ROCKET_CFLUSHDLONE (0)

void cache_flush_range(void* base, uint64_t size)
{
    /**
     * Despite the actual semantics of this method being to flush all caches to
     * main memory, this function only flushes l1. It is just purposed for the 
     * cases of instruction and data cache synchronization across multiple harts
     * (so fence.i does not suffice) which we expect will be unified at the 
     * L2 cache for rocket-based platforms. If the need arises to use this 
     * method for somekind for coherency purposes with non-coherent bus-masters
     * it needs to be extended to guarantee a flush to the first point of 
     * coherency.
     */ 

    struct sbiret ret = sbi_ecall(SBI_EXTID_ROCKET, SBI_EXT_ROCKET_CFLUSHDLONE, 
        (uintptr_t) base, size, 0, 0, 0, 0);

    if(ret.error) {
        ERROR("cache_flush_range failed");
    }

    asm volatile("fence.i");
}