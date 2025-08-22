//
// Created by ljw on 25-8-10.
//
#include "apic.h"

#include <idt.h>
#include <io.h>
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

// ---------------------- local APIC ----------------------
#define IA32_APIC_BASE_MSR 0x1B
#define APIC_ENABLE (1ull << 11)
#define APIC_BSP (1ull << 8)

extern idt_entry idt[256];
extern Idtr idtr;
static volatile u32 *lapic = (volatile u32 *) 0xFEE00000;


enum {
    LAPIC_ID = 0x020,
    LAPIC_VER = 0x030, //APIC 版本号 + 支持的 LVT 条目数
    LAPIC_TPR = 0x080, //当前任务优先级（0~255），高优先级中断屏蔽低优先级中断
    LAPIC_EOI = 0x0B0, //写任意值通知 APIC 中断处理结束
    LAPIC_SVR = 0x0F0, //设置 Spurious 向量号，bit8 用于全局使能 APIC
    LAPIC_LVT_TIMER = 0x320, //本地 APIC 定时器配置（向量号、触发模式等）
    LAPIC_LVT_LINT0 = 0x350, //LINT0 引脚中断配置（可设为 NMI、外部中断、屏蔽等）
    LAPIC_LVT_LINT1 = 0x360, //LINT1 引脚中断配置（常用于 NMI）
    LAPIC_LVT_ERROR = 0x370, //APIC 错误中断配置
    LAPIC_TIMER_INITCNT = 0x380, //APIC Timer 初始计数值
    LAPIC_TIMER_CURCNT = 0x390, //APIC Timer 当前计数值
    LAPIC_TIMER_DIV = 0x3E0 //APIC Timer 分频设置
};

static inline void lapic_write(u32 off, u32 val) {
    lapic[off / 4] = val;
    (void) lapic[LAPIC_ID / 4];
}

static inline u32 lapic_read(u32 off) {
    return lapic[off / 4];
}

// ---------------------- 8259a PIC Soft-off ----------------------
static inline void pic_disable_all() {
    out_byte(0x21, 0xFF);
    out_byte(0xA1, 0xFF);
}

// ---------------------- Timer ISR ----------------------
extern void isr_0xF0();
extern void isr_0xFE();
extern void isr_0xFF();

static volatile u64 g_ticks = 0;

void apic_eoi() {
    lapic_write(LAPIC_EOI,0);
}

// c层处理函数（由汇编stub调用）
void apic_timer_isr_c() {
    g_ticks++;
    //中间的周期性工作
    //printk("APIC timer tick!\n,%d",g_ticks);

    apic_eoi();
}

void apic_error_isr_c() {
    apic_eoi();
}

static bool check_apic() {
    u32 a,b,c,d;
    asm __volatile(
        "cpuid\n"
        :"=a"(a), "=b"(b), "=c"(c), "=d"(d)
        :"a"(1),"c"(0));
    bool is_apic= (d & (1u << 9)) != 0;
    printk("APIC=%s\n",is_apic? "yes":"no");
    return is_apic;
}
static bool check_apic2() {
    u32 a,b,c,d;
    asm __volatile(
        "cpuid\n"
        :"=a"(a), "=b"(b), "=c"(c), "=d"(d)
        :"a"(1),"c"(0));
    bool is_apic2 = (c & (1u << 21)) != 0;
    printk("APIC2=%s\n",is_apic2? "yes":"no");
    return is_apic2;
}

#define PIT_CH2     0x42
#define PIT_CMD     0x43
#define PIT_FREQUENCY 1193182

void pit_delay(u32 ms) {
    u32 ticks = PIT_FREQUENCY / 1000 * ms;

    // 通道2, 方式0 (one-shot)
    out_byte(PIT_CMD, 0xB2);

    // 写 reload 值
    out_byte(PIT_CH2, ticks & 0xFF);
    out_byte(PIT_CH2, (ticks >> 8) & 0xFF);

    // 轮询等待 (PIT 通道 2 输出 pin)
    while (!(in_byte(0x61) & 0x20));
}
u32 calibrate_apic_timer() {
    // 设定 divide=1
    lapic_write(LAPIC_TIMER_DIV, 0xB);

    // 装载一个很大的初值
    lapic_write(LAPIC_TIMER_INITCNT, 0xFFFFFFFF);

    // 启动 PIT 延时 10ms
    pit_delay(10);

    // 读剩余计数
    u32 cur = lapic_read(LAPIC_TIMER_CURCNT);

    u32 ticks = 0xFFFFFFFF - cur;
    u32 apic_freq = ticks * (100); // 10ms → 放大100倍 = Hz
    return apic_freq;
}

void apic_timer_init(u32 apic_freq, float hz) {
    lapic_write(LAPIC_TIMER_DIV, 0xB); // divide=1

    // 配置 LVT Timer = periodic 0x20作为时钟中断号,周期触发，不屏蔽中断
    lapic_write(LAPIC_LVT_TIMER, 0xF0 | (1<<17) | (0<<16));
    set_idt_gate(0xF0,isr_0xF0,0,14,0,0x18);

    // 设置初始计数
    u32 init_count = apic_freq / hz;
    lapic_write(LAPIC_TIMER_INITCNT, init_count);
}

void apic_init() {

    if (!check_apic()) {
        return;
    }
    
    cli();

    //1.关8259a
    pic_disable_all();

    //2.打开Local APIC
    u64 apic_r=rdmsr(IA32_APIC_BASE_MSR);
    apic_r |= APIC_ENABLE;  //开启APIC
    apic_r |= APIC_BSP;     //此核是引导核
    wrmsr(IA32_APIC_BASE_MSR,apic_r);

    //3.这里需要映射虚拟地址


    //4.配置IDT： 向量0xF0（本地计时器），0xFE（错误中断）,0xFF(虚拟中断)
    set_idt_gate(0xFE,isr_0xFE,0,14,0,0x18);
    set_idt_gate(0xFF,isr_0xFF,0,14,0,0x18);

    //5.使能APIC （SVR bit 8） 中断向量号为 0xFF
    lapic_write(LAPIC_SVR, 0x100 | 0xFF); 

    //6. 接收所有优先级中断
    lapic_write(LAPIC_TPR,0x00);

    //7.屏蔽 LINT0/LINT1，设置ERROR向量
    lapic_write(LAPIC_LVT_LINT0, 1u << 16);
    lapic_write(LAPIC_LVT_LINT1, 1u << 16);
    lapic_write(LAPIC_LVT_ERROR, 0xFE);

    //8.配置本地APIC定时器,div by 1，周期模式
    if (!check_APIC_Timer_status()) {
        return;
    }

    u32 freq = calibrate_apic_timer();
    printk("APIC Timer Frequency: %d Hz\n", freq);

    apic_timer_init(freq,0.1);

    sti();
}