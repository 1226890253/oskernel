; 0柱面0磁道2扇區
[ORG  0x500]

[SECTION .data]
KERNEL_ADDR equ 0x1200

[SECTION .gdt]
SEG_BASE equ 0
SEG_LIMIT equ 0xfffff

;段选择子，因为ti和PRL都是0，所以不需要额外设置
CODE_SELECTOR equ (1 << 3)
DATA_SELECTOR equ (2 << 3)

gdt_base:
    dd 0, 0
gdt_code:
    dw SEG_LIMIT & 0xffff
    dw SEG_BASE & 0xffff
    db SEG_BASE >> 16 & 0xff
    db 0b1001_1000
    db 0b0100_0000 | (SEG_LIMIT >> 16 & 0xf)
    db SEG_BASE >> 24 & 0xff

gdt_data:
    dw SEG_LIMIT & 0xffff
    dw SEG_BASE & 0xffff
    db SEG_BASE >> 16 & 0xff
    db 0b1001_0010
    db 0b1100_0000 | (SEG_LIMIT >> 16 & 0xf)
    db SEG_BASE >> 24 & 0xff
gdt_ptr:
    dw $ - gdt_base
    dd gdt_base

[SECTION .text]
[BITS 16]
global setup_start
setup_start:
    mov     ax, 0
    mov     ss, ax
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     si, ax

    mov     si, prepare_enter_protected_mode_msg
    call    print
    ;关中断并进入保护模式
    cli
    lgdt [gdt_ptr] ;设置gdt表

    ;开启A20
    in al, 0x92
    or al, 0b0000_0010
    out 0x92, al

    ;开启保护模式
    mov eax, cr0
    or eax, 1
    mov cr0,eax

    jmp CODE_SELECTOR:protected_mode

print:
    mov ah, 0x0e
    mov bh, 0
    mov bl, 0x01
.loop:
    mov al, [si]
    cmp al, 0
    jz .done
    int 0x10

    inc si
    jmp .loop
.done:
    ret

[BITS 32]
protected_mode:
    mov ax, DATA_SELECTOR
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov esp, 0x9fbff

    ;将内核读入内存
    mov edi, KERNEL_ADDR
    mov ecx, 3  ;从第3块盘起读60块盘
    mov bl, 60
    call read_hd

    jmp CODE_SELECTOR:KERNEL_ADDR


read_hd:
    mov dx, 0x1f2
    mov al, bl
    out dx, al

    inc dx      ;0x1f3
    mov al, cl
    out dx, al

    inc dx      ;0x1f4
    mov al, ch
    out dx, al

    inc dx      ;0x1f5
    shr ecx, 16
    mov al, cl
    out dx, al

    inc dx      ;0x1f6
    ;shr ecx, 8
    and ch, 0b0000_1111
    mov al, 0b1110_0000
    or al, ch
    out dx, al

    inc dx
    mov al, 0x20
    out dx, al

    mov cl, bl

.start_read:
    push cx

    call .wait_hd_prepare
    call read_hd_data

    pop cx
    loop .start_read

    ret

.wait_hd_prepare:
    mov dx, 0x1f7

.check:
    in al, dx
    and al, 0b1000_1000
    cmp al, 0b0000_1000
    jnz .check
    ret

read_hd_data:
    mov dx, 0x1f0
    mov cx, 256

.read_word:
    in ax, dx
    mov [edi], ax
    add edi, 2
    loop .read_word

    ret

prepare_enter_protected_mode_msg:
    db "prepare to go into protected mode", 10, 13, 0





















