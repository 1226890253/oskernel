#include <apic.h>
#include <idt.h>

#include "mm.h"
#include "tty.h"
#include "kernel.h"
#include "fppinit.h"


void check_apic() {
    u32 a,b,c,d;
    asm __volatile(
        "cpuid\n"
        :"=a"(a), "=b"(b), "=c"(c), "=d"(d)
        :"a"(1),"c"(0));
    bool is_apic= (d & (1u << 9)) != 0;
    bool is_apic2 = (c & (1u << 21)) != 0;
    printk("APIC=%s\n",is_apic? "yes":"no");
    printk("APIC2=%s\n",is_apic2? "yes":"no");
}


void main64() {
    console_init();
    init_fp_sse();
    printk("hello,x64!!,1_0x%x 2_0x%x 3_0x%x 4_0x%x 5_0x%x 6_0x%x 7_0x%x 8_0x%x 9_0x%x\n", 0x1111, 0x2222, 0x3333, 0x4444, 0x5555,
           0x6666, 0x7777, 0x8888, 0x9999);
    print_check_mm_info();
    mov_gdt();
    kernelPage_init();
    check_apic();
    print_CPUID();
    check_APIC_Timer_status();
    check_APIC_TSC();

    apic_singleCore_timer_init();
    u32 t=*(volatile u32*)(0xFEE00000ull + 0x20);
    printk("success read t=%d",t);
    //int a=10/0;

    while (true) {
        asm __volatile("hlt\n");
    }
}
