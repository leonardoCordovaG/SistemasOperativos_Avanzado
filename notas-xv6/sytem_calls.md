# System calls en xv6-riscv

## Cómo viaja una llamada al sistema (la "cadena de montaje")

Cada syscall toca **6 sitios**. De usuario a kernel:

| # | Archivo | Rol |
|---|---|---|
| 1 | [user/user.h](../xv6-riscv/user/user.h) | Prototipo C que ve el programa de usuario (`int fork(void);`). |
| 2 | [user/usys.pl](../xv6-riscv/user/usys.pl) | Script Perl que **genera** `usys.S`: el stub que mete el número de syscall en el registro `a7` y ejecuta `ecall` (salto a modo supervisor). |
| 3 | [kernel/syscall.h](../xv6-riscv/kernel/syscall.h) | `#define SYS_xxx N` — asigna el número a cada syscall. |
| 4 | [kernel/syscall.c](../xv6-riscv/kernel/syscall.c) | `extern` del handler + entrada en la tabla `syscalls[]` (y en `syscall_names[]`). La función `syscall()` lee `a7`, busca en la tabla y llama al handler; el retorno se guarda en `a0`. |
| 5 | `kernel/sysproc.c` / `sysfile.c` / `log.c` | El handler `sys_xxx(void)`: lee argumentos con `argint/argaddr/argstr` y llama a la lógica real del kernel. |
| 6 | `kernel/proc.c`, `fs.c`, `file.c`, … | La implementación de verdad (`kfork`, `kexit`, `filewrite`, …). |

Argumentos: el handler no recibe parámetros C; los saca del `trapframe` con
`argint(n, &x)` (entero), `argaddr(n, &p)` (puntero) o `argstr(n, buf, max)` (string).

---

## Listado de system calls

> Nota: esta copia de xv6 está **modificada por el curso**. Respecto al xv6 original:
> `sleep` se renombró a `pause`, y se añadieron `sync`, `getreadcount` y `trace`.
> `sbrk` recibe un 2º argumento (asignación *lazy* vs *eager*).

### Gestión de procesos — handlers en [kernel/sysproc.c](../xv6-riscv/kernel/sysproc.c)

