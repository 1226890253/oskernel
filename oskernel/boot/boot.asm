[ORG 0x7c00]

%include "/home/ljw/CLionProjects/test1/oskernel/boot/common.data"

[SECTION .text]
[BITS 16]
global _start
_start:
   mov ax, 3
   int 0x10

   mov edi, SETUP_ADDR_BASE
   mov ecx, 1
   mov bl, 2
   call read_hd

   mov si, jmp_to_setup
   call print

   jmp SETUP_ADDR_BASE

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

jmp_to_setup:
    db "jump to setup...", 10, 13, 0

times 510 - ( $ - $$) db 0
db 0x55, 0xaa