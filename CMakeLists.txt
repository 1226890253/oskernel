cmake_minimum_required(VERSION 3.30)
project(os VERSION 1.0)

#set(CMAKE_CXX_STANDARD 11)

add_executable(os
#        oskernel/init/main.c
#        oskernel/kernel/chr_drv/console.c
#        oskernel/kernel/io.c
#        oskernel/kernel/vsprintf.c
#        oskernel/kernel/printk.c
#        oskernel/kernel/gdt.c
#        oskernel/lib/string.c

        oskernel64/init/main64.c
        oskernel64/kernel/chr_drv/console.c
        oskernel64/kernel/io.c
        oskernel64/kernel/vsprintf.c
        oskernel64/kernel/printk.c
        oskernel64/lib/string.c
        oskernel64/mm/mm.c
        oskernel64/mm/bitmap.c
        oskernel64/lib/math.c
        oskernel64/init/fppinit.c
        oskernel64/kernel/idt.c
        oskernel64/kernel/apic.c

)

target_include_directories(os PUBLIC
        oskernel64/include
#        oskernel/include
)