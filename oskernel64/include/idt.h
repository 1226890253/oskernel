#ifndef IDT_H
#define IDT_H
#include <types.h>

#define PIC_M_CTRL  0x20    // 主片的控制端口
#define PIC_M_DATA  0x21    // 主片的数据端口
#define PIC_S_CTRL  0xa0    // 从片的控制端口
#define PIC_S_DATA  0xa1    // 从片的数据端口
#define PIC_EOI     0x20    // 通知中断控制器中断结束

#define IDT_SIZE 256

typedef union{
    u64 value1;
    u64 value2;
    struct __attribute__((packed)){
        u16 offset_low;
        u16 seg_selector;

        u8 ist;  //Interrupt Stack Table,为0默认不使用
        u8 type:4;  //详见表  14为中断门，15为陷阱门
        u8 reserved:1;
        u8 DPL:2; //允许进入该门的特权级
        u8 persent:1;

        u16 offset_middle;
        u32 offset_high;
        u32 reserved2;
    };
}idt_entry;

typedef struct __attribute__((packed))  {
    u16 limit;  //注意，此limit也需要移位
    u64 base;  //表基址
}Idtr;

u64 rdmsr(u32 msr);
void wrmsr(u32 msr, u64 value);
void set_idt_gate(u32 index, void (*handler)(void), u8 ist, u8 type, u8 dpl,u8 sel);
void lidt();
void idt_init();
void cli();
void sti();
#endif //IDT_H
