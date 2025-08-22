#include <apic.h>
#include <idt.h>
#include <tss.h>

#include "mm.h"
#include "tty.h"
#include "kernel.h"
#include "fppinit.h"


void main64() {
    console_init();
    init_fp_sse();
    print_check_mm_info();
    mov_gdt();
    kernelPage_init();
    print_CPUID();


    idt_init();
    apic_init();

    while (true) {
        asm __volatile("hlt\n");
    }
}
