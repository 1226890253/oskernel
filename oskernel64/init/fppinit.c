//
// Created by ljw on 25-8-3.
//

#include "fppinit.h"
void init_fp_sse(void) {
    __asm__ __volatile__ (
        "mov %%cr0, %%rax\n\t"
        "andq $~(1<<2), %%rax\n\t"   // clear EM
        "orq $(1<<1), %%rax\n\t"     // set MP
        "mov %%rax, %%cr0\n\t"
        "mov %%cr4, %%rax\n\t"
        "orq $((1<<9)|(1<<10)), %%rax\n\t" // set OSFXSR and OSXMMEXCPT
        "mov %%rax, %%cr4\n\t"
        "fninit\n\t"
        :
        :
        : "rax"
    );
}