#ifndef STDARG_H
#define STDARG_H

typedef char* va_list;

// 跳过开头fmt参数，将p指向第一个可变参数
#define va_start(p, count) (p = (va_list)&count + sizeof(char*))
// 这行代码做了两件事情：1、修改p_args; 2、取值
#define va_arg(p, t) (*(t*)((p += sizeof(char*)) - sizeof(char*)))
#define va_end(p) (p = 0)

#endif
