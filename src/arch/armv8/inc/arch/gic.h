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

#ifndef __GIC_H__
#define __GIC_H__

#include <bitmap.h>
#include <arch/smc.h>

#define SM_GIC_FIQ_SET          (0xC5000000)
#define SM_GIC_FIQ_ACK          (0xC5000001)
#define SM_GIC_FIQ_END          (0xC5000002)
#define SM_GIC_FIQ_RAISE_SGI    (0xC5000003)
#define SM_GIC_FIQ_GET_PEND     (0xC5000004)

#define GIC_MAX_INTERUPTS 1024
#define GIC_MAX_SGIS 16
#define GIC_MAX_PPIS 16
#define GIC_CPU_PRIV (GIC_MAX_SGIS + GIC_MAX_PPIS)
#define GIC_MAX_SPIS (GIC_MAX_INTERUPTS - GIC_CPU_PRIV)
#define GIC_PRIO_BITS 8
#define GIC_TARGET_BITS 8
#define GIC_MAX_TARGETS GIC_TARGET_BITS
#define GIC_CONFIG_BITS 2
#define GIC_SEC_BITS 2
#define GIC_SGI_BITS 8
#define GIC400_PRIO_MASK (0xF0)

#define GIC_INT_REG(NINT) (NINT / (sizeof(uint32_t) * 8))
#define GIC_INT_MASK(NINT) (1U << NINT % (sizeof(uint32_t) * 8))
#define GIC_INT_ADDR_MASK (0xFFF)
#define GIC_INT_REG_SIZE (32)
#define GIC_NUM_INT_REGS(NINT) GIC_INT_REG(NINT)
#define GIC_NUM_PRIVINT_REGS (GIC_CPU_PRIV / (sizeof(uint32_t) * 8))
#define GIC_MISC_REG_MASK (0x7F)

#define GIC_PRIO_REG(NINT) ((NINT * GIC_PRIO_BITS) / (sizeof(uint32_t) * 8))
#define GIC_NUM_PRIO_REGS(NINT) GIC_PRIO_REG(NINT)
#define GIC_PRIO_OFF(NINT) (NINT * GIC_PRIO_BITS) % (sizeof(uint32_t) * 8)
#define GIC_PRIO_ADDR_MASK (0x1FF)

#define GIC_TARGET_REG(NINT) ((NINT * GIC_TARGET_BITS) / (sizeof(uint32_t) * 8))
#define GIC_NUM_TARGET_REGS(NINT) GIC_TARGET_REG(NINT)
#define GIC_TARGET_OFF(NINT) (NINT * GIC_TARGET_BITS) % (sizeof(uint32_t) * 8)
#define GIC_TARGET_ADDR_MASK (0x1FF)

#define GIC_CONFIG_REG(NINT) ((NINT * GIC_CONFIG_BITS) / (sizeof(uint32_t) * 8))
#define GIC_NUM_CONFIG_REGS(NINT) GIC_CONFIG_REG(NINT)
#define GIC_CONFIG_OFF(NINT) (NINT * GIC_CONFIG_BITS) % (sizeof(uint32_t) * 8)
#define GIC_CONFIG_ADDR_MASK (0x1FF)

#define GIC_NUM_SEC_REGS(NINT) ((NINT * GIC_SEC_BITS) / (sizeof(uint32_t) * 8))

#define GIC_NUM_SGI_REGS \
    ((GIC_MAX_SGIS * GIC_SGI_BITS) / (sizeof(uint32_t) * 8))
#define GICD_SGI_REG(NINT) (NINT / 4)
#define GICD_SGI_OFF(NINT) ((NINT % 4) * 8)

#define GIC_NUM_APR_REGS ((1UL << (GIC_PRIO_BITS - 1)) / (sizeof(uint32_t) * 8))

/* Distributor Control Register, GICD_CTLR */

#define GICD_CTLR_EN_BIT (0x1)

/*  Interrupt Controller Type Register, GICD_TYPER */

