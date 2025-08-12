#ifndef MM_H
#define MM_H
#include "types.h"

#define PML4_ADDR 0x0000
#define PDPT_ADDR 0x1000
#define PD_ADDR 0x2000
#define PT_ADDR 0x3000

#define GDT_ADDR 0x4000
#define IDT_ADDR 0x5000
#define BITMAP_ADDR 0x10000
#define KERNEL_START 0x100000
#define USR_ADDR_START 0xa00000

typedef struct ARDS {
    unsigned int baseAddrLow;
    unsigned int baseAddrHigh;
    unsigned int lengthLow;
    unsigned int lengthHigh;
    unsigned int type;
} ARDS;

typedef struct check_mm_info {
    unsigned short times;
    ARDS *ards;
} check_mm_info;

typedef union {
    u64 val;

    struct {
        u64 present: 1; //present位 ，必须为1
        u64 rw: 1; //读写权限，内核必须为1
        u64 us: 1; //user/system, 内核为0
        u64 pwt: 1; //缓存位，不管
        u64 pcd: 1; //缓存位，不管
        u64 access: 1; //access,访问位，由硬件在访问时置1
        u64 ignore: 1; //忽略
        u64 reserve: 1; //保留
        u64 ignore2: 3; //忽略
        u64 R: 1; //忽略，必须为0
        u64 addr: 40; //下级表地址
        u64 ignore3: 11; //忽略
        u64 XD: 1; //禁止执行位，IA32-EFER.NEX=1时启用
    }__attribute__((packed));
} PML4_ENTRY;

typedef struct {
    PML4_ENTRY items[512];
} PML4;

typedef union {
    u64 val;

    struct {
        u64 present: 1;
        u64 rw: 1;
        u64 us: 1;
        u64 pwt: 1;
        u64 pcd: 1;
        u64 access: 1;
        u64 ignore1: 1;
        u64 pagesize: 1; //是否开启大页，4级分页时必须为0
        u64 ignore2: 3;
        u64 reserve: 1;
        u64 addr: 40;
        u64 ignore3: 11;
        u64 XD: 1;
    }__attribute__((packed));
} PDPT_ENTRY;

typedef struct {
    PDPT_ENTRY items[512];
} PDPT;

typedef union {
    u64 val;

    struct {
        u64 present: 1;
        u64 rw: 1;
        u64 us: 1;
        u64 pwt: 1;
        u64 pcd: 1;
        u64 access: 1;
        u64 ignore1: 1;
        u64 pagesize: 1; //是否开启大页，4级分页时必须为0
        u64 ignore2: 3;
        u64 reserve: 1;
        u64 addr: 40;
        u64 ignore3: 11;
        u64 XD: 1;
    }__attribute__((packed));
} PD_ENTRY;

typedef struct {
    PD_ENTRY items[512];
} PD;

typedef union {
    u64 val;

    struct {
        u64 present: 1;
        u64 rw: 1;
        u64 us: 1;
        u64 pwt: 1;
        u64 pcd: 1;
        u64 access: 1;
        u64 dirty: 1; //dirty位，指示此页是否被写过，由硬件置位
        u64 pat: 1; //缓存策略
        u64 global: 1; //全局位
        u64 ignore1: 2;
        u64 reserve: 1;
        u64 addr: 40;
        u64 ignore2: 7;
        u64 prot_key: 4; //protection key 保护位，当CR4.PKE or CR4.PKS=1时启用
        u64 XD: 1;
    }__attribute__((packed));
} PT_ENTRY;

typedef struct {
    PT_ENTRY items[512];
} PT;

#pragma pack(push,1)
typedef struct GDTR {
    unsigned short limit; //历史遗留问题：0x0000表示1字节，0xFFFF表示65536字节，故需要-1
    unsigned int address_start;
} GDTR;
#pragma pack(pop)

typedef struct {
    u64 mm_start;
    u64 mm_size;
    u64 mm_end; //[mm_start,mm_end)
    u64 page_total;
} MM_Entry;

typedef struct {
    int length;
    MM_Entry items[32];
} MM_Info;


void set_PML4i(PML4 *pml4, int index, PDPT *pdpt);

void set_PDPTi(PDPT *pdpt, int index, PD *pd);

void set_PDi_4k(PD *pdpt, int index, PT *pt);

void set_PTi_4k(PT *pt, int index, u64 addr);

void set_PDi_2M(PD *pd, int index, u64 addr);

void print_check_mm_info();

void mov_gdt();

void kernelPage_init();
#endif //MM_H
