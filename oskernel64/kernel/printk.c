#include "tty.h"
#include "kernel.h"

char buff[1024] = {0};
// char printk_buff[1024]={0};
int printk(const char *fmt, ...) {
    int i;
    asm __volatile("push %%r9\n"
        "push %%r8\n"
        "push %%rcx\n"
        "push %%rdx\n"
        "push %%rsi\n"
        "push %%rdi\n"
        :::);
    unsigned long *rsp = NULL;
    asm __volatile("movq %%rsp, %[rsp]\n"
        :[rsp] "=r" (rsp)
        :);
    i = vsprintf(buff, fmt, rsp+1);
    console_write(buff, i);
    asm __volatile("addq $48, %%rsp"::);
}
