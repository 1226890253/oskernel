#include "gdt.h"

#include "tty.h"
#include "kernel.h"

extern void x64_cpu_check();

void main() {
    console_init();
    int add_64=0x100000; //1MB
    x64_cpu_check();
    entry64();//进入64位长模式！！！:-)
    printk("enable x64 \n");
    asm __volatile("push $0x0018\n"
        "push $0x00100000\n"
        "retf"::);
    //asm volatile("jmp 0x18:0x00100000;");
    printk("not entry x64 kernel\n");
    while (1);
}