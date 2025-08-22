#ifndef MM_H
#define MM_H
#include "types.h"

#define PML4_ADDR 0x1000
#define PDPT_ADDR 0x2000
#define PD_ADDR 0x3000

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

typedef struct __attribute__((aligned(4096))){
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

typedef struct __attribute__((aligned(4096))){
    PDPT_ENTRY items[512];
} PDPT;

typedef union {
    u64 val;
    struct {// PS=0（指向下级PT）
        u64 present: 1;
        u64 rw: 1;
        u64 us: 1;
        u64 pwt: 1;
        u64 pcd: 1;
        u64 access: 1;
        u64 ignore1: 1;
        u64 pagesize: 1;
        u64 ignore2: 3;
        u64 reserve: 1;
        u64 addr: 40;
        u64 ignore3: 11;
        u64 XD: 1;
    }__attribute__((packed));
} PD_ENTRY_4K;

typedef struct __attribute__((aligned(4096))){
    PD_ENTRY_4K items[512];
} PD_4K;

typedef union {
    u64 val;
    struct { // PS=1（2MiB大页）
        u64 present:1, rw:1, us:1, pwt:1, pcd:1, a:1, d:1, pagesize:1, g:1, avail:3;
        u64 pat:1;      // bit12
        u64 rsvd:8;     // 20:13 = 0
        u64 addr:31;  // 51:21
        u64 avail2:7; u64 pk:4; u64 nx:1;
    } __attribute__((packed));
} PD_ENTRY_2M;

typedef struct __attribute__((aligned(4096))){
    PD_ENTRY_2M items[512];
} PD_2M;

typedef union {
    u64 val;

    struct {
        u64 present: 1;
        u64 rw: 1;
        u64 us: 1;
        //Page Write-Through: 0 Write-back,写操作先写缓存，再延迟写回内存
        // 1: Write-Through, 直写模式，写操作会同时更新缓存和内存
        u64 pwt: 1;
        u64 pcd: 1; //Page Cache Disable 禁用缓存
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

typedef struct __attribute__((aligned(4096))){
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

void set_PDPTi_2M(PDPT *pdpt, int index, PD_2M *pd);

void set_PDPTi_4K(PDPT *pdpt, int index, PD_4K *pd);

void set_PDi_4k(PD_4K *pdpt, int index, PT *pt);

void set_PTi_4k(PT *pt, int index, u64 addr);

void set_PDi_2M(PD_2M *pd, int index, u64 addr);

void print_check_mm_info();

void mov_gdt();

void kernelPage_init();
#endif //MM_H
