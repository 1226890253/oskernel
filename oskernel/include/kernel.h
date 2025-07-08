//
// Created by ziya on 22-6-26.
//

#ifndef KERNEL_H
#define KERNEL_H

#include "stdarg.h"

int vsprintf(char *buf, const char *fmt, va_list args);

int printk(const char * fmt, ...);

#endif
