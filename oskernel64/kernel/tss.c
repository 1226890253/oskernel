
#define PACKED  __attribute__((packed))
#define ALIGNED(x) __attribute__((aligned(x)))
#include <string.h>
#include <types.h>
#include "tss.h"

/* ---------------- 64-bit TSS 定义（Intel SDM） ---------------- */
struct PACKED tss64 {
    u32 reserved0;
    u64 rsp0;      // CPL=0 的栈
    u64 rsp1;
    u64 rsp2;
    u64 reserved1;
    u64 ist1;      // IST1..IST7
    u64 ist2;
    u64 ist3;
    u64 ist4;
    u64 ist5;
    u64 ist6;
    u64 ist7;
    u64 reserved2;
    u16 reserved3;
    u16 iopb_offset; // I/O 位图相对 TSS 基址的偏移
};

/* 65536 个端口 → 8192 字节位图；最后必须再加 1 字节，全 1 */
#define IO_BITMAP_BYTES (65536/8)
struct PACKED ALIGNED(16) tss_block {
    struct tss64  tss;
    u8       io_bitmap[IO_BITMAP_BYTES + 1];
};

/* ----------- TSS 全局对象 + 栈（示例：单 CPU） ---------------- */
static struct tss_block cpu0_tss;

static u8 ALIGNED(16) kstack0[16 * 1024];  // RSP0：内核主栈
static u8 ALIGNED(16) ist1_stack[8 * 1024]; // IST1：给 #PF
static u8 ALIGNED(16) ist2_stack[8 * 1024]; // IST2：给 #DF

static inline u64 stack_top(void *base, size_t size) {
    return (u64)base + (u64)size;
}

/* ---------------- 16 字节 TSS 描述符结构 ---------------- */
struct PACKED tss_desc {
    u16 limit0;       // [15:0]
    u16 base0;        // [15:0]
    u8  base1;        // [23:16]
    u8  type;         // 访问字节：P|DPL|Type（TSS64 Avail=0x9）
    u8  limit1_flags; // limit[19:16] | flags（G=0, AVL=0, 等）
    u8  base2;        // [31:24]
    u32 base3;        // [63:32]
    u32 reserved;
};

/* 将 64 位 TSS 基址与 limit 写入 16 字节描述符 */
static void fill_tss_descriptor(struct tss_desc *d, u64 base, u32 limit)
{
    memset(d, 0, sizeof(*d));
    d->limit0 = (u16)(limit & 0xFFFF);
    d->base0  = (u16)(base  & 0xFFFF);
    d->base1  = (u8)((base  >> 16) & 0xFF);
    d->type   = 0x89;                // P=1(0x80) | Type=0x9(Available 64-bit TSS), DPL=0
    d->limit1_flags = (u8)((limit >> 16) & 0x0F); // flags 高 4 位留 0（G=0）
    d->base2  = (u8)((base  >> 24) & 0xFF);
    d->base3  = (u32)((base >> 32) & 0xFFFFFFFF);
}

/* ----------------- 对外初始化入口 ----------------- */
/**
 * @param gdt_base   指向当前生效 GDT 的线性地址（可写）
 * @param tss_index  在 GDT 中放置 TSS 描述符的槽位索引（按 8 字节计）
 *                   注意：TSS 占用两个槽位（16 字节），从 tss_index 开始的连续两项
 * @return           返回可直接用于 ltr 的 selector（tss_index<<3）
 */
u16 tss_init_and_load(void *gdt_base, int tss_index)
{
    /* 1) 清零 TSS + I/O 位图，并填 rsp0/istN */
    memset(&cpu0_tss, 0, sizeof(cpu0_tss));

    cpu0_tss.tss.rsp0 = stack_top(kstack0, sizeof(kstack0));
    cpu0_tss.tss.ist1 = stack_top(ist1_stack, sizeof(ist1_stack)); // 建议给 #PF
    cpu0_tss.tss.ist2 = stack_top(ist2_stack, sizeof(ist2_stack)); // 建议给 #DF

    /* I/O 位图：全 1 = 拒绝所有端口 I/O（用户态） */
    memset(cpu0_tss.io_bitmap, 0xFF, sizeof(cpu0_tss.io_bitmap));
    /* iopb_offset 指向位图起始（相对 TSS 基址） */
    cpu0_tss.tss.iopb_offset = (u16)((unsigned long int)(&cpu0_tss.io_bitmap[0]) - (unsigned long int)&cpu0_tss.tss);

    /* 2) 在 GDT 写入 16B TSS 描述符（limit 必须覆盖 TSS+位图） */
    u64 tss_base  = (u64)&cpu0_tss.tss;
    u32 tss_limit = (u32)(sizeof(cpu0_tss) - 1);

    struct tss_desc desc;
    fill_tss_descriptor(&desc, tss_base, tss_limit);

    /* GDT 写入：TSS 占 16 字节（两个 8B 槽位） */
    u8 *gdt = (u8 *)gdt_base;
    u8 *slot = gdt + (tss_index * 8);
    memcpy(slot, &desc, sizeof(desc));

    /* 3) ltr 载入 TR */
    u16 sel = (u16)(tss_index << 3);  // TI=0, RPL=0
    __asm__ __volatile__("ltr %0" : : "r"(sel) : "memory");

    return sel;
}