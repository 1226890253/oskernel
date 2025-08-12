#include <io.h>
#include "idt.h"

#include <types.h>

static idt_entry idt[256];
static Idtr idtr;

static inline u64 rdmsr(u32 msr) {
    u32 lo, hi;
    asm __volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(msr));
    return ((u64) hi << 32) | lo;
}

static inline void wrmsr(u32 msr, u64 value) {
    u32 lo = (u32) value, hi = (u32) (value >> 32);
    asm __volatile("wrmsr"::"c"(msr),"a"(lo),"d"(hi));
}

static inline void cli() { asm volatile("cli"); }
static inline void sti() { asm volatile("sti"); }

static void idt_set_gate(u32 index, u64 offset, void (*handler)(void), u8 ist, u8 type, u8 dpl) {
    u64 addr = (u64) handler;
    idt[index].value1 = 0;
    idt[index].value2 = 0;

    idt[index].offset_low = addr & 0xFFFF;
    idt[index].offset_middle = (addr >> 16) & 0xFFFF;
    idt[index].offset_high = (addr >> 32) & 0xFFFFFFFF;

    idt[index].ist = ist;
    idt[index].type = type;
    idt[index].DPL = dpl;

    idt[index].persent = 1;
}

static inline u32 *lidt(void *base, u16 limit) {
    Idtr d = {.base = (u64) base, .limit = limit - 1};
    asm __volatile("lidt %0"::"m"(d));
}

// ---------------------- local APIC ----------------------
#define IA32_APIC_BASE_MSR 0x1B
#define APIC_ENABLE (1ull << 11)

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
static inline pic_disable_all() {
    out_byte(0x21, 0xFF);
    out_byte(0xA1, 0xFF);
}

// ---------------------- Timer ISR ----------------------
extern void isr_apic_timer_stub();
extern void isr_apic_error_stub();

static volatile u64 g_ticks = 0;

static void apic_eoi() {
    lapic_write(LAPIC_EOI,0);
}

// c层处理函数（由汇编stub调用）
void apic_timer_isr_c() {
    g_ticks++;
    //中间的周期性工作
    apic_eoi();
}




























