extern void main();

void _start() {
    asm volatile(
        "    bl main\n"
        "    mov x8, #93\n"
        "    ret\n"
    );
}