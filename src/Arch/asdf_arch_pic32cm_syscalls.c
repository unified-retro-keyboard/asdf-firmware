// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_arch_pic32cm_syscalls.c
 *
 * Minimal newlib syscall stubs for the bare-metal PIC32CM PL10 targets. The
 * keyboard firmware performs no host I/O, so these satisfy newlib's link-time
 * references with do-nothing implementations. Providing them lets the link drop
 * --specs=nosys.specs, whose stubs otherwise emit "_X is not implemented"
 * linker warnings. _sbrk grows the heap from the linker-provided `end` symbol.
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2026 David Fenyes. MIT License; see LICENSE.
 */
// SPDX-License-Identifier: MIT

#include <stddef.h>
#include <sys/stat.h>
#include <errno.h>

//lint -e970 -e818 D12: newlib's system-call signatures, for the rest of this module

// newlib calls these by name; it declares none of them in its headers.
void *_sbrk(ptrdiff_t incr);
int _close(int file);
int _fstat(int file, struct stat *st);
int _isatty(int file);
int _lseek(int file, int ptr, int dir);
int _read(int file, char *ptr, int len);
int _write(int file, char *ptr, int len);
void _exit(int status) __attribute__((noreturn));
int _kill(int pid, int sig);
int _getpid(void);

//lint -esym(526, end) -esym(2701, end) defined by the linker script
extern char end; // first address past .bss / start of heap (linker script)

/**
 * Grows the heap.
 *
 * Moves the heap end by incr bytes.
 *
 * @param incr  Number of bytes to add to the heap (negative to shrink it).
 * @return The previous heap end: the start of the newly added space.
 *
 * The heap starts at the linker-provided `end` symbol and grows upward. There
 * is no check against the stack or the end of RAM; the firmware does not use
 * the heap.
 */
void *_sbrk(ptrdiff_t incr)
{
  static char *heap_end = &end;
  char *prev = heap_end;

  heap_end = &heap_end[incr];
  return prev;
}

/**
 * Closes a file; no files exist, so it always fails.
 *
 * No side effects.
 *
 * @param file  File descriptor; ignored.
 * @return -1.
 */
int _close(int file)
{
  (void) file;
  return -1;
}

/**
 * Reports every file as a character device.
 *
 * Sets st->st_mode to S_IFCHR, which makes newlib treat the stream as
 * unbuffered.
 *
 * @param file  File descriptor; ignored.
 * @param st    Status to fill in.
 * @return 0.
 */
int _fstat(int file, struct stat *st)
{
  (void) file;
  st->st_mode = S_IFCHR;
  return 0;
}

/**
 * Reports every file as a terminal.
 *
 * No side effects.
 *
 * @param file  File descriptor; ignored.
 * @return 1.
 */
int _isatty(int file)
{
  (void) file;
  return 1;
}

/**
 * Seeks within a file; there is nothing to seek, so it does nothing.
 *
 * No side effects.
 *
 * @param file  File descriptor; ignored.
 * @param ptr   Offset; ignored.
 * @param dir   Seek origin; ignored.
 * @return 0.
 */
int _lseek(int file, int ptr, int dir)
{
  (void) file;
  (void) ptr;
  (void) dir;
  return 0;
}

/**
 * Reads from a file; there is no input, so it always reads nothing.
 *
 * No side effects.
 *
 * @param file  File descriptor; ignored.
 * @param ptr   Destination buffer; ignored.
 * @param len   Buffer length; ignored.
 * @return 0 (end of file).
 */
int _read(int file, char *ptr, int len)
{
  (void) file;
  (void) ptr;
  (void) len;
  return 0;
}

/**
 * Writes to a file; the data is discarded.
 *
 * No side effects.
 *
 * @param file  File descriptor; ignored.
 * @param ptr   Data to write; ignored.
 * @param len   Number of bytes to write.
 * @return len, as if every byte were written.
 */
int _write(int file, char *ptr, int len)
{
  (void) file;
  (void) ptr;
  return len;
}

/**
 * Ends the program by halting in a loop.
 *
 * Never returns.
 *
 * @param status  Exit status; ignored.
 *
 * Complexity: 2
 */
void _exit(int status)
{
  (void) status;
  for (;;) {
  }
}

/**
 * Sends a signal; there are no processes to signal, so it always fails.
 *
 * Sets errno to EINVAL.
 *
 * @param pid  Process ID; ignored.
 * @param sig  Signal number; ignored.
 * @return -1.
 */
int _kill(int pid, int sig)
{
  (void) pid;
  (void) sig;
  errno = EINVAL;
  return -1;
}

/**
 * Returns the ID of the only process.
 *
 * No side effects.
 *
 * @return 1.
 */
int _getpid(void) { return 1; }
