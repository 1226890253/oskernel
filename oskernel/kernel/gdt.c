#include "gdt.h"
#include "string.h"
#include "kernel.h"
#include "types.h"

#define PML4_ADDR 0x8000
#define PDPT_ADDR 0x9000
#define PD_ADDR 0xa000

#define GDT_ADDR 0xb000

// static u64 __attribute__((aligned(4096))) PML4[4];
// static u64 __attribute__((aligned(4096))) PDPT[4];
// static u64 __attribute__((aligned(4096))) PD[4];

u64 (*pml4)[512]; //= (u64 (*)[512]) PML4_ADDR;
u64 (*pdpt)[512]; //= (u64 (*)[512]) PDPT_ADDR;
u64 (*pd)[512]; //= (u64 (*)[512]) PD_ADDR;

u64 (*gdt)[256];// = GDT_ADDR;
gdtr32 gdtr;

static inline void write_cr3(u32 v) {
    asm __volatile("mov %0, %%cr3"
        ::"r"(v):"memory");
}

static inline void read_cr3() {
    int cr3 = 0;
    asm __volatile("mov %%cr3, %0\n"
        :"=r"(cr3)::);
    printk("cr3: 0x%x\n", cr3);
}

static inline void set_cr4_pae() {
    asm __volatile("mov %%cr4, %%eax\n"
        "or  $(1<<5), %%eax\n"
        "mov %%eax, %%cr4\n"
        :
        :
        :"eax");
}

void read_cr4(void) {
    int cr4 = 0;
    asm __volatile("mov %%cr4, %0\n"
        :"=r"(cr4));
    printk("cr4: 0x%x\n", cr4);
}

static inline void set_cr0_pg() {
    asm __volatile("mov %%cr0, %%eax\n"
        "or $(1<<31), %%eax\n"
        "mov %%eax, %%cr0\n"::
        :"eax");
}

static inline void set_LME() {
    u32 lo, hi;
    asm __volatile("rdmsr": "=a"(lo),"=d"(hi): "c"(0xC0000080));
    lo |= (1 << 8);
    asm __volatile("wrmsr" :: "a"(lo),"d"(hi),"c"(0xC0000080));
}

static inline u64 read_lme() {
    u32 lo, hi;
    asm __volatile(
        "mov #0xC0000080, %%ecx\n"
        "rdmsr": "=a"(lo), "=d"(hi)::);
    u64 tmp = (u64) lo | (u64) hi << 32;
    return tmp;
}

static void init_x64_code_desp(int gdt_index) {
    gdt_desp *desp = &(*gdt)[gdt_index];

    desp->limit_low = 0;
    desp->base_low = 0;
    desp->type = 0b1000;
    desp->segment = 1;
    desp->DPL = 0;
    desp->present = 1;
    desp->limit_high = 0;
    desp->available = 0;
    desp->long_mode = 1;
    desp->big = 0;
    desp->granularity = 0;
    desp->base_high = 0;
}

static void init_x64_date_desp(int gdt_index) {
    gdt_desp *desp = &(*gdt)[gdt_index];
    desp->limit_low = 0;
    desp->base_low = 0;
    desp->type = 0b0010;
    desp->segment = 1;
    desp->DPL = 0;
    desp->present = 1;
    desp->limit_high = 0;
    desp->available = 0;
    desp->long_mode = 1;
    desp->big = 0;
    desp->granularity = 0;
    desp->base_high = 0;
}

static void init_x64_descriptor() {
    gdt = (u64 (*)[256]) GDT_ADDR;

    asm __volatile("sgdt %[gdtr]"
        :[gdtr] "=m" (gdtr)
        :
        :"memory");
    printk("gdt: base 0x%x, limit: 0x%x\n", gdtr.address_start, gdtr.limit);
    memset(gdt, 0, sizeof *gdt);
    memcpy(gdt, (void *) gdtr.address_start, gdtr.limit + 1);

    init_x64_code_desp(3);
    init_x64_date_desp(4);

    gdtr.address_start = gdt;
    gdtr.limit = sizeof(*gdt) - 1;

    printk("new gdt: base 0x%x, limit: 0x%x\n", gdtr.address_start, gdtr.limit);
    asm __volatile("lgdt %[gdtr]"
        :
        :[gdtr] "m"(gdtr)
        :"memory");
}

static void build_paging() {
    pml4 = (u64 (*)[512]) PML4_ADDR;
    pdpt = (u64 (*)[512]) PDPT_ADDR;
    pd = (u64 (*)[512]) PD_ADDR;

    memset(pml4, 0, sizeof *pml4);
    memset(pdpt, 0, sizeof *pdpt);
    memset(pd, 0, sizeof *pd);

    (*pml4)[0] = (u64) &(*pdpt) | 0b11;
    (*pdpt)[0] = (u64) &(*pd) | 0b11;
    (*pd)[0] = (0x0ull) | (1 << 7) | 0b11; //[0-2MIB)
    (*pd)[1] = (0x00200000ull) | (1 << 7) | 0b11; //[2Mib-4Mib)
    (*pd)[2] = (0x00400000ull) | (1 << 7) | 0b11; //[4Mib-6Mib)
    (*pd)[3] = (0x00600000ull) | (1 << 7) | 0b11; //[6Mib-8Mib)
    (*pd)[4] = (0x00800000ull) | (1 << 7) | 0b11; //[8Mib-10Mib)

    // PML4[0] = (u64) &PDPT | 0b11;
    // PDPT[0] = (u64) &PD | 0b11;
    // PD[0] = (0x0ull) | (1 << 7) | 0b11; //[0-2MIB)
    // PD[1] = (0x00200000ull) | (1 << 7) | 0b11; //[2Mib-4Mib)
}

void entry64() {
    build_paging();
    write_cr3((u64) &(*pml4));
    set_cr4_pae();
    set_LME();
    set_cr0_pg(); //这里进入64位长模式
    init_x64_descriptor(); //准备64位模式下代码段和数据段描述符
}
