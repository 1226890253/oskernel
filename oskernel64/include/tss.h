//
// Created by ljw on 25-8-15.
//

#ifndef TSS_H
#define TSS_H

#include "types.h"

u16 tss_init_and_load(void *gdt_base, int tss_index);
#endif //TSS_H
