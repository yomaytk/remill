.global _start
.section .text
.extern main

_start:
        bl      main        // call easy_func
        // exit system call
        mov     r8, #93         // syscall: exit
        mov     r8, r0
        bx      lr
        svc     #0              // make syscall