#define GICD_TYPER_ITLINENUM_OFF (0)
#define GICD_TYPER_ITLINENUM_LEN (5)
#define GICD_TYPER_CPUNUM_OFF (5)
#define GICD_TYPER_CPUNUM_LEN (3)
#define GICD_TYPER_SECUREXT_BIT (1UL << 10)
#define GICD_TYPER_LSPI_OFF (11)
#define GICD_TYPER_LSPI_LEN (6)

/* Software Generated Interrupt Register, GICD_SGIR */

#define GICD_TYPER_ITLN_OFF 0
#define GICD_TYPER_ITLN_LEN 5
#define GICD_TYPER_ITLN_MSK BIT_MASK(GICD_TYPER_ITLN_OFF, GICD_TYPER_ITLN_LEN)
#define GICD_TYPER_CPUN_OFF 5
#define GICD_TYPER_CPUN_LEN 3
#define GICD_TYPER_CPUN_MSK BIT_MASK(GICD_TYPER_CPUN_OFF, GICD_TYPER_CPUN_LEN)

#define GICD_SGIR_SGIINTID_OFF 0
#define GICD_SGIR_SGIINTID_LEN 4
#define GICD_SGIR_SGIINTID_MSK \
    (BIT_MASK(GICD_SGIR_SGIINTID_OFF, GICD_SGIR_SGIINTID_LEN))
#define GICD_SGIR_SGIINTID(sgir) \
    bit_extract(sgir, GICD_SGIR_SGIINTID_OFF, GICD_SGIR_SGIINTID_LEN)
#define GICD_SGIR_CPUTRGLST_OFF 16
#define GICD_SGIR_CPUTRGLST_LEN 8
#define GICD_SGIR_CPUTRGLST_MSK(trgt) (trgt << GICD_SGIR_CPUTRGLST_OFF)
#define GICD_SGIR_CPUTRGLST(sgir) \
    bit_extract(sgir, GICD_SGIR_CPUTRGLST_OFF, GICD_SGIR_CPUTRGLST_LEN)
#define GICD_SGIR_TRGLSTFLT_OFF 24
#define GICD_SGIR_TRGLSTFLT_LEN 2
#define GICD_SGIR_TRGLSTFLT_MSK(flt) (flt << GICD_SGIR_TRGLSTFLT_OFF)
#define GICD_SGIR_TRGLSTFLT(sgir) \
    bit_extract(sgir, GICD_SGIR_TRGLSTFLT_OFF, GICD_SGIR_TRGLSTFLT_LEN)

typedef struct {
    uint32_t CTLR;
    uint32_t TYPER;
    uint32_t IIDR;
    uint8_t pad0[0x0080 - 0x000C];
    uint32_t IGROUPR[GIC_NUM_INT_REGS(GIC_MAX_INTERUPTS)];  // banked CPU
    uint32_t ISENABLER[GIC_NUM_INT_REGS(GIC_MAX_INTERUPTS)];
    uint32_t ICENABLER[GIC_NUM_INT_REGS(GIC_MAX_INTERUPTS)];
    uint32_t ISPENDR[GIC_NUM_INT_REGS(GIC_MAX_INTERUPTS)];
    uint32_t ICPENDR[GIC_NUM_INT_REGS(GIC_MAX_INTERUPTS)];
    uint32_t ISACTIVER[GIC_NUM_INT_REGS(GIC_MAX_INTERUPTS)];
    uint32_t ICACTIVER[GIC_NUM_INT_REGS(GIC_MAX_INTERUPTS)];
    uint32_t IPRIORITYR[GIC_NUM_PRIO_REGS(GIC_MAX_INTERUPTS)];
    //    uint8_t pad1[0x0800 - 0x07FC];
    uint32_t ITARGETSR[GIC_NUM_TARGET_REGS(GIC_MAX_INTERUPTS)];
    //    uint8_t pad2[0x0C00 - 0x0820];
    uint32_t ICFGR[GIC_NUM_CONFIG_REGS(GIC_MAX_INTERUPTS)];
    uint8_t pad3[0x0E00 - 0x0D00];
    uint32_t NSACR[GIC_NUM_SEC_REGS(GIC_MAX_INTERUPTS)];
    uint32_t SGIR;
    uint8_t pad4[0x0F10 - 0x0F04];
    uint32_t CPENDSGIR[GIC_NUM_SGI_REGS];
    uint32_t SPENDSGIR[GIC_NUM_SGI_REGS];
} __attribute__((__packed__)) gicd_t;

