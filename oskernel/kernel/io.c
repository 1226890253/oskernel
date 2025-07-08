#include "io.h"

char in_byte(int port) {
    unsigned char ret;
    asm volatile (
        "inb %[port], %[ret]"
        : [ret] "=a" (ret)
        : [port] "Nd" (port)
    );
    return ret;
}

short in_word(int port) {
    unsigned short ret;
    asm volatile (
        "inw %[port], %[ret]"
        : [ret] "=a" (ret)
        : [port] "Nd" (port)
    );
    return ret;
}

void out_byte(int port, int v) {
    asm volatile (
        "outb %[value], %[port]"
        :
        : [value] "a" ((unsigned char)v),
          [port]  "Nd" (port)
    );
}

void out_word(int port, int v) {
    asm volatile (
        "outw %[value], %[port]"
        :
        : [value] "a" ((unsigned short)v),
          [port]  "Nd" (port)
    );
}