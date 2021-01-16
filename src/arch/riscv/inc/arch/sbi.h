/**
 * Bao Hypervisor
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#ifndef __SBI_H__
#define __SBI_H__

#include <bao.h>
#include <spinlock.h>

/**
 * From https://github.com/riscv/riscv-sbi-doc
 */

#define SBI_SUCCESS (0)
#define SBI_ERR_FAILURE (-1)
#define SBI_ERR_NOT_SUPPORTED (-2)
#define SBI_ERR_INVALID_PARAM (-3)
#define SBI_ERR_DENIED (-4)
#define SBI_ERR_INVALID_ADDRESS (-5)
#define SBI_ERR_ALREADY_AVAILABLE (-6)

struct sbiret {
    long error;
    long value;
};

struct sbi_hsm {
    spinlock_t lock;
    enum { STARTED, STOPPED, START_PENDING, STOP_PENDING } state;
    uint64_t start_addr;
    uint64_t priv; 
};

void sbi_init();

void sbi_console_putchar(int ch);

struct sbiret sbi_get_spec_version(void);
struct sbiret sbi_get_impl_id(void);
struct sbiret sbi_get_impl_version(void);
struct sbiret sbi_probe_extension(long extension_id);
struct sbiret sbi_get_mvendorid(void);
struct sbiret sbi_get_marchid(void);
struct sbiret sbi_get_mimpid(void);

struct sbiret sbi_send_ipi(const unsigned long hart_mask,
                           unsigned long hart_mask_base);

struct sbiret sbi_set_timer(uint64_t stime_value);

struct sbiret sbi_remote_fence_i(const unsigned long hart_mask,
                                 unsigned long hart_mask_base);

struct sbiret sbi_remote_sfence_vma(const unsigned long hart_mask,
                                    unsigned long hart_mask_base,
                                    unsigned long start_addr,
                                    unsigned long size);

struct sbiret sbi_remote_sfence_vma_asid(const unsigned long hart_mask,
                                         unsigned long hart_mask_base,
                                         unsigned long start_addr,
                                         unsigned long size,
                                         unsigned long asid);

struct sbiret sbi_remote_hfence_gvma_vmid(const unsigned long hart_mask,
                                          unsigned long hart_mask_base,
                                          unsigned long start_addr,
                                          unsigned long size,
                                          unsigned long vmid);

struct sbiret sbi_remote_hfence_gvma(const unsigned long hart_mask,
                                     unsigned long hart_mask_base,
                                     unsigned long start_addr,
                                     unsigned long size);

struct sbiret sbi_remote_hfence_vvma_asid(const unsigned long hart_mask,
                                          unsigned long hart_mask_base,
                                          unsigned long start_addr,
                                          unsigned long size,
                                          unsigned long asid);

struct sbiret sbi_remote_hfence_vvma(const unsigned long hart_mask,
                                     unsigned long hart_mask_base,
                                     unsigned long start_addr,
                                     unsigned long size);

struct sbiret sbi_hart_start(unsigned long hartid, unsigned long start_addr,
                             unsigned long priv);
struct sbiret sbi_hart_stop();
struct sbiret sbi_hart_status(unsigned long hartid);

static inline struct sbiret sbi_ecall(long eid, long fid, long a0, long a1,
                                      long a2, long a3, long a4, long a5)
{
    long register _a0 asm("a0") = a0;
    long register _a1 asm("a1") = a1;
    long register _a2 asm("a2") = a2;
    long register _a3 asm("a3") = a3;
    long register _a4 asm("a4") = a4;
    long register _a5 asm("a5") = a5;
    long register _a6 asm("a6") = fid;
    long register _a7 asm("a7") = eid;

    asm volatile("ecall"
                 : "+r"(_a0), "+r"(_a1)
                 : "r"(_a2), "r"(_a3), "r"(_a4), "r"(_a5), "r"(_a6), "r"(_a7)
                 : "memory");

    struct sbiret ret = {.error = _a0, .value = _a1};

    return ret;
}

#endif /* __SBI_H__ */
