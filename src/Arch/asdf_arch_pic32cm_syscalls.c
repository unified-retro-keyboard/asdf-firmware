// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
//  Unified Keyboard Project
//  ASDF keyboard firmware
//
//  asdf_arch_pic32cm_syscalls.c
//
// Minimal newlib syscall stubs for the bare-metal PIC32CM PL10 targets. The
// keyboard firmware performs no host I/O, so these satisfy newlib's link-time
// references with do-nothing implementations. Providing them lets the link drop
// --specs=nosys.specs, whose stubs otherwise emit "_X is not implemented"
// linker warnings. _sbrk grows the heap from the linker-provided `end` symbol.

#include <stddef.h>
#include <sys/stat.h>
#include <errno.h>

extern char end; // first address past .bss / start of heap (linker script)
static char *heap_end = &end;

void *_sbrk(ptrdiff_t incr)
{
  char *prev = heap_end;
  heap_end += incr;
  return prev;
}

int _close(int file)
{
  (void) file;
  return -1;
}

int _fstat(int file, struct stat *st)
{
  (void) file;
  st->st_mode = S_IFCHR;
  return 0;
}

int _isatty(int file)
{
  (void) file;
  return 1;
}

int _lseek(int file, int ptr, int dir)
{
  (void) file;
  (void) ptr;
  (void) dir;
  return 0;
}

int _read(int file, char *ptr, int len)
{
  (void) file;
  (void) ptr;
  (void) len;
  return 0;
}

int _write(int file, char *ptr, int len)
{
  (void) file;
  (void) ptr;
  return len;
}

void _exit(int status)
{
  (void) status;
  for (;;) {
  }
}

int _kill(int pid, int sig)
{
  (void) pid;
  (void) sig;
  errno = EINVAL;
  return -1;
}

int _getpid(void) { return 1; }
