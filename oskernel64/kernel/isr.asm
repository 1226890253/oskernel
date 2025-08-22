[SECTION .text]
[BITS 64]

global isr_common_entry
global isr_common_entry_err

extern isr_dispatch_c

%macro make_isr_stub 1
    global isr_%1
isr_%1:
    push 0;
    push %1;
    jmp isr_common_entry
%endmacro

%macro make_isr_stub_err 1
    global isr_%1
isr_%1:
    push %1  ;vector
    jmp isr_common_entry_err
%endmacro

;/* ===== Intel/通用 x86 异常（Architecture-defined）===== */
make_isr_stub      0x00   ;/* #DE  Divide Error（除0），无 error-code */
make_isr_stub      0x01   ;/* #DB  Debug（调试），无 error-code */
make_isr_stub      0x02   ;/* NMI  不可屏蔽中断， 无 error-code */
make_isr_stub      0x03   ;/* #BP  Breakpoint（INT3），无 error-code */
make_isr_stub      0x04   ;/* #OF  Overflow（INTO），无 error-code */
make_isr_stub      0x05   ;/* #BR  BOUND Range Exceeded， 无 error-code */
make_isr_stub      0x06   ;/* #UD  Invalid Opcode， 无 error-code */
make_isr_stub      0x07   ;/* #NM  Device Not Available（no-Math），无 error-code */
make_isr_stub_err  0x08   ;/* #DF  Double Fault，          有 error-code（通常为0） */
make_isr_stub      0x09   ;/* (Reserved) Coprocessor Segment Overrun（历史保留），无 error-code */
make_isr_stub_err  0x0A   ;/* #TS  Invalid TSS，           有 error-code（选择子） */
make_isr_stub_err  0x0B   ;/* #NP  Segment Not Present，   有 error-code（选择子） */
make_isr_stub_err  0x0C   ;/* #SS  Stack-Segment Fault，   有 error-code（选择子/0） */
make_isr_stub_err  0x0D   ;/* #GP  General Protection，    有 error-code（选择子/0） */
make_isr_stub_err  0x0E   ;/* #PF  Page Fault，            有 error-code（位域） */
make_isr_stub      0x0F   ;/* (Reserved) 保留，            无 error-code */

make_isr_stub      0x10   ;/* #MF  x87 FPU Floating-Point Error， 无 error-code */
make_isr_stub_err  0x11   ;/* #AC  Alignment Check，              有 error-code（通常为0） */
make_isr_stub      0x12   ;/* #MC  Machine Check，                无 error-code（特殊处理） */
make_isr_stub      0x13   ;/* #XF  SIMD Floating-Point Exception，无 error-code */
make_isr_stub      0x14   ;/* #VE  Virtualization Exception（VMX/EPT），无 error-code */
make_isr_stub_err  0x15   ;/* #CP  Control Protection Exception（CET），有 error-code */
make_isr_stub      0x16   ;/* (Reserved) 保留，无 error-code */
make_isr_stub      0x17   ;/* (Reserved) 保留，无 error-code */
make_isr_stub      0x18   ;/* (Reserved) 保留，无 error-code */
make_isr_stub      0x19   ;/* (Reserved) 保留，无 error-code */
make_isr_stub      0x1A   ;/* (Reserved) 保留，无 error-code */
make_isr_stub      0x1B   ;/* (Reserved) 保留，无 error-code */
make_isr_stub      0xF0   ;/* Local APIC timer */
make_isr_stub      0xFE   ;/* Local APIC error */
make_isr_stub      0xFF   ;/* Local APIC spurious interrupt */

%macro PUSH_GPRS 0
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro POP_GPRS 0
    pop  r15
    pop  r14
    pop  r13
    pop  r12
    pop  r11
    pop  r10
    pop  r9
    pop  r8
    pop  rdi
    pop  rsi
    pop  rbp
    pop  rbx
    pop  rdx
    pop  rcx
    pop  rax
%endmacro

isr_common_entry:
    PUSH_GPRS

    mov r10,rsp
    lea r11, [r10+120+16]
    mov r12, [r11+8]
    test r12b, 3
    jz .Lno_swap_entry
    swapgs
.Lno_swap_entry:
    mov r13, rsp
    and r13, 0xF
    mov r14, 0
    jz .Laligned
    sub rsp, 8
    mov r14, 1
.Laligned:
    mov rdi, rsp
    call isr_dispatch_c

    test r14, r14
    jz .Lno_fix
    add rsp,8
.Lno_fix:
    test r12b, 3
    jz .Lno_swap_exit
    swapgs
.Lno_swap_exit:
    POP_GPRS
    add rsp,16
    iretq

isr_common_entry_err:
    PUSH_GPRS

    mov r10,rsp
    lea r11, [r10+120+16]
    mov r12, [r11+8] ;cs
    test r12b, 3
    jz .Lno_swap_entry_e
    swapgs
.Lno_swap_entry_e:
    mov r13,rsp
    and r13,0xF
    mov r14,0
    jz .Laligned_e
    sub rsp,8
    mov r14,1
.Laligned_e:
    mov rdi, rsp
    call isr_dispatch_c

    test r14, r14
    jz .Lno_fix_e
    add rsp, 8
.Lno_fix_e:
    test r12b,3
    jz .Lno_swap_exit_e
    swapgs
.Lno_swap_exit_e:
    POP_GPRS
    add rsp,16
    iretq


























