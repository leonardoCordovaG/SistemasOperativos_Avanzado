.section .text
.global _start

_start:
    # Configurar vector de traps
    la t0, bad_dispatcher
    csrw mtvec, t0

    # Pruebas de Syscalls
    # Prueba 1: Syscall 1 (Suma: a0 + a1) -> Esperado: 15 + 25 = 40
    li a7, 1
    li a0, 15
    li a1, 25
    ecall

    # Prueba 2: Syscall 2 (Multiplicación: a0 * a1) -> Esperado: 6 * 7 = 42
    li a7, 2
    li a0, 6
    li a1, 7
    ecall

main_loop:
    j main_loop

.align 4
bad_dispatcher:
    csrr t0, mcause
    li t1, 11
    bne t0, t1, trap_error

    # --- DESPACHADOR DEFECTUOSO ---
    li t1, 1
    beq a7, t1, do_add

    li t1, 2
    beq a7, t1, do_mul

    li a0, -1
    j exit_sys

do_add:
    # Error 1: Sobrescribe a0 como registro temporal destruyendo el parametro
    li a0, 0
    add a0, a0, a1
    j exit_sys

do_mul:
    mul a0, a0, a1
    j exit_sys

exit_sys:
    # Error 2: Falta actualizar mepc (+4 bytes) antes de retornar
    mret

trap_error:
    1: j 1b