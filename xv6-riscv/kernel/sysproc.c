#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

extern uint readcount;

uint64
sys_getreadcount(void)
{
  return readcount;
}

uint64
sys_trace(void)
{
  int mask;
  argint(0, &mask);
  myproc()->tracemask = mask;
  return 0;
}

// Devuelve al espacio de usuario el numero de bytes de memoria fisica libre.
uint64
sys_getfreemem(void)
{
  return count_free_bytes();
}

// int pgaccess(void *base, int len, void *mask)
// Inspecciona 'len' paginas desde 'base'. Por cada pagina con PTE_A activo,
// marca el bit i de una mascara de 32 bits y limpia PTE_A. El resultado se
// copia a la direccion de usuario 'mask' con copyout().
uint64 sys_pgaccess(void)
{
  uint64 base;
  int len;
  uint64 maskaddr;

  argaddr(0, &base);
  argint(1, &len);
  argaddr(2, &maskaddr);

  if (len < 0 || len > 32)
    return -1;

  struct proc *p = myproc();
  uint64 mask = 0;

  for (int i = 0; i < len; i++) {
    uint64 va = base + (uint64)i * PGSIZE;
    pte_t *pte = walk(p->pagetable, va, 0);
    if (pte == 0 || (*pte & PTE_V) == 0)
      return -1;
    if (*pte & PTE_A) {
      mask |= (1L << i);
      *pte &= ~PTE_A; // limpiar para poder detectar accesos futuros
    }
  }

  uint32 umask = (uint32)mask;
  if (copyout(p->pagetable, p->sz, maskaddr, (char *)&umask, sizeof(umask)) < 0)
    return -1;

  return 0;
}
