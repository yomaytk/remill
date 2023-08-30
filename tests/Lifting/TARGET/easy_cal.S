.globl __entry0
.globl __mhex

__entry0:
.text
.data
    # bl easy_cal
    mov x8, 0x5d
    svc 0

__mhex:
.text
.data
    sub sp, sp, 0x10
    str w0, [sp, #0xc]
    mov w8, 1
    str w8, [sp, #0x8]
    b _40024c
_40024c:
    ldr w8, [sp, #0xc]
    ldr w9, [sp, #0x8]
    sdiv w8, w8, w9
    subs w8, w8, 9
    b.le _400278
_400260:
    b _400264
_400264:
    ldr w8, [sp, #0x8]
    mov w9, 0xa
    mul w8, w8, w9
    str w8, [sp, #0x8]
    b _40027c
_400278:
    b _400280
_40027c:
    b _40024c
_400280:
    ldr w0, [sp, #0x8]
    add sp, sp, #0x20
    ret