| Syscall | Qué hace | Handler | Lógica real |
|---|---|---|---|
| `fork()` | Crea un proceso hijo copiando el actual; devuelve 0 al hijo y el pid del hijo al padre. | `sys_fork` [sysproc.c:26](../xv6-riscv/kernel/sysproc.c#L26) | `kfork()` en `proc.c` |
| `exit(status)` | Termina el proceso actual (no retorna). | `sys_exit` [sysproc.c:11](../xv6-riscv/kernel/sysproc.c#L11) | `kexit()` en `proc.c` |
| `wait(*status)` | Espera a que termine un hijo; escribe su código de salida en `status`. | `sys_wait` [sysproc.c:32](../xv6-riscv/kernel/sysproc.c#L32) | `kwait()` en `proc.c` |
| `kill(pid)` | Marca al proceso `pid` para que muera. | `sys_kill` [sysproc.c:93](../xv6-riscv/kernel/sysproc.c#L93) | `kkill()` en `proc.c` |
| `getpid()` | Devuelve el pid del proceso actual. | `sys_getpid` [sysproc.c:20](../xv6-riscv/kernel/sysproc.c#L20) | — |
| `sbrk(n, tipo)` | Aumenta/reduce la memoria del proceso en `n` bytes; devuelve la dirección base anterior. | `sys_sbrk` [sysproc.c:40](../xv6-riscv/kernel/sysproc.c#L40) | `growproc()` / `vmfault()` (lazy) |
| `pause(n)` | Duerme el proceso `n` ticks de reloj (era `sleep` en xv6 original). | `sys_pause` [sysproc.c:68](../xv6-riscv/kernel/sysproc.c#L68) | `sleep()` en `proc.c` |
| `uptime()` | Devuelve cuántos ticks de reloj han pasado desde el arranque. | `sys_uptime` [sysproc.c:104](../xv6-riscv/kernel/sysproc.c#L104) | var global `ticks` |
| `getreadcount()` | (curso) Devuelve cuántas veces se ha llamado a `read()` en todo el sistema. | `sys_getreadcount` [sysproc.c:117](../xv6-riscv/kernel/sysproc.c#L117) | `readcount` (global en `sysfile.c:68`) |
| `trace(mask)` | (curso) Activa el rastreo: imprime cada syscall cuyo bit esté en `mask` al retornar. | `sys_trace` [sysproc.c:123](../xv6-riscv/kernel/sysproc.c#L123) | `p->tracemask`, impreso en `syscall.c:181` |

### Sistema de archivos / E-S — handlers en [kernel/sysfile.c](../xv6-riscv/kernel/sysfile.c)

| Syscall | Qué hace | Handler |
|---|---|---|
| `open(path, flags)` | Abre (o crea con `O_CREATE`) un archivo; devuelve un descriptor. | `sys_open` [sysfile.c:332](../xv6-riscv/kernel/sysfile.c#L332) |
| `read(fd, buf, n)` | Lee hasta `n` bytes de `fd` a `buf`; devuelve los bytes leídos (0 = EOF). | `sys_read` [sysfile.c:71](../xv6-riscv/kernel/sysfile.c#L71) |
| `write(fd, buf, n)` | Escribe `n` bytes de `buf` en `fd`; devuelve los bytes escritos. | `sys_write` [sysfile.c:86](../xv6-riscv/kernel/sysfile.c#L86) |
| `close(fd)` | Cierra el descriptor `fd`. | `sys_close` [sysfile.c:101](../xv6-riscv/kernel/sysfile.c#L101) |
| `dup(fd)` | Duplica `fd` en el descriptor libre más bajo; ambos comparten offset. | `sys_dup` [sysfile.c:55](../xv6-riscv/kernel/sysfile.c#L55) |
| `pipe(int fd[2])` | Crea un pipe: `fd[0]` extremo de lectura, `fd[1]` de escritura. | `sys_pipe` [sysfile.c:505](../xv6-riscv/kernel/sysfile.c#L505) |
| `fstat(fd, *st)` | Rellena `struct stat` con los metadatos del archivo de `fd`. | `sys_fstat` [sysfile.c:114](../xv6-riscv/kernel/sysfile.c#L114) |
| `link(old, new)` | Crea un enlace duro `new` que apunta al mismo inodo que `old`. | `sys_link` [sysfile.c:127](../xv6-riscv/kernel/sysfile.c#L127) |
| `unlink(path)` | Borra el nombre `path` (y el archivo si era el último enlace). | `sys_unlink` [sysfile.c:205](../xv6-riscv/kernel/sysfile.c#L205) |
| `mkdir(path)` | Crea un directorio. | `sys_mkdir` [sysfile.c:401](../xv6-riscv/kernel/sysfile.c#L401) |
| `mknod(path, major, minor)` | Crea un archivo de dispositivo (nodo especial). | `sys_mknod` [sysfile.c:417](../xv6-riscv/kernel/sysfile.c#L417) |
| `chdir(path)` | Cambia el directorio de trabajo del proceso. | `sys_chdir` [sysfile.c:437](../xv6-riscv/kernel/sysfile.c#L437) |
| `exec(path, argv[])` | Reemplaza la imagen del proceso actual por el programa en `path`. | `sys_exec` [sysfile.c:462](../xv6-riscv/kernel/sysfile.c#L462) |
| `sync()` | (curso) Fuerza el volcado del log del sistema de archivos a disco. | `sys_sync` [log.c:247](../xv6-riscv/kernel/log.c#L247) |

---

## Cómo crear tu propia system call

Ejemplo: añadir `int getprocs(void)` que devuelva el nº de procesos activos.

1. **Reservar el número** — [kernel/syscall.h](../xv6-riscv/kernel/syscall.h)
   ```c
   #define SYS_getprocs 25   // el siguiente libre
   ```

2. **Registrar el handler** — [kernel/syscall.c](../xv6-riscv/kernel/syscall.c)
   ```c
   extern uint64 sys_getprocs(void);           // junto a los demás extern
   ...
   [SYS_getprocs] = sys_getprocs,              // en la tabla syscalls[]
   [SYS_getprocs] = "getprocs",                // en syscall_names[] (para trace)
   ```

3. **Implementar el handler** — normalmente en [kernel/sysproc.c](../xv6-riscv/kernel/sysproc.c)
   ```c
   uint64
   sys_getprocs(void)
   {
     // lee argumentos si los hay:
     // int n; argint(0, &n);
     return count_procs();   // función real, la escribes en proc.c
   }
   ```
   Si tocas datos de otro archivo (p. ej. `proc.c`), añade el prototipo en
   [kernel/defs.h](../xv6-riscv/kernel/defs.h).

4. **Exponerla a espacio de usuario**
   - [user/user.h](../xv6-riscv/user/user.h): `int getprocs(void);`
   - [user/usys.pl](../xv6-riscv/user/usys.pl): `entry("getprocs");`
     (esto genera el stub en `usys.S` al compilar)

5. **(Opcional) Programa de prueba** — crea `user/getprocs.c` con un `main()`
   y añádelo a `UPROGS` en el [Makefile](../xv6-riscv/Makefile).

6. **Compilar y probar**
   ```sh
   make qemu
   # dentro de xv6:
   $ getprocs
   ```

### Errores típicos
- Olvidar la línea en `usys.pl` → error de *linker* (`undefined reference to getprocs`).
- Número de syscall repetido o fuera de rango → `unknown sys call`.
- Leer argumentos con el índice equivocado en `argint/argaddr` (empiezan en 0).
- No hacer `make clean` si el Makefile no regenera `usys.S` (normalmente sí lo hace).
