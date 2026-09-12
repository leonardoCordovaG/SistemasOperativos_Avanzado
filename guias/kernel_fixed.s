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
    csrr t0, mcause         # leer la causa del trap (0x5 = Load access fault)
    csrr t1, mepc           # t1 = direccion de la instruccion que fallo (la 'ld')
    addi t1, t1, 4          # avanzar al siguiente instruccion (salta la 'ld')
    csrw mepc, t1           # actualizar el punto de retorno
    mret                    # retorna a 'loop'
