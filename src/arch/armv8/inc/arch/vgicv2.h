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

#ifndef __VGICV2_H__
#define __VGICV2_H__

#include <arch/gic.h>

#define GICD_GROUP(addr) ((addr & 0xF80) >> 7)
#define GICD_REG_IND(REG) (offsetof(gicd_t, REG) & GIC_MISC_REG_MASK)
#define GICD_REG_GROUP(REG) GICD_GROUP(offsetof(gicd_t, REG))
#define GICD_IS_REG(REG, offset)            \
    (((offset) >= offsetof(gicd_t, REG)) && \
     (offset) < (offsetof(gicd_t, REG) + sizeof(gicd.REG)))

#define GICD_CALL(ACCESS_FUNCTION, id, ...) \
    vgic_is_owner(id) ? gicd_##ACCESS_FUNCTION(id, ##__VA_ARGS__) : 0

typedef struct vm vm_t;
struct gic_dscrp;

void vgic_init(vm_t *vm, const struct gic_dscrp *gic_dscrp);

#endif /* __VGICV2_H__ */
