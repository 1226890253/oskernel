#ifndef GDT_H
#define GDT_H

#define PML4_ADDR 0x8000
#define PDPT_ADDR 0x9000
#define PD_ADDR 0xa000

#pragma pack(push,1)
typedef struct gdtr32 {
    unsigned short limit; //历史遗留问题：0x0000表示1字节，0xFFFF表示65536字节，故需要-1
    unsigned int address_start;
} gdtr32;
#pragma pack(pop)

typedef struct __attribute__((packed)) gdt_desp {
    unsigned short limit_low: 16;      // 段界限 0 ~ 15 位
    unsigned int base_low : 24;    // 基地址 0 ~ 23 位 16M
    unsigned int type : 4;        // 段类型
    unsigned int segment : 1;     // 1 表示代码段或数据段，0 表示系统段
    unsigned int DPL : 2;         // Descriptor Privilege Level 描述符特权等级 0 ~ 3
    unsigned int present : 1;     // 存在位，1 在内存中，0 在磁盘上
    unsigned int limit_high : 4;  // 段界限 16 ~ 19;
    unsigned int available : 1;   // 该安排的都安排了，送给操作系统吧
    unsigned int long_mode : 1;   // 64 位扩展标志
    unsigned int big : 1;         // 32 位 还是 16 位;
    unsigned int granularity : 1; // 粒度 4KB 或 1B
    unsigned int base_high;       // 基地址 24 ~ 31 位
} gdt_desp;

void entry64();
#endif //GDT_H

int check64();