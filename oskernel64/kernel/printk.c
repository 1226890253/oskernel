#include "tty.h"
#include "kernel.h"

char buff[1024] = {0};
// char printk_buff[1024]={0};
int printk(const char *fmt, ...) {
    asm __volatile(
        "movq %%rdx, %%r12\n"
        "movq %%rsi, %%r11\n"
        "movq %%rcx, %%r13\n"
        "movq %%r8, %%r14\n"
        "movq %%r9, %%r15\n"
        ::);
    int i;
    u64 va_list[24] = {0};
    asm __volatile(
        "movq %%r11, 0(%[va_list])\n"
        "movq %%r12, 8(%[va_list])\n"
        "movq %%r13, 16(%[va_list])\n"
        "movq %%r14, 24(%[va_list])\n"
        "movq %%r15, 32(%[va_list])\n"
        ::[va_list]"r"(va_list):);

    const char *f2 = fmt;
    int count = 0;
    while (*f2) {
        if (*f2++ == '%') count++;
    }
    count -= 5;
    u64 * rbp=NULL;
    asm __volatile(
        "movq %%rbp, %[rbp]"
        :[rbp]"=r"(rbp));
    for (int i = 0; i < count; i++) {
        va_list[i+5]=*(rbp+2+i);
    }

    i = vsprintf(buff, fmt, va_list);
    console_write(buff, i);
}
