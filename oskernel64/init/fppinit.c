//
// Created by ljw on 25-8-3.
//

#include "fppinit.h"
void init_fp_sse(void) {
    // __asm__ __volatile__ (
    //     "mov %%cr0, %%rax\n\t"
    //     "andq $~(1<<2), %%rax\n\t"   // clear EM
    //     "orq $(1<<1), %%rax\n\t"     // set MP
    //     "mov %%rax, %%cr0\n\t"
    //     "mov %%cr4, %%rax\n\t"
    //     "orq $((1<<9)|(1<<10)), %%rax\n\t" // set OSFXSR and OSXMMEXCPT
    //     "mov %%rax, %%cr4\n\t"
    //     "fninit\n\t"
    //     :
    //     :
    //     : "rax"
    // );
    __asm__ __volatile__ (
       /* CR0: clear EM, TS; set MP, NE */
       "mov %%cr0, %%rax\n\t"
       "andq $~((1<<2)|(1<<3)), %%rax\n\t"   /* EM=bit2, TS=bit3 -> clear */
       "orq  $((1<<1)|(1<<5)), %%rax\n\t"    /* MP=bit1, NE=bit5 -> set   */
       "mov %%rax, %%cr0\n\t"

       /* CR4: set OSFXSR, OSXMMEXCPT */
       "mov %%cr4, %%rax\n\t"
       "orq $((1<<9)|(1<<10)), %%rax\n\t"    /* OSFXSR=bit9, OSXMMEXCPT=bit10 */
       "mov %%rax, %%cr4\n\t"

       /* 初始化 x87 FPU；MXCSR 如需特定值，另行 ldmxcsr */
       "fninit\n\t"
       :
       :
       : "rax", "cc", "memory"
   );
}