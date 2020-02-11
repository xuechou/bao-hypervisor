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

#include <arch/gic.h>
#include <vm.h>

volatile gicd_t gicd __attribute__((section(".devices"), aligned(PAGE_SIZE)));
volatile gicc_t gicc __attribute__((section(".devices"), aligned(PAGE_SIZE)));

static spinlock_t gicd_lock;


static inline void gicc_init()
{
    gicc.PMR = -1;
    gicc.CTLR |= GICC_CTLR_EN_BIT | GICC_CTLR_EOImodeNS_BIT;
}

inline void gic_cpu_init()
{
    for (int i = 0; i < GIC_NUM_INT_REGS(GIC_CPU_PRIV); i++) {
        /**
         * Make sure all private interrupts are not enabled, non pending,
         * non active.
         */
        gicd.ICENABLER[i] = -1;
        gicd.ICPENDR[i] = -1;
        gicd.ICACTIVER[i] = -1;
    }

    /* Clear any pending SGIs. */
    for (int i = 0; i < GIC_NUM_SGI_REGS; i++) {
        gicd.CPENDSGIR[i] = -1;
    }

    for (int i = 0; i < GIC_NUM_PRIO_REGS(GIC_CPU_PRIV); i++) {
        gicd.IPRIORITYR[i] = -1;
    }

    gicc_init();
}

inline void gic_init()
{
    size_t int_num = gic_num_irqs();

    /* Bring distributor to known state */
    for (int i = GIC_NUM_PRIVINT_REGS; i < GIC_NUM_INT_REGS(int_num); i++) {
        /**
         * Make sure all interrupts are not enabled, non pending,
         * non active.
         */
        gicd.ICENABLER[i] = -1;
        gicd.ICPENDR[i] = -1;
        gicd.ICACTIVER[i] = -1;
    }

    /* All interrupts have lowest priority possible by default */
    for (int i = GIC_CPU_PRIV; i < GIC_NUM_PRIO_REGS(int_num); i++)
        gicd.IPRIORITYR[i] = -1;

    /* No CPU targets for any interrupt by default */
    for (int i = GIC_CPU_PRIV; i < GIC_NUM_TARGET_REGS(int_num); i++)
        gicd.ITARGETSR[i] = 0;

    /* ICFGR are platform dependent, leave them as is */

    /* Enable distributor */
    gicd.CTLR |= GICD_CTLR_EN_BIT;
}

void gic_fiq_handle()
{
    uint64_t ack = gic_fiq_ack();
    uint64_t id = bit_extract(ack, GICC_IAR_ID_OFF, GICC_IAR_ID_LEN);
    uint64_t src = bit_extract(ack, GICC_IAR_CPU_OFF, GICC_IAR_CPU_LEN);

    if (id >= 1022) return;

    interrupts_handle(id, src);
    gic_fiq_end(id);
}

void gicd_send_sgi(uint64_t sgi, uint8_t trgt, uint8_t filter)
{
    gicd.SGIR = (sgi & GICD_SGIR_SGIINTID_MSK) | GICD_SGIR_CPUTRGLST_MSK(trgt) |
                GICD_SGIR_TRGLSTFLT_MSK(filter);
}

uint64_t gicd_get_prio(uint64_t int_id)
{
    uint64_t reg_ind = GIC_PRIO_REG(int_id);
    uint64_t off = GIC_PRIO_OFF(int_id);

    spin_lock(&gicd_lock);

    uint64_t prio =
        gicd.IPRIORITYR[reg_ind] >> off & BIT_MASK(off, GIC_PRIO_BITS);

    spin_unlock(&gicd_lock);

    return prio;
}

void gicd_set_prio(uint64_t int_id, uint8_t prio)
{
    uint64_t reg_ind = GIC_PRIO_REG(int_id);
    uint64_t off = GIC_PRIO_OFF(int_id);
    uint64_t mask = BIT_MASK(off, GIC_PRIO_BITS);

    spin_lock(&gicd_lock);

    gicd.IPRIORITYR[reg_ind] =
        (gicd.IPRIORITYR[reg_ind] & ~mask) | ((prio << off) & mask);

    spin_unlock(&gicd_lock);
}

enum int_state gicd_get_state(uint64_t int_id)
{
    uint64_t reg_ind = GIC_INT_REG(int_id);
    uint64_t mask = GIC_INT_MASK(int_id);

    spin_lock(&gicd_lock);

