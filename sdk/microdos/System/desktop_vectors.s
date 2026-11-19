.section .header, "ax"
.globl desktop_vectors
desktop_vectors:
    jmp desktop_shell
    jmp desktop_init
    jmp desktop_tick
    jmp desktop_redraw
