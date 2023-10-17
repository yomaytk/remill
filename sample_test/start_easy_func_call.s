.global _start
.section .text
.extern main

_start:
        bl      main        // call easy_func
        // exit system call
        mov     x8, #93         // syscall: exit
        mov     x8, x0
	ret
        svc     #0              // make syscall
