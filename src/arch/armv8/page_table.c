/**
 * Bao, a Lightweight Static Partitioning Hypervisor
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

#include <bao.h>
#include <page_table.h>
#include <arch/sysregs.h>
#include <cpu.h>

struct page_table_dscr armv8_pt_dscr = {
    .lvls = 4,
    .lvl_wdt = (size_t[]){48, 39, 30, 21},
    .lvl_off = (size_t[]){39, 30, 21, 12},
    .lvl_term = (bool[]){false, true, true, true},
};

/**
 * This might be modified at initialization depending on the
 * value of parange and consequently SL0 in VTCR_EL2.
 */
struct page_table_dscr armv8_pt_s2_dscr = {
    .lvls = 4,
    .lvl_wdt = (size_t[]){48, 39, 30, 21},
    .lvl_off = (size_t[]){39, 30, 21, 12},
    .lvl_term = (bool[]){false, true, true, true},
};

size_t parange_table[] = {32, 36, 40, 42, 44, 48};

struct page_table_dscr* hyp_pt_dscr = &armv8_pt_dscr;
struct page_table_dscr* vm_pt_dscr = &armv8_pt_s2_dscr;

size_t parange __attribute__((section(".data")));

void pt_set_recursive(struct page_table* pt, size_t index)
{
    paddr_t pa;
    mem_translate(&cpu.as, (vaddr_t)pt->root, &pa);
    pte_t* pte = cpu.as.pt.root + index;
    pte_set(pte, pa, PTE_TABLE | PTE_HYP_FLAGS);
    pt->root_flags &= ~PT_ROOT_FLAGS_REC_IND_MSK;
    pt->root_flags |=
        (index << PT_ROOT_FLAGS_REC_IND_OFF) & PT_ROOT_FLAGS_REC_IND_MSK;
}

pte_t* pt_get_pte(struct page_table* pt, size_t lvl, vaddr_t va)
{
    struct page_table* cpu_pt = &cpu.as.pt;

    size_t rec_ind_off = cpu_pt->dscr->lvl_off[cpu_pt->dscr->lvls - lvl - 1];
    size_t rec_ind_len = cpu_pt->dscr->lvl_wdt[cpu_pt->dscr->lvls - lvl - 1];
    pte_t mask = (1UL << rec_ind_off) - 1;
    pte_t rec_ind_mask = ((1UL << rec_ind_len) - 1) & ~mask;
    size_t rec_ind = ((pt->root_flags & PT_ROOT_FLAGS_REC_IND_MSK) >>
                        PT_ROOT_FLAGS_REC_IND_OFF);
    pte_t addr = ~mask;
    addr &= PTE_ADDR_MSK;
    addr &= ~(rec_ind_mask);
    addr |= ((rec_ind << rec_ind_off) & rec_ind_mask);
    addr |= (((va >> pt->dscr->lvl_off[lvl]) * sizeof(pte_t)) & (mask));

    return (pte_t*)addr;
}

pte_t* pt_get(struct page_table* pt, size_t lvl, vaddr_t va)
{
    if (lvl == 0) return pt->root;

    uintptr_t pte = (uintptr_t)pt_get_pte(pt, lvl, va);
    pte &= ~(pt_size(pt, lvl) - 1);
    return (pte_t*)pte;
}

pte_t pt_pte_type(struct page_table* pt, size_t lvl)
{
    return (lvl == pt->dscr->lvls - 1) ? PTE_PAGE : PTE_SUPERPAGE;
}

bool pte_page(struct page_table* pt, pte_t* pte, size_t lvl)
{
    if (lvl != pt->dscr->lvls - 1) {
        return false;
    }

    return (*pte & PTE_TYPE_MSK) == PTE_PAGE;
}

bool pte_table(struct page_table* pt, pte_t* pte, size_t lvl)
{
    if (lvl == pt->dscr->lvls - 1) {
        return false;
    }

    return (*pte & PTE_TYPE_MSK) == PTE_TABLE;
}
