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

#include <arch/vgicv2.h>
#include <vm.h>

static inline bool vgic_is_owner(uint64_t id)
{
    return (id < GIC_MAX_PPIS && id != IPI_CPU_MSG) ||
           bitmap_get(cpu.vcpu->vm->interrupt_bitmap, id);
}

static uint32_t vgic_target_translate(vm_t *vm, uint32_t trgt, bool topcpu)
{
    union {
        uint32_t mask;
        uint8_t buf[4];
    } from, to;

    from.mask = trgt;
    to.mask = 0;

    for (int i = 0; i < sizeof(trgt) * 8 / GIC_TARGET_BITS; i++) {
        to.buf[i] =
            topcpu
                ? vm_translate_to_pcpu_mask(vm, from.buf[i], GIC_TARGET_BITS)
                : vm_translate_to_vcpu_mask(vm, from.buf[i], GIC_TARGET_BITS);
    }

    return to.mask;
}

static void vgicd_emul_misc_access(emul_access_t *acc)
{
    uint64_t reg = acc->addr & GIC_MISC_REG_MASK;

    if (acc->write) return;

    switch (reg) {
        case GICD_REG_IND(CTLR):
            vcpu_writereg(cpu.vcpu, acc->reg, gicd_get_ctlr());
            break;
        case GICD_REG_IND(TYPER):
            vcpu_writereg(cpu.vcpu, acc->reg, gicd_get_typer());
            break;
        case GICD_REG_IND(IIDR):
            vcpu_writereg(cpu.vcpu, acc->reg, gicd_get_iidr());
            break;
    }
}

static void vgicd_emul_ienabler_access(emul_access_t *acc, bool en)
{
    uint64_t reg_ind = (acc->addr & GIC_MISC_REG_MASK) / sizeof(uint32_t);
    uint32_t bit = vcpu_readreg(cpu.vcpu, acc->reg);
    uint64_t first_int = GIC_INT_REG_SIZE * reg_ind;

    for (int i = 0; i < GIC_INT_REG_SIZE; i++) {
        if (bit_get(bit, i)) {
            if (acc->write) {
                GICD_CALL(set_enable, i + first_int, en);
            } else {
                vcpu_writereg(cpu.vcpu, acc->reg,
                              GICD_CALL(enabled, i + first_int));
            }
        }
    }
}

static void vgicd_emul_istate_access(emul_access_t *acc, enum int_state state,
                                     bool en)
{
    uint64_t reg_ind = (acc->addr & GIC_MISC_REG_MASK) / sizeof(uint32_t);
    uint32_t bit = vcpu_readreg(cpu.vcpu, acc->reg);
    uint64_t first_int = GIC_INT_REG_SIZE * reg_ind;

    for (int i = 0; i < GIC_INT_REG_SIZE; i++) {
        if (bit_get(bit, i)) {
            if (acc->write) {
                GICD_CALL(set_state, i + first_int, state * en);
            } else {
                vcpu_writereg(cpu.vcpu, acc->reg,
                              GICD_CALL(get_state, i + first_int));
            }
        }
    }
}

static void vgicd_emul_ipriorityr_access(emul_access_t *acc)
{
    /* We look at prio regs as 8 bit registers */
    uint32_t val = acc->write ? vcpu_readreg(cpu.vcpu, acc->reg) : 0;
    uint64_t first_int = (8 / GIC_PRIO_BITS) * (acc->addr & GIC_PRIO_ADDR_MASK);

    if (acc->write) {
        for (int i = 0; i < acc->width; i++) {
            GICD_CALL(set_prio, first_int + i,
                      bit_extract(val, GIC_PRIO_BITS * i, GIC_PRIO_BITS));
        }
    } else {
        for (int i = 0; i < acc->width; i++) {
            val |= GICD_CALL(get_prio, first_int + i) << (GIC_PRIO_BITS * i);
        }
        vcpu_writereg(cpu.vcpu, acc->reg, val);
    }
}

static void vgicd_emul_itargetr_access(emul_access_t *acc)
{
    uint32_t val = acc->write ? vcpu_readreg(cpu.vcpu, acc->reg) : 0;
    uint64_t first_int =
        (8 / GIC_TARGET_BITS) * (acc->addr & GIC_TARGET_ADDR_MASK);

    if (acc->write) {
        val = vgic_target_translate(cpu.vcpu->vm, val, true);
        for (int i = 0; i < acc->width; i++) {
            GICD_CALL(set_trgt, first_int + i,
                      bit_extract(val, GIC_TARGET_BITS * i, GIC_TARGET_BITS));
        }
    } else {
        for (int i = 0; i < acc->width; i++) {
            val |= GICD_CALL(get_trgt, first_int + i) << (GIC_TARGET_BITS * i);
        }
        val = vgic_target_translate(cpu.vcpu->vm, val, false);
        vcpu_writereg(cpu.vcpu, acc->reg, val);
    }
}