/* CPU Interface Control Register, GICC_CTLR */

#define GICC_CTLR_EN_BIT (0x1)
#define GICC_CTLR_EOImodeNS_BIT (1UL << 9)
#define GICC_CTLR_WR_MSK (0x1)
#define GICC_IAR_ID_OFF (0)
#define GICC_IAR_ID_LEN (10)
#define GICC_IAR_ID_MSK (BIT_MASK(GICC_IAR_ID_OFF, GICC_IAR_ID_LEN))
#define GICC_IAR_CPU_OFF (10)
#define GICC_IAR_CPU_LEN (3)
#define GICC_IAR_CPU_MSK (BIT_MASK(GICC_IAR_CPU_OFF, GICC_IAR_CPU_LEN))

typedef struct {
    uint32_t CTLR;
    uint32_t PMR;
    uint32_t BPR;
    uint32_t IAR;
    uint32_t EOIR;
    uint32_t RPR;
    uint32_t HPPIR;
    uint32_t ABPR;
    uint32_t AIAR;
    uint32_t AEOIR;
    uint32_t AHPPIR;
    uint8_t pad0[0x00D0 - 0x002C];
    uint32_t APR[GIC_NUM_APR_REGS];
    uint32_t NSAPR[GIC_NUM_APR_REGS];
    uint8_t pad1[0x00FC - 0x00F0];
    uint32_t IIDR;
    uint8_t pad2[0x1000 - 0x0100];
    uint32_t DIR;
} __attribute__((__packed__)) gicc_t;

extern volatile gicd_t gicd;
extern volatile gicc_t gicc;

enum int_state { INV, PEND, ACT, PENDACT };

void gic_init();
void gic_cpu_init();
void gicd_send_sgi(uint64_t int_id, uint8_t trgt, uint8_t filter);

void gicd_set_enable(uint64_t int_id, bool en);
void gicd_set_prio(uint64_t int_id, uint8_t prio);
void gicd_set_state(uint64_t int_id, enum int_state state);
void gicd_set_trgt(uint64_t int_id, uint8_t trgt);
void gicd_set_cfgr(uint64_t int_id, uint8_t cfgr);
bool gicd_enabled(uint64_t int_id);
uint8_t gicd_get_trgt(uint64_t int_id);
uint8_t gicd_get_cfgr(uint64_t int_id);
uint64_t gicd_get_prio(uint64_t int_id);
enum int_state gicd_get_state(uint64_t int_id);

static inline uint64_t gic_num_irqs()
{
    uint32_t itlinenumber =
        bit_extract(gicd.TYPER, GICD_TYPER_ITLN_OFF, GICD_TYPER_ITLN_LEN);
    return 32 * itlinenumber + 1;
}

static inline bool gic_is_sgi(uint64_t int_id)
{
    return int_id < GIC_MAX_SGIS;
}

static inline uint32_t gicd_get_ctlr()
{
    return gicd.CTLR;
}

static inline uint32_t gicd_get_typer()
{
    return gicd.TYPER;
}

static inline uint32_t gicd_get_iidr()
{
    return gicd.IIDR;
}

static inline void gic_set_as_fiq(uint64_t id, uint64_t prio)
{
    smc_call(SM_GIC_FIQ_SET, id, prio, 0, NULL);
}

static inline uint64_t gic_fiq_ack()
{
    return smc_call(SM_GIC_FIQ_ACK, 0, 0, 0, NULL);
}

static inline void gic_fiq_end(uint64_t id)
{
    smc_call(SM_GIC_FIQ_END, id, 0, 0, NULL);
}

static inline void gic_fiq_raise_sgi(uint64_t target, uint64_t id)
{
    smc_call(SM_GIC_FIQ_RAISE_SGI, id, target, 0, NULL);
}

static inline bool gic_fiq_get_pend()
{
    return smc_call(SM_GIC_FIQ_GET_PEND, 0, 0, 0, NULL);
}

#endif /* __GIC_H__ */
