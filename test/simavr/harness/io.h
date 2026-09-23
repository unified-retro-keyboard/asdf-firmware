#ifndef ASDF_SIM_IO_H
#define ASDF_SIM_IO_H

#include <stdint.h>
#include <simavr/sim_avr.h>
#include "asdf_simavr_io_select.h"

/* Wire up the harness's strobe-edge capture to the given CPU/map.
 * After this, every strobe transition appends a record via cap_push().
 * Both edges are captured (not just the active edge) because keymaps
 * can flip strobe polarity at runtime via asdf_arch_set_neg_strobe. */
void io_wire_output(avr_t *cpu, const asdf_io_map_t *io);

/* Wire up matrix input injection. After this, the firmware's row strobes
 * will trigger the harness to drive the column input(s) according to the
 * currently-set matrix state. */
void io_wire_input(avr_t *cpu, const asdf_io_map_t *io);

/* Set/clear a key in the simulated matrix.
 * row 0..ASDF_NUM_ROWS-1 = normal keys; row 8 = DIP switch slot. */
void matrix_press(int row, int col);
void matrix_release(int row, int col);
void matrix_clear(void);

/* Convenience: set all 8 DIP switch bits to the given value, replacing
 * any previous DIP state. */
void set_dip(uint8_t value);

/* ── pin transition watching ─────────────────────────────────────────────
 * Count level changes on an arbitrary port bit.  Used to prove which pin an
 * output action actually drives, independent of the strobe/data capture path.
 * Two independent watch slots are provided: enough to compare an intended
 * pin against the one a mis-wired action would hit instead. */
#define IO_WATCH_SLOTS 2

/* Attach slot (0..IO_WATCH_SLOTS-1) to port/bit and prime it from the pin's
 * current level. Safe to call once per slot. Returns 0 on success, -1 for an
 * invalid slot/pin or a pin that simavr cannot expose. */
int io_watch_pin(avr_t *cpu, int slot, char port, int bit);

/* Zero all transition counts, keeping each slot's last-known level, so
 * transitions are measured relative to the level at the time of the reset.
 * Call after boot to discard power-on pin settling. */
void io_watch_reset(void);

/* Transitions observed on a slot since the last io_watch_reset(). */
unsigned io_watch_count(int slot);

/* Cycle of the first (edge 0) or second (edge 1) transition on a slot since
 * the last io_watch_reset(); 0 if it has not happened. */
uint64_t io_watch_edge_cycle(int slot, int edge);

#endif
