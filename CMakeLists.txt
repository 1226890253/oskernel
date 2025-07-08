cmake_minimum_required(VERSION 3.30)
project(os VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)

add_executable(os main.cpp
        oskernel/init/main.c
        oskernel/kernel/io.c
        oskernel/kernel/chr_drv/console.c
        oskernel/lib/string.c
        oskernel/kernel/vsprintf.c
        oskernel/kernel/printk.c
)

target_include_directories(os PUBLIC
        ${CMAKE_SOURCE_DIR}/oskernel/include
)