    enum int_state pend = (gicd.ISPENDR[reg_ind] & mask) ? PEND : 0;
    enum int_state act = (gicd.ISACTIVER[reg_ind] & mask) ? ACT : 0;

    spin_unlock(&gicd_lock);

    return pend | act;
}

static void gicd_set_pend(uint64_t int_id, bool pend)
{
    spin_lock(&gicd_lock);

    if (gic_is_sgi(int_id)) {
        uint64_t reg_ind = GICD_SGI_REG(int_id);
        uint64_t off = GICD_SGI_OFF(int_id);

        if (pend) {
            gicd.SPENDSGIR[reg_ind] = (1U) << (off + cpu.id);
        } else {
            gicd.CPENDSGIR[reg_ind] = BIT_MASK(off, 8);
        }
    } else {
        uint64_t reg_ind = GIC_INT_REG(int_id);

        if (pend) {
            gicd.ISPENDR[reg_ind] = GIC_INT_MASK(int_id);
        } else {
            gicd.ICPENDR[reg_ind] = GIC_INT_MASK(int_id);
        }
    }

    spin_unlock(&gicd_lock);
}

static void gicd_set_act(uint64_t int_id, bool act)
{
    uint64_t reg_ind = GIC_INT_REG(int_id);

    spin_lock(&gicd_lock);

    if (act) {
        gicd.ISACTIVER[reg_ind] = GIC_INT_MASK(int_id);
    } else {
        gicd.ICACTIVER[reg_ind] = GIC_INT_MASK(int_id);
    }

    spin_unlock(&gicd_lock);
}

inline void gicd_set_state(uint64_t int_id, enum int_state state)
{
    gicd_set_act(int_id, state & ACT);
    gicd_set_pend(int_id, state & PEND);
}

void gicd_set_trgt(uint64_t int_id, uint8_t trgt)
{
    uint64_t reg_ind = GIC_TARGET_REG(int_id);
    uint64_t off = GIC_TARGET_OFF(int_id);
    uint32_t mask = BIT_MASK(off, GIC_TARGET_BITS);

    spin_lock(&gicd_lock);

    gicd.ITARGETSR[reg_ind] =
        (gicd.ITARGETSR[reg_ind] & ~mask) | ((trgt << off) & mask);

    spin_unlock(&gicd_lock);
}

uint8_t gicd_get_trgt(uint64_t int_id)
{
    uint64_t reg_ind = GIC_TARGET_REG(int_id);
    uint64_t off = GIC_TARGET_OFF(int_id);

    spin_lock(&gicd_lock);

    uint8_t ret = gicd.ITARGETSR[reg_ind] & BIT_MASK(off, GIC_TARGET_BITS);

    spin_unlock(&gicd_lock);

    return ret;
}

void gicd_set_cfgr(uint64_t int_id, uint8_t cfgr)
{
    uint64_t reg_ind = GIC_CONFIG_REG(int_id);
    uint64_t off = GIC_CONFIG_OFF(int_id);
    uint32_t mask = BIT_MASK(off, GIC_CONFIG_BITS);

    spin_lock(&gicd_lock);

    gicd.ICFGR[reg_ind] =
        (gicd.ICFGR[reg_ind] & ~mask) | ((cfgr << off) & mask);

    spin_unlock(&gicd_lock);
}

uint8_t gicd_get_cfgr(uint64_t int_id)
{
    uint64_t reg_ind = GIC_CONFIG_REG(int_id);
    uint64_t off = GIC_CONFIG_OFF(int_id);

    spin_lock(&gicd_lock);

    uint8_t ret = gicd.ICFGR[reg_ind] & BIT_MASK(off, GIC_CONFIG_BITS);

    spin_unlock(&gicd_lock);

    return ret;
}

void gicd_set_enable(uint64_t int_id, bool en)
{
    uint64_t reg_ind = GIC_INT_REG(int_id);
    uint64_t bit = GIC_INT_MASK(int_id);

    spin_lock(&gicd_lock);

    if (en) {
        gicd.ISENABLER[reg_ind] = bit;
    } else {
        gicd.ICENABLER[reg_ind] = bit;
    }

    spin_unlock(&gicd_lock);
}

bool gicd_enabled(uint64_t int_id)
{
    uint64_t reg_ind = GIC_INT_REG(int_id);
    uint64_t bit = GIC_INT_MASK(int_id);

    spin_lock(&gicd_lock);

    uint64_t enabled = gicd.ISENABLER[reg_ind] & bit;

    spin_unlock(&gicd_lock);

    return enabled;
}
