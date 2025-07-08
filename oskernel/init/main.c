#include "tty.h"
#include "kernel.h"

void main() {
    console_init();

     char* s = "ziya";

     for (int i = 0; i < 2048; ++i) {
         printk("name: %1234 %s, age:%d\n", s, i);
     }

    while (1);
}