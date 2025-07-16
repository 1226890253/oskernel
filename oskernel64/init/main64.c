#include "tty.h"
#include "kernel.h"

void main64(){
    console_init();
    printk("hello,x64!!,0x%x___0x%x\n",0x8899,0x5566);
    while (true) {
        asm __volatile("hlt\n");
    }
}
