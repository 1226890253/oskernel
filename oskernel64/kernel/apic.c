//
// Created by ljw on 25-8-10.
//
#include "apic.h"

#include <kernel.h>
#include <string.h>
#include <types.h>

static inline void cpuid(u32 leaf, u32 subleaf,u32 *a, u32 *b, u32 *c, u32 *d)
{
    u32 ra, rb, rc, rd;
    __asm__ volatile ("cpuid"
                      : "=a"(ra), "=b"(rb), "=c"(rc), "=d"(rd)
                      : "a"(leaf), "c"(subleaf));
    if (a) *a = ra; if (b) *b = rb; if (c) *c = rc; if (d) *d = rd;
}

void print_CPUID() {
    u32 a,b,c,d;

    // 1. 获取厂商
    cpuid(0, 0, &a, &b, &c, &d);
    char vendor[13];
    *(u32*)&vendor[0] = b;
    *(u32*)&vendor[4] = d;
    *(u32*)&vendor[8] = c;
    vendor[12] = 0;
    printk("Vendor: %s\n", vendor);

    // 2. 获取型号信息
    cpuid(1, 0, &a, &b, &c, &d);
    u32 stepping    = a & 0xF;
    u32 model       = (a >> 4) & 0xF;
    u32 family      = (a >> 8) & 0xF;
    u32 type        = (a >> 12) & 0x3;
    u32 extmodel    = (a >> 16) & 0xF;
    u32 extfamily   = (a >> 20) & 0xFF;

    u32 display_family = (family == 0x0F) ? (family + extfamily) : family;
    u32 display_model  = (family == 0x06 || family == 0x0F)
                                ? ((extmodel << 4) + model) : model;

    printk("Family: %u, Model: %u, Stepping: %u\n",
           display_family, display_model, stepping);

    // 3. 获取品牌字符串
    char brand[49];
    for (int i=0; i<3; i++) {
        cpuid(0x80000002 + i, 0, &a, &b, &c, &d);
        memcpy(brand + i*16, &a, 4);
        memcpy(brand + i*16 + 4, &b, 4);
        memcpy(brand + i*16 + 8, &c, 4);
        memcpy(brand + i*16 + 12, &d, 4);
    }
    brand[48] = 0;
    printk("Brand: %s\n", brand);

}
bool check_APIC_Timer_status() {
    u32 a,b,c,d;

    // 1) 查询最大基本 CPUID 叶
    cpuid(0x0, 0x0, &a, &b, &c, &d);
    u32 max_basic = a;

    if (max_basic < 0x6) {
        printk("CPUID.06H Not supported: ARAT cannot be determined (APIC timers may be affected by P/C status)\n");
        return false;
    }

    // 2) 读取 CPUID.06H:EAX，检查 ARAT（bit 2）
    cpuid(0x6, 0x0, &a, &b, &c, &d);
    bool arat = a & (1u << 2);

    if (arat) {
        printk("ARAT=1: APIC timers have a constant rate (not affected by P-state/C-state).\n");
        return true;
    } else {
        printk("ARAT=0: The APIC timer may temporarily stop or drift during deep C state or frequency scaling.\n");
        return true;
    }
}

bool check_APIC_TSC() {
    u32 a,b,c,d;
    cpuid(0x1, 0x0, &a, &b, &c, &d);
    if (c & (1u << 24)) {
        printk("APIC support TSC-Deadline model\n");
        return true;
    }
    printk("APIC does not support TSC-Deadline model\n");
    return false;
}
