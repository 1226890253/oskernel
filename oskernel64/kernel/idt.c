#include <io.h>
#include "idt.h"

#include <apic.h>
#include <kernel.h>
#include <types.h>

idt_entry idt[256]={0};
Idtr idtr;

u64 rdmsr(u32 msr) {
    u32 lo, hi;
    asm __volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(msr));
    return ((u64) hi << 32) | lo;
}

void wrmsr(u32 msr, u64 value) {
    u32 lo = (u32) value, hi = (u32) (value >> 32);
    asm __volatile("wrmsr"::"c"(msr),"a"(lo),"d"(hi));
}

void set_idt_gate(u32 index, void (*handler)(void), u8 ist, u8 type, u8 dpl,u8 sel) {
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

void lidt() {
    asm __volatile("lidt %0"::"m"(idtr));
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
            printk("GP General Protected!\n");
            die_halt("#GP",f);
        case 0x0E:
            printk("PF Page fault\n");
            decode_pf_err(f->error);
            unsigned long cr2;
            asm volatile("mov %%cr2, %0" : "=r"(cr2));
            printk("  CR2=%x\n", (unsigned long long)cr2);
            die_halt("#PF",f);
        case 0xF0:
            apic_timer_isr_c();
            return;
        case 0xFE:
            apic_error_isr_c();
            return;
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


void idt_init() {
    set_idt_gate(0x00,isr_0x00,0,14,0,0x18); //DE
    set_idt_gate(0x08,isr_0x08,0,14,0,0x18); //DF ist=2?
    set_idt_gate(0x0D,isr_0x0D,0,14,0,0x18); //GP
    set_idt_gate(0x0E,isr_0x0E,0,14,0,0x18); //PF ist=1?

    idtr.base = (u64)idt;
    idtr.limit = (sizeof idt) -1;
    lidt();
}


