static void vgicd_emul_icfgr_access(emul_access_t *acc)
{
    uint32_t val = acc->write ? vcpu_readreg(cpu.vcpu, acc->reg) : 0;
    uint64_t first_int =
        (8 / GIC_CONFIG_BITS) * (acc->addr & GIC_CONFIG_ADDR_MASK);

    if (acc->write) {
        for (int i = 0; i < acc->width; i++) {
            GICD_CALL(set_cfgr, first_int + i,
                      bit_extract(val, GIC_CONFIG_BITS * i, GIC_CONFIG_BITS));
        }
    } else {
        for (int i = 0; i < acc->width; i++) {
            val |= GICD_CALL(get_trgt, first_int + i) << (GIC_CONFIG_BITS * i);
        }
        vcpu_writereg(cpu.vcpu, acc->reg, val);
    }
}

static void vgicd_emul_sgiregs_access(emul_access_t *acc)
{
    if (acc->write) {
        uint32_t val = vcpu_readreg(cpu.vcpu, acc->reg);
        uint64_t trgt =
            vgic_target_translate(cpu.vcpu->vm, GICD_SGIR_CPUTRGLST(val), true);

        GICD_CALL(send_sgi, GICD_SGIR_SGIINTID(val), trgt,
                  GICD_SGIR_TRGLSTFLT(val));
    }
}

static bool vgicd_emul_handler(emul_access_t *acc)
{
    if (acc->width > 4) return false;

    uint32_t acc_offset = acc->addr & GIC_INT_ADDR_MASK;

    switch (GICD_GROUP(acc->addr)) {
        case GICD_REG_GROUP(CTLR):
        case GICD_REG_GROUP(ISENABLER):
        case GICD_REG_GROUP(ISPENDR):
        case GICD_REG_GROUP(ISACTIVER):
        case GICD_REG_GROUP(ICENABLER):
        case GICD_REG_GROUP(ICPENDR):
        case GICD_REG_GROUP(ICACTIVER):
        case GICD_REG_GROUP(ICFGR):
            /* Only allow aligned 32-bit accesses or byte */
            if (acc->width != 4 || acc->addr & 0x3) {
                return false;
            }
            break;

        case GICD_REG_GROUP(SGIR):
            /* Allow byte access or align 16-bit accesses */
            if ((acc->width == 4 && (acc->addr & 0x3)) ||
                (acc->width == 2 && (acc->addr & 0x1))) {
                return false;
            }
            break;

        default:
            if (GICD_IS_REG(IPRIORITYR, acc_offset) ||
                GICD_IS_REG(ITARGETSR, acc_offset)) {
                /* Allow byte access or align 16-bit accesses */
                if ((acc->width == 4 && (acc->addr & 0x3)) ||
                    (acc->width == 2 && (acc->addr & 0x1))) {
                    return false;
                }
            }
    }

    switch (GICD_GROUP(acc->addr)) {
        case GICD_REG_GROUP(CTLR):
            vgicd_emul_misc_access(acc);
            break;
        case GICD_REG_GROUP(ISENABLER):
            vgicd_emul_ienabler_access(acc, true);
            break;
        case GICD_REG_GROUP(ISPENDR):
            vgicd_emul_istate_access(acc, PEND, true);
            break;
        case GICD_REG_GROUP(ISACTIVER):
            vgicd_emul_istate_access(acc, ACT, true);
            break;
        case GICD_REG_GROUP(ICENABLER):
            vgicd_emul_ienabler_access(acc, false);
            break;
        case GICD_REG_GROUP(ICPENDR):
            vgicd_emul_istate_access(acc, PEND, false);
            break;
        case GICD_REG_GROUP(ICACTIVER):
            vgicd_emul_istate_access(acc, ACT, false);
            break;
        case GICD_REG_GROUP(ICFGR):
            vgicd_emul_icfgr_access(acc);
            break;
        case GICD_REG_GROUP(SGIR):
            vgicd_emul_sgiregs_access(acc);
            break;
        default:
            if (GICD_IS_REG(IPRIORITYR, acc_offset)) {
                vgicd_emul_ipriorityr_access(acc);
            } else if (GICD_IS_REG(ITARGETSR, acc_offset)) {
                vgicd_emul_itargetr_access(acc);
            }
    }

    return true;
}

inline void vgic_init(vm_t *vm, const struct gic_dscrp *gic_dscrp)
{
    size_t n = NUM_PAGES(sizeof(gicc_t));
    void *va =
        mem_alloc_vpage(&vm->as, SEC_VM_ANY, (void *)gic_dscrp->gicc_addr, n);
    if (va != (void *)gic_dscrp->gicc_addr) {
        ERROR("failed to alloc vm address space to hold gicc");
    }
    mem_map_dev(&vm->as, va, platform.arch.gic.gicc_addr, n);

    emul_region_t emu = {.va_base = gic_dscrp->gicd_addr,
                         .pa_base = (uint64_t)&gicd,
                         .size = ALIGN(sizeof(gicd_t), PAGE_SIZE),
                         .handler = vgicd_emul_handler};

    vm_add_emul(vm, &emu);
}
