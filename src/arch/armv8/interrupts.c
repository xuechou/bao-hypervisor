/**
 * Bao, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *      Angelo Ruocco <angeloruocco90@gmail.com>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#include <bao.h>
#include <interrupts.h>

#include <cpu.h>
#include <platform.h>
#include <arch/gic.h>
#include <mem.h>
#include <arch/sysregs.h>
#include <vm.h>

inline void interrupts_arch_init()
{
    if (cpu.id == CPU_MASTER) {
        mem_map_dev(&cpu.as, (void *)&gicc, platform.arch.gic.gicc_addr,
                    NUM_PAGES(sizeof(gicc)));
        mem_map_dev(&cpu.as, (void *)&gicd, platform.arch.gic.gicd_addr,
                    NUM_PAGES(sizeof(gicd)));
    }

    /* Wait for core 0 to map interupt controller */
    cpu_sync_barrier(&cpu_glb_sync);

    if (cpu.id == CPU_MASTER) gic_init();

    gic_cpu_init();
}

inline void interrupts_arch_ipi_send(uint64_t target_cpu, uint64_t ipi_id)
{
    if (ipi_id < GIC_MAX_SGIS) gic_fiq_raise_sgi(target_cpu, ipi_id);
}

inline void interrupts_arch_cpu_enable(bool en)
{
    if (en)
        asm volatile("msr DAIFClr, %0\n" ::"I"(PSTATE_DAIF_I_BIT));
    else
        asm volatile("msr DAIFSet, %0\n" ::"I"(PSTATE_DAIF_I_BIT));
}

void interrupts_arch_enable(uint64_t id, bool en)
{
    uint8_t prio = 0x7F;

    gicd_set_enable(id, en);
    gicd_set_prio(id, prio);
    gicd_set_trgt(id, 1 << cpu.id);
    gic_set_as_fiq(id, prio);
}

inline bool interrupts_arch_check(uint64_t id)
{
    return gic_fiq_get_pend(id);
}

inline bool interrupts_arch_conflict(bitmap_t interrupt_bitmap, uint64_t id)
{
    return (id == IPI_CPU_MSG ||
            (bitmap_get(interrupt_bitmap, id) && id > GIC_CPU_PRIV));
}

inline void interrupts_arch_clear(uint64_t id) {}

/**
 * To reduce overhead, the virtual gic interface is not used, and the VMs are
 * allowed to directly access the cpu controller.
 */
inline void interrupts_arch_vm_assign(vm_t *vm, uint64_t id) {}
inline void interrupts_arch_vm_inject(vm_t *vm, uint64_t id, uint64_t src) {}
