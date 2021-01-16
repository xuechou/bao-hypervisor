#include <platform.h>

struct platform_desc platform = {

    .cpu_num = 4,

    .region_num = 1,
    .regions =  (struct mem_region[]) {
        {
            .base = 0x40200000,
            .size = 0x40000000 - 0x200000
        }
    },

    .console = {
        .base = 0xff000000
    },

    .cache = {
        .lvls = 2,
        .min_shared_lvl = 1,
        .type = {SEPARATE, UNIFIED},
        .indexed = {{VIPT, VIPT}, {PIPT}},
        .line_size = {{64, 64}, {64}},
        .assoc = {{4, 4}, {8}},
        .numset = {{64, 64}, {1024}}
    },

    .arch = {
        .plic_base = 0xc000000,
    }

};
