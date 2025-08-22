#include "mm.h"

#include <bitmap.h>
#include <tss.h>

#include "string.h"
#include "types.h"
#include "kernel.h"
#include "math.h"

#define ARDS_ADDR 0x7000

MM_Info mm_info;
PD_4K pd_APIC;
PT pt_APIC;

void print_check_mm_info() {
    check_mm_info *p = (check_mm_info *) (ARDS_ADDR);
    ARDS *ards = (ARDS *) (ARDS_ADDR + 2);

    unsigned short times = p->times;

    printk("===== memory check info =====\n");
    mm_info.length = times;
    for (int i = 0; i < times; i++) {
        ARDS *tmp = ards + i;
        u64 address = ((u64) tmp->baseAddrHigh << 32) + tmp->baseAddrLow;
        u64 length = ((u64) tmp->lengthHigh << 32) + tmp->lengthLow;
        if (tmp->type == 1) {
            mm_info.items[i].mm_start = address;
            mm_info.items[i].mm_size = length;
            mm_info.items[i].mm_end = address + length;
            mm_info.items[i].page_total = ceil((double) length / (double) 0x1000);
        }
        printk("\t Address= 0x%X, lengh= 0x%X, type = %d\n", address, length, tmp->type);
    }

    printk("===== memory check info end=====\n");
}

void kernelPage_init() {
    PML4 *pml4 = PML4_ADDR;
    PDPT *pdpt = PDPT_ADDR;
    PD_2M *pd_2m = PD_ADDR;

    set_PML4i(pml4,0,pdpt);
    set_PDPTi_2M(pdpt,0,pd_2m);
    //管理0-10M内存地址，采用线性内存
    set_PDi_2M(pd_2m,0,0x0);
    set_PDi_2M(pd_2m,1,0x200000);
    set_PDi_2M(pd_2m,2,0x400000);
    set_PDi_2M(pd_2m,3,0x600000);
    set_PDi_2M(pd_2m,4,0x800000);
    set_PDi_2M(pd_2m,5,0xa00000);

    //映射0xFEE0_0000，APIC MMIO内存,大小4k
    PD_4K* pd_apic = &pd_APIC;
    PT* pt_apic= &pt_APIC;

    set_PDPTi_4K(pdpt,3,pd_apic);
    set_PDi_4k(pd_apic, 503, pt_apic);
    set_PTi_4k(pt_apic, 0 ,0xFEE00000);
    pt_apic->items[0].pwt=1;
    pt_apic->items[0].pcd=1;


    asm __volatile(
        "mov %0, %%cr3\n"
        :
        :"r"(pml4));
    printk("kernel page init=====\n");
}

void set_PML4i(PML4 *pml4, int index, PDPT *pdpt) {
    pml4->items[index].val = 0;
    pml4->items[index].present = 1;
    pml4->items[index].rw = 1;
    pml4->items[index].us = 0;
    pml4->items[index].addr = (u64) pdpt >> 12;
}

void set_PDPTi_2M(PDPT *pdpt, int index, PD_2M *pd) {
    pdpt->items[index].val = 0;
    pdpt->items[index].present = 1;
    pdpt->items[index].rw = 1;
    pdpt->items[index].us = 0;
    pdpt->items[index].addr = (u64) pd >> 12;
}

void set_PDi_2M(PD_2M *pd, int index, u64 addr) {
    pd->items[index].val = 0;
    pd->items[index].present = 1;
    pd->items[index].rw = 1;
    pd->items[index].us = 0;
    pd->items[index].pagesize=1;  //开启2mb大页
    pd->items[index].addr = addr >> 21;
}


void set_PDPTi_4K(PDPT *pdpt, int index, PD_4K *pd) {
    pdpt->items[index].val = 0;
    pdpt->items[index].present = 1;
    pdpt->items[index].rw = 1;
    pdpt->items[index].us = 0;
    pdpt->items[index].addr = (u64) pd >> 12;
}

void set_PDi_4k(PD_4K *pd, int index, PT *pt) {
    pd->items[index].val = 0;
    pd->items[index].present = 1;
    pd->items[index].rw = 1;
    pd->items[index].us = 0;
    pd->items[index].addr = (u64) pt >> 12;
}

void set_PTi_4k(PT *pt, int index, u64 addr) {
    pt->items[index].val = 0;
    pt->items[index].present = 1;
    pt->items[index].rw = 1;
    pt->items[index].us = 0;
    pt->items[index].addr = addr >> 12;
}




void mov_gdt() {
    GDTR gdtr;
    asm __volatile("sgdt %[gdtr]"
        :[gdtr] "=m" (gdtr)
        :
        :"memory");
    memcpy(GDT_ADDR, gdtr.address_start, gdtr.limit + 1);
    gdtr.address_start = GDT_ADDR;
    asm __volatile("lgdt %[gdtr]"
        :
        :[gdtr] "m" (gdtr)
        :"memory");
    printk("success set gdtrAddr= 0x%X,limit=0x%X\n", gdtr.address_start, gdtr.limit);

    //tss_init_and_load(gdtr.address_start,5);
}

// void mm_apply(u32 mm_size, ) {
//
// }