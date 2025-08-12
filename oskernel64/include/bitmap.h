#ifndef BITMAP_H
#define BITMAP_H
#include "types.h"

typedef struct {
    u8* map;        //map首地址
    u32 size;       //map大小
    u64 mm_start;     //管理的内存首地址
    u32 page_total; //总共管理多少页
    u32 page_used;  //已经使用了多少页
}Bitmap;

#endif //BITMAP_H
