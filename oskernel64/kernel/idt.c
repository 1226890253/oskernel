#include <io.h>
#include "idt.h"

#include <kernel.h>
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

static void set_idt_gate(u32 index, void (*handler)(void), u8 ist, u8 type, u8 dpl,u8 sel) {
    u64 addr = (u64) handler;
    idt[index].value1 = 0;
    idt[index].value2 = 0;

    idt[index].seg_selector=sel;
    idt[index].offset_low = addr & 0xFFFF;
    idt[index].offset_middle = (addr >> 16) & 0xFFFF;
    idt[index].offset_high = (addr >> 32) & 0xFFFFFFFF;

    idt[index].ist = ist;
    idt[index].type = type; //14:中断门 15:陷阱门
    idt[index].DPL = dpl;

    idt[index].persent = 1;
}

static inline void lidt() {
    asm __volatile("lidt %0"::"m"(idtr));
}

// ---------------------- local APIC ----------------------
#define IA32_APIC_BASE_MSR 0x1B
#define APIC_ENABLE (1ull << 11)
#define APIC_BSP (1ull << 8)

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
// extern void isr_apic_timer_stub();
// extern void isr_apic_error_stub();

void isr_apic_timer_stub() {}
void isr_apic_error_stub() {}

static volatile u64 g_ticks = 0;

void apic_eoi() {
    lapic_write(LAPIC_EOI,0);
}

// c层处理函数（由汇编stub调用）
void apic_timer_isr_c() {
    g_ticks++;
    //中间的周期性工作
    apic_eoi();
}

void apic_error_isr_c() {
    apic_eoi();
}

typedef struct __attribute__((packed)) {
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rdi, rsi, rbp, rbx, rdx, rcx, rax;

    u64 vector;
    //如果是带错误码的中断，硬件还会额外压error
    u64 error;

    //硬件帧
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
}isr_frame;

static void dump_frame(const isr_frame *f) {
    printk("  RIP=%x CS=%x  RFLAGS=%x  RSP=%x SS=%x\n",
           (unsigned long long)f->rip, (unsigned long long)f->cs,
           (unsigned long long)f->rflags, (unsigned long long)f->rsp,
           (unsigned long long)f->ss);
    printk("  RAX=%x RBX=%x RCX=%x RDX=%x\n",
           (unsigned long long)f->rax, (unsigned long long)f->rbx,
           (unsigned long long)f->rcx, (unsigned long long)f->rdx);
    printk("  RSI=%x RDI=%x RBP=%x\n",
           (unsigned long long)f->rsi, (unsigned long long)f->rdi,
           (unsigned long long)f->rbp);
    printk("  R8 =%x R9 =%x R10=%x R11=%x\n",
           (unsigned long long)f->r8, (unsigned long long)f->r9,
           (unsigned long long)f->r10,(unsigned long long)f->r11);
    printk("  R12=%x R13=%x R14=%x R15=%x\n",
           (unsigned long long)f->r12,(unsigned long long)f->r13,
           (unsigned long long)f->r14,(unsigned long long)f->r15);
}

/* #PF 错误码解码 */
static void decode_pf_err(u64 err) {
    printk("  PF-err: P=%d W/R=%d U/S=%d RSVD=%d I/D=%d PK=%d SS=%d\n",
           (int)(err & 1), (int)((err>>1)&1), (int)((err>>2)&1),
           (int)((err>>3)&1), (int)((err>>4)&1), (int)((err>>5)&1),
           (int)((err>>6)&1));
}

__attribute__((noreturn))
static inline void die_halt(const char* reason, const isr_frame* f) {
    printk("\n==== KERNEL PANIC: %s (vector=0x%x, error=0x%x) ====\n",
           reason, (unsigned long long)f->vector, (unsigned long long)f->error);
    dump_frame(f);
    while (true) asm __volatile("cli; hlt");
}

//执行这个函数的时候已经是进入中断处理例程了
void isr_dispatch_c(isr_frame* f) {
    u8 vec = (u8)f->vector;
    switch (vec) {
        /* 0x00–0x1F: 架构异常（不写 EOI） */
        case 0x00:
            printk("#DE Divide Error\n");
            die_halt("#DE",f);
        case 0x08:
            printk("DF Double Fault!\n");
            die_halt("#DF",f);
        case 0x0D:
            printk("GP General Protected!");
            die_halt("#GP",f);
        case 0x0E:
            printk("PF Page fault");
            unsigned long cr2;
            asm volatile("mov %%cr2, %0" : "=r"(cr2));
            printk("CR2=%x\n", (unsigned long long)cr2);
            decode_pf_err(f->error);
            die_halt("#PF",f);
        default:
            if (vec >= 0x20) {//外部中断
                apic_eoi();
                return;
            }
    }
}

extern void isr_0x00();
extern void isr_0x01();
extern void isr_0x02();
extern void isr_0x03();
extern void isr_0x04();
extern void isr_0x05();
extern void isr_0x06();
extern void isr_0x07();
extern void isr_0x08();
extern void isr_0x09();
extern void isr_0x0A();
extern void isr_0x0B();
extern void isr_0x0C();
extern void isr_0x0D();
extern void isr_0x0E();
extern void isr_0x0F();
// ---------------------- 初始化入口 ----------------------
void apic_singleCore_timer_init() {
    cli();

    //1.关8259a
    pic_disable_all();

    //2.打开Local APIC
    u64 apic_r=rdmsr(IA32_APIC_BASE_MSR);
    apic_r |= APIC_ENABLE;  //开启APIC
    apic_r |= APIC_BSP;     //此核是引导核
    wrmsr(IA32_APIC_BASE_MSR,apic_r);

    //3.这里需要映射虚拟地址


    //4.配置IDT： 向量0XF0（本地计时器），0xFE（错误中断）
    set_idt_gate(0x00,isr_0x00,0,14,0,0x18); //DE
    set_idt_gate(0x08,isr_0x08,0,14,0,0x18); //DF ist=2?
    set_idt_gate(0x0D,isr_0x0D,0,14,0,0x18); //GP
    set_idt_gate(0x0E,isr_0x0E,0,14,0,0x18); //PF ist=1?

    // set_idt_gate(0xF0,isr_apic_timer_stub,0,14,0,0x8);
    // set_idt_gate(0xFE,isr_apic_error_stub,0,14,0,0x8);
    idtr.base = (u64)idt;
    idtr.limit = (sizeof idt) -1;
    lidt();

    //5.使能APIC （SVR bit 8）
    lapic_write(LAPIC_SVR, 0x100 | 0xFF);

    //6. 接收所有优先级中断
    lapic_write(LAPIC_TPR,0x00);

    //7.屏蔽 LINT0/LINT1，设置ERROR向量
    lapic_write(LAPIC_LVT_LINT0, 1u << 16);
    lapic_write(LAPIC_LVT_LINT1, 1u << 16);
    lapic_write(LAPIC_LVT_ERROR, 0xFE);

    // //8.配置本地APIC定时器,div by 16
    // lapic_write(LAPIC_TIMER_DIV, 0b0011);



    sti();

}




















