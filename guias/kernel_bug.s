.section .text
.global _start

_start:
    # 1. Configurar mtvec con el manejador
    la t0, bad_handler
    csrw mtvec, t0

    # 2. Provocar excepcion: Acceso a direccion de memoria nula (0x0)
    li t0, 0x0
    ld t1, 0(t0)

loop:
    j loop

bad_handler:
    # Manejador con error: No actualiza mepc antes de retornar
    csrr t0, mcause
    mret


