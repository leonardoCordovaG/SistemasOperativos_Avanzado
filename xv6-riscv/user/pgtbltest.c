#include "kernel/types.h"
#include "kernel/riscv.h"
#include "user/user.h"

static int fallos = 0;

static void pgaccess_test(void)
{
  const int N = 32;           // paginas a reservar / inspeccionar
  uint32 mask = 0;

  char *buf = sbrk(N * PGSIZE);
  if (buf == (char *)-1) {
    printf("pgaccess_test: sbrk fallo\n");
    fallos++;
    return;
  }

  // Toca (escribe) solo algunas paginas: 0, 5, 17, 29.
  int tocadas[] = {0, 5, 17, 29};
  int ntocadas = sizeof(tocadas) / sizeof(tocadas[0]);
  uint32 esperado = 0;
  for (int k = 0; k < ntocadas; k++) {
    buf[tocadas[k] * PGSIZE] = 42;
    esperado |= (1U << tocadas[k]);
  }

  // Primera inspeccion: deben aparecer exactamente las paginas tocadas.
  if (pgaccess(buf, N, &mask) < 0) {
    printf("pgaccess_test: pgaccess() devolvio error\n");
    fallos++;
    return;
  }

  printf("pgaccess_test: mask     = 0x%x\n", mask);
  printf("pgaccess_test: esperado = 0x%x\n", esperado);

  if (mask != esperado) {
    printf("pgaccess_test: FALLO (mascara distinta a la esperada)\n");
    fallos++;
  } else {
    printf("pgaccess_test: OK - deteccion correcta\n");
  }

  // Segunda inspeccion inmediata: pgaccess ya limpio PTE_A y no hemos
  // vuelto a tocar 'buf', asi que la mascara debe ser 0.
  mask = 0;
  if (pgaccess(buf, N, &mask) < 0) {
    printf("pgaccess_test: segunda llamada devolvio error\n");
    fallos++;
    return;
  }
  if (mask != 0) {
    printf("pgaccess_test: FALLO - PTE_A no se limpio (mask = 0x%x)\n", mask);
    fallos++;
  } else {
    printf("pgaccess_test: OK - PTE_A se limpio correctamente\n");
  }
}

static void
pgaccess_err_test(void)
{
  uint32 mask = 0;
  // len fuera de rango debe fallar.
  if (pgaccess((void *)0, 33, &mask) >= 0) {
    printf("pgaccess_err_test: FALLO - len > 32 deberia dar error\n");
    fallos++;
  } else {
    printf("pgaccess_err_test: OK - rechaza len invalido\n");
  }
}

int
main(void)
{
  printf("=== pgtbltest ===\n");
  pgaccess_test();
  pgaccess_err_test();

  if (fallos == 0)
    printf("=== pgtbltest: TODOS LOS TESTS PASARON ===\n");
  else
    printf("=== pgtbltest: %d test(s) FALLARON ===\n", fallos);

  exit(fallos == 0 ? 0 : 1);
}
