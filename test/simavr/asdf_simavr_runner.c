#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <simavr/sim_avr.h>

#include "asdf_simavr_io_select.h"
#include "sim.h"
#include "io.h"
#include "run.h"
#include "capture.h"
#include "sim_assert.h"
#include "vcd.h"
#include "test_types.h"
#include "keymap_data/asdf_simavr_test_classic.h"
#include "keymap_data/asdf_simavr_test_classic_caps.h"
#include "keymap_data/asdf_simavr_test_apple2.h"
#include "keymap_data/asdf_simavr_test_apple2_caps.h"
#include "keymap_data/asdf_simavr_test_sol.h"
#include "keymap_data/asdf_simavr_test_ace1000.h"

/* Harness timing for the functional modes (events, identity, string).
 *
 * These modes check which bytes a key produces, not how fast. Their waits are
 * deliberately generous because registration time under simulation varies
 * well beyond the 10 ms debounce: simavr's atmega1280 model in particular
 * delays some keypresses to 26 ms, while the same firmware image run on the
 * atmega2560 model registers them in 10-11 ms. Latency is measured only by
 * latency mode.
 *
 *   SIM_KEY_WAIT_MS   longest wait for a pressed key's byte
 *   SIM_SETTLE_MS     wait after pressing or releasing a modifier or trigger
 *                     key, for it to take effect */
#define SIM_KEY_WAIT_MS 50
#define SIM_SETTLE_MS 50

/* Latency mode: presses sampled across one scan tick; the longest accepted
 * press-to-output time (the worst case measured with elapsed-time scanning is
 * 13.2 ms, sol on atmega328p, against 10 ms of debounce); and how long to wait
 * for any output before the sample fails. */
#define SIM_LATENCY_SAMPLES 20
#define SIM_LATENCY_MAX_MS 15
#define SIM_LATENCY_LIMIT_MS 50

/* Accepted output strobe width (nominally 10 us, ASDF_STROBE_LENGTH_US) and
 * long pulse width (nominally 50 ms, ASDF_PULSE_DELAY_LONG_MS). The long pulse
 * is counted in ticks from the scan that starts it, so it can end up to one
 * scan period early (about 1.2 ms on the 8 MHz atmega328p) or one tick late. */
#define SIM_STROBE_MIN_US 10
#define SIM_STROBE_MAX_US 20
#define SIM_LONG_PULSE_MIN_MS 47
#define SIM_LONG_PULSE_MAX_MS 52

static int pred_capture_nonempty(void *ctx) { (void)ctx; return cap_count() > 0; }
static int pred_capture_even(void *ctx) { (void)ctx; return !(cap_count() & 1u); }

static void usage(const char *argv0)
{
    fprintf(stderr,
        "usage: %s --target <chip> --keymap <name> --elf <path>\n"
        "  --target    atmega328p | atmega168p | atmega1280 | atmega2560\n"
        "              (atmega640 is built but unsupported by simavr 1.6)\n"
        "  --keymap    classic | classic_caps | apple2 | apple2_caps | sol | ace1000\n"
        "  --elf       path to the .elf produced by build-<chip>/\n"
        "  --boot-only boot and run a brief idle period; skip keypress events\n"
        "  --verbose   log every captured output byte\n"
        "  --vcd PATH  dump VCD of watched pins to PATH\n"
        "  --gdb PORT  start simavr gdb stub on PORT and wait for attach\n"
        "  --mode M    events (default) | identity | string | out2 | repeat\n"
        "              | latency\n",
        argv0);
    exit(2);
}

typedef struct {
    const char *target;
    const char *keymap;
    const char *elf_path;
    const char *vcd_path;
    const char *mode;
    int gdb_port;
    int verbose;
    int boot_only;
} args_t;

static args_t parse_args(int argc, char **argv)
{
    args_t a = {0};
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--target") && i + 1 < argc) a.target = argv[++i];
        else if (!strcmp(argv[i], "--keymap") && i + 1 < argc) a.keymap = argv[++i];
        else if (!strcmp(argv[i], "--elf") && i + 1 < argc) a.elf_path = argv[++i];
        else if (!strcmp(argv[i], "--vcd") && i + 1 < argc) a.vcd_path = argv[++i];
        else if (!strcmp(argv[i], "--gdb") && i + 1 < argc) a.gdb_port = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--mode") && i + 1 < argc) a.mode = argv[++i];
        else if (!strcmp(argv[i], "--verbose")) a.verbose = 1;
        else if (!strcmp(argv[i], "--boot-only")) a.boot_only = 1;
        else { fprintf(stderr, "unknown arg: %s\n", argv[i]); usage(argv[0]); }
    }
    if (!a.target || !a.keymap || !a.elf_path) usage(argv[0]);
    if (!a.mode) a.mode = "events";
    if (strcmp(a.mode, "events") && strcmp(a.mode, "identity") &&
        strcmp(a.mode, "string") && strcmp(a.mode, "out2") &&
        strcmp(a.mode, "repeat") && strcmp(a.mode, "latency")) {
        fprintf(stderr, "unknown mode: %s\n", a.mode);
        usage(argv[0]);
    }
    return a;
}

static const sim_keymap_test_t *pick_keymap(const char *name)
{
    if (!strcmp(name, "classic"))      return &classic_test;
    if (!strcmp(name, "classic_caps")) return &classic_caps_test;
    if (!strcmp(name, "apple2"))       return &apple2_test;
    if (!strcmp(name, "apple2_caps"))  return &apple2_caps_test;
    if (!strcmp(name, "sol"))          return &sol_test;
    if (!strcmp(name, "ace1000"))      return &ace1000_test;
    return 0;
}

static const sim_identity_test_t *pick_identity(const char *name)
{
    if (!strcmp(name, "classic"))      return &classic_identity_test;
    if (!strcmp(name, "classic_caps")) return &classic_caps_identity_test;
    if (!strcmp(name, "apple2"))       return &apple2_identity_test;
    if (!strcmp(name, "apple2_caps"))  return &apple2_caps_identity_test;
    if (!strcmp(name, "sol"))          return &sol_identity_test;
    if (!strcmp(name, "ace1000"))      return &ace1000_identity_test;
    return 0;
}

static sim_coord_t identity_mod_coord(const sim_identity_test_t *id, int mod)
{
    switch (mod) {
        case SIM_MOD_SHIFT: return id->modifier_shift;
        case SIM_MOD_CAPS:  return id->modifier_caps_toggle;
        case SIM_MOD_CTRL:  return id->modifier_ctrl;
        default: { sim_coord_t z = { -1, -1 }; return z; }
    }
}

static const sim_string_test_t *pick_string_test(const char *name)
{
    if (!strcmp(name, "classic"))      return &classic_string_test;
    if (!strcmp(name, "classic_caps")) return &classic_caps_string_test;
    if (!strcmp(name, "apple2"))       return &apple2_string_test;
    if (!strcmp(name, "apple2_caps"))  return &apple2_caps_string_test;
    if (!strcmp(name, "sol"))          return &sol_string_test;
    if (!strcmp(name, "ace1000"))      return &ace1000_string_test;
    return 0;
}

/* Only the sol keymap routes a virtual output to PHYSICAL_OUT2, so it is the
 * only keymap with out2 regression data. */
static const sim_out2_test_t *pick_out2_test(const char *name)
{
    if (!strcmp(name, "sol")) return &sol_out2_test;
    return 0;
}

static const sim_repeat_test_t *pick_repeat_test(const char *name)
{
    if (!strcmp(name, "sol")) return &sol_repeat_test;
    return 0;
}

static sim_coord_t string_mod_coord(const sim_string_test_t *st, int mod)
{
    switch (mod) {
        case SIM_MOD_SHIFT: return st->modifier_shift;
        case SIM_MOD_CAPS:  return st->modifier_caps_toggle;
        case SIM_MOD_CTRL:  return st->modifier_ctrl;
        default: { sim_coord_t z = { -1, -1 }; return z; }
    }
}

int main(int argc, char **argv)
{
    args_t a = parse_args(argc, argv);

    const asdf_io_map_t *io = asdf_io_pick(a.target);
    if (!io) { fprintf(stderr, "FAIL: no I/O map for %s\n", a.target); return 1; }

    avr_t *cpu = sim_load(a.target, a.elf_path, io);
    if (!cpu) return 1;

    cap_init();
    io_wire_output(cpu, io);
    io_wire_input(cpu, io);

    if (a.vcd_path) {
        if (vcd_begin(cpu, io, a.vcd_path) != 0)
            fprintf(stderr, "WARN: VCD recording disabled\n");
    }

    if (!strcmp(a.mode, "events")) {
        const sim_keymap_test_t *km = pick_keymap(a.keymap);
        if (!km) { fprintf(stderr, "FAIL: no test data for keymap %s\n", a.keymap); return 1; }

        set_dip(km->dip_value);

        /* Let the firmware boot and run a few scan cycles to settle. */
        if (sim_wait_ms(cpu, km->boot_scan_ticks, io->cpu_frequency_hz) < 0) {
            fprintf(stderr, "FAIL: cpu halted during boot\n");
            return 1;
        }

        /* Boot-only mode: confirm the CPU survived boot, then exit.
         * Run an extra 200 ms of idle time to confirm the scan loop is alive. */
        if (a.boot_only) {
            if (sim_wait_ms(cpu, 200, io->cpu_frequency_hz) < 0) {
                fprintf(stderr, "FAIL: %s/%s cpu halted after boot\n", a.target, a.keymap);
                vcd_end();
                return 1;
            }
            vcd_end();
            printf("OK: %s/%s boot-only smoke passed at cycle %" PRIu64 "\n",
                   a.target, a.keymap, (uint64_t)cpu->cycle);
            return 0;
        }

        /* Drain anything emitted during boot. */
        cap_clear();

        for (int i = 0; i < km->num_events; i++) {
            const sim_event_t *e = &km->events[i];
            char what[64];
            snprintf(what, sizeof what, "%s/event[%d]@(%d,%d)", a.keymap, i, e->row, e->col);

            if (e->with_modifier == SIM_MOD_SHIFT) {
                matrix_press(km->modifier_shift.row, km->modifier_shift.col);
                /* SHIFT must be stable before the key lookup runs. */
                sim_wait_ms(cpu, SIM_SETTLE_MS, io->cpu_frequency_hz);
                cap_clear();
            } else if (e->with_modifier == SIM_MOD_CTRL) {
                matrix_press(km->modifier_ctrl.row, km->modifier_ctrl.col);
                sim_wait_ms(cpu, SIM_SETTLE_MS, io->cpu_frequency_hz);
                cap_clear();
            }

            uint64_t key_wait = (io->cpu_frequency_hz / 1000) * SIM_KEY_WAIT_MS;
            if (e->hold_cycles > key_wait) key_wait = e->hold_cycles;

            matrix_press(e->row, e->col);
            if (sim_expect_byte_within(cpu, e->expected, key_wait, what) != 0) {
                vcd_end();
                return 1;
            }
            matrix_release(e->row, e->col);

            if (e->with_modifier == SIM_MOD_SHIFT)
                matrix_release(km->modifier_shift.row, km->modifier_shift.col);
            else if (e->with_modifier == SIM_MOD_CTRL)
                matrix_release(km->modifier_ctrl.row, km->modifier_ctrl.col);

            /* Wait for debounce + repeat-delay guard before the next event. */
            sim_wait_ms(cpu, 50, io->cpu_frequency_hz);
            cap_clear();
        }

        vcd_end();
        printf("OK: %s/%s passed %d events at cycle %" PRIu64 "\n",
               a.target, a.keymap, km->num_events, (uint64_t)cpu->cycle);
        return 0;
    }

    if (!strcmp(a.mode, "identity")) {
        const sim_identity_test_t *id = pick_identity(a.keymap);
        if (!id) {
            fprintf(stderr, "FAIL: no identity test data for keymap %s\n", a.keymap);
            return 1;
        }

        set_dip(id->dip_value);

        if (sim_wait_ms(cpu, id->boot_scan_ticks, io->cpu_frequency_hz) < 0) {
            fprintf(stderr, "FAIL: cpu halted during identity boot wait\n");
            vcd_end();
            return 1;
        }

        if (id->expected_len <= 0) {
            fprintf(stderr, "FAIL: %s/%s identity test has no expected bytes (expected_len=%d)\n",
                    a.target, a.keymap, id->expected_len);
            vcd_end();
            return 1;
        }

        /* Drain any residual boot output before triggering the ID hook. */
        cap_clear();

        /* Press modifier (if any), then press the trigger key, release both,
         * then wait for the ID-message print train to complete. */
        if (id->trigger_modifier != SIM_MOD_NONE) {
            sim_coord_t mc = identity_mod_coord(id, id->trigger_modifier);
            matrix_press(mc.row, mc.col);
            sim_wait_ms(cpu, SIM_SETTLE_MS, io->cpu_frequency_hz);
            cap_clear();
        }
        matrix_press(id->trigger_key.row, id->trigger_key.col);
        sim_wait_ms(cpu, SIM_SETTLE_MS, io->cpu_frequency_hz);
        matrix_release(id->trigger_key.row, id->trigger_key.col);
        sim_wait_ms(cpu, SIM_SETTLE_MS, io->cpu_frequency_hz);
        if (id->trigger_modifier != SIM_MOD_NONE) {
            sim_coord_t mc = identity_mod_coord(id, id->trigger_modifier);
            matrix_release(mc.row, mc.col);
            sim_wait_ms(cpu, SIM_SETTLE_MS, io->cpu_frequency_hz);
        }
        sim_wait_ms(cpu, id->capture_ticks, io->cpu_frequency_hz);

        /* Each output byte is captured on both strobe edges (rising and
         * falling), matching the behaviour in events-mode where cap_clear()
         * discards the second-edge duplicate after each event.  So the raw
         * count is 2× the number of unique bytes emitted. */
        size_t captured = cap_count();
        size_t unique = captured / 2;
        if ((int)unique != id->expected_len) {
            fprintf(stderr, "FAIL: %s/%s identity length mismatch: captured %zu (%zu unique), expected %d\n",
                    a.target, a.keymap, captured, unique, id->expected_len);
            for (size_t i = 0; i < captured; i++) {
                asdf_cap_record_t r;
                cap_pop(&r);
                fprintf(stderr, "  [%zu] cycle=%" PRIu64 " byte=0x%02x %c\n",
                        i, r.cycle, r.byte,
                        (r.byte >= 0x20 && r.byte < 0x7f) ? (char)r.byte : '.');
            }
            vcd_end();
            return 1;
        }

        for (int i = 0; i < id->expected_len; i++) {
            asdf_cap_record_t r;
            cap_pop(&r);          /* first edge: the actual data */
            {
                asdf_cap_record_t dummy;
                cap_pop(&dummy);  /* second edge: discard duplicate */
            }
            if (r.byte != (uint8_t)id->expected[i]) {
                fprintf(stderr, "FAIL: %s/%s identity byte[%d] = 0x%02x, expected 0x%02x\n",
                        a.target, a.keymap, i, r.byte, (uint8_t)id->expected[i]);
                /* Drain and dump remaining bytes for diagnostic context. */
                for (int j = i + 1; j < id->expected_len; j++) {
                    asdf_cap_record_t rr, dd;
                    if (!cap_pop(&rr)) break;
                    if (!cap_pop(&dd)) break;  /* ring parity violation - abort dump */
                    fprintf(stderr, "  [%d] cycle=%" PRIu64 " byte=0x%02x %c (expected 0x%02x)\n",
                            j, rr.cycle, rr.byte,
                            (rr.byte >= 0x20 && rr.byte < 0x7f) ? (char)rr.byte : '.',
                            (uint8_t)id->expected[j]);
                }
                vcd_end();
                return 1;
            }
        }

        vcd_end();
        printf("OK: %s/%s identity %d bytes at cycle %" PRIu64 "\n",
               a.target, a.keymap, id->expected_len, (uint64_t)cpu->cycle);
        return 0;
    }

    if (!strcmp(a.mode, "string")) {
        const sim_string_test_t *st = pick_string_test(a.keymap);
        if (!st) {
            fprintf(stderr, "FAIL: no string test data for keymap %s\n", a.keymap);
            return 1;
        }

        set_dip(st->dip_value);

        if (sim_wait_ms(cpu, st->boot_scan_ticks, io->cpu_frequency_hz) < 0) {
            fprintf(stderr, "FAIL: cpu halted during string boot wait\n");
            vcd_end();
            return 1;
        }
        cap_clear();   /* drain boot output */

        for (int i = 0; i < st->num_steps; i++) {
            const sim_string_step_t *step = &st->steps[i];
            char what[64];

            switch (step->type) {
                case SIM_STEP_KEY: {
                    snprintf(what, sizeof what, "%s/step[%d]@(%d,%d)",
                             a.keymap, i, step->row, step->col);
                    matrix_press(step->row, step->col);
                    if (sim_expect_byte_within(cpu, step->expected,
                                               (io->cpu_frequency_hz / 1000) * SIM_KEY_WAIT_MS,
                                               what) != 0) {
                        vcd_end();
                        return 1;
                    }
                    matrix_release(step->row, step->col);
                    sim_wait_ms(cpu, 50, io->cpu_frequency_hz);
                    cap_clear();
                    break;
                }
                case SIM_STEP_MOD_DOWN: {
                    sim_coord_t c = string_mod_coord(st, step->modifier);
                    matrix_press(c.row, c.col);
                    sim_wait_ms(cpu, SIM_SETTLE_MS, io->cpu_frequency_hz);
                    cap_clear();
                    break;
                }
                case SIM_STEP_MOD_UP: {
                    sim_coord_t c = string_mod_coord(st, step->modifier);
                    matrix_release(c.row, c.col);
                    sim_wait_ms(cpu, SIM_SETTLE_MS, io->cpu_frequency_hz);
                    cap_clear();
                    break;
                }
                case SIM_STEP_MOD_TAP: {
                    sim_coord_t c = string_mod_coord(st, step->modifier);
                    matrix_press(c.row, c.col);
                    sim_wait_ms(cpu, SIM_SETTLE_MS, io->cpu_frequency_hz);
                    matrix_release(c.row, c.col);
                    sim_wait_ms(cpu, SIM_SETTLE_MS, io->cpu_frequency_hz);
                    cap_clear();
                    break;
                }
            }
        }

        vcd_end();
        printf("OK: %s/%s passed %d string steps at cycle %" PRIu64 "\n",
               a.target, a.keymap, st->num_steps, (uint64_t)cpu->cycle);
        return 0;
    }

    if (!strcmp(a.mode, "out2")) {
        const sim_out2_test_t *o2 = pick_out2_test(a.keymap);
        if (!o2) {
            fprintf(stderr, "FAIL: no out2 test data for keymap %s\n", a.keymap);
            vcd_end();
            return 1;
        }
        if (!io->out2_port || !io->led2_port) {
            fprintf(stderr, "FAIL: %s has no OUT2/LED2 pin map\n", a.target);
            vcd_end();
            return 1;
        }

        /* Slot 0 = the pin OUT2 must drive; slot 1 = the pin the historical
         * 328P defect drove instead. */
        if (io_watch_pin(cpu, 0, io->out2_port, io->out2_bit) != 0 ||
            io_watch_pin(cpu, 1, io->led2_port, io->led2_bit) != 0) {
            fprintf(stderr, "FAIL: could not watch %s OUT2/LED2 pins\n", a.target);
            vcd_end();
            return 1;
        }

        set_dip(o2->dip_value);

        if (sim_wait_ms(cpu, o2->boot_scan_ticks, io->cpu_frequency_hz) < 0) {
            fprintf(stderr, "FAIL: cpu halted during out2 boot wait\n");
            vcd_end();
            return 1;
        }

        /* Discard boot-time pin settling: LED2 legitimately moves during
         * init, and only transitions caused by the keypress are of interest. */
        io_watch_reset();

        matrix_press(o2->trigger_key.row, o2->trigger_key.col);
        if (sim_wait_ms(cpu, o2->hold_ms, io->cpu_frequency_hz) < 0) {
            fprintf(stderr, "FAIL: cpu halted while holding out2 trigger\n");
            vcd_end();
            return 1;
        }
        matrix_release(o2->trigger_key.row, o2->trigger_key.col);
        if (sim_wait_ms(cpu, o2->settle_ms, io->cpu_frequency_hz) < 0) {
            fprintf(stderr, "FAIL: cpu halted while settling out2 pulse\n");
            vcd_end();
            return 1;
        }

        unsigned out2_edges = io_watch_count(0);
        unsigned led2_edges = io_watch_count(1);
        int failed = 0;

        if (out2_edges != 2) {
            fprintf(stderr,
                    "FAIL: %s/%s OUT2 (P%c%d) changed %u time(s), expected exactly "
                    "2 edges after pressing (%d,%d)\n",
                    a.target, a.keymap, io->out2_port, io->out2_bit,
                    out2_edges,
                    o2->trigger_key.row, o2->trigger_key.col);
            failed = 1;
        }
        if (out2_edges == 2) {
            double width_ms = (double)(io_watch_edge_cycle(0, 1) - io_watch_edge_cycle(0, 0))
                              / (io->cpu_frequency_hz / 1000);
            if (width_ms < SIM_LONG_PULSE_MIN_MS || width_ms > SIM_LONG_PULSE_MAX_MS) {
                fprintf(stderr,
                        "FAIL: %s/%s OUT2 pulse width %.2f ms, expected %d-%d ms\n",
                        a.target, a.keymap, width_ms, SIM_LONG_PULSE_MIN_MS,
                        SIM_LONG_PULSE_MAX_MS);
                failed = 1;
            }
        }
        if (led2_edges != 0) {
            fprintf(stderr,
                    "FAIL: %s/%s LED2 (P%c%d) changed %u time(s) on an OUT2 action; "
                    "the output is driving the wrong pin\n",
                    a.target, a.keymap, io->led2_port, io->led2_bit, led2_edges);
            failed = 1;
        }
        if (failed) {
            vcd_end();
            return 1;
        }

        vcd_end();
        printf("OK: %s/%s OUT2 (P%c%d) toggled %u time(s), LED2 (P%c%d) untouched, "
               "at cycle %" PRIu64 "\n",
               a.target, a.keymap, io->out2_port, io->out2_bit, out2_edges,
               io->led2_port, io->led2_bit, (uint64_t)cpu->cycle);
        return 0;
    }

    if (!strcmp(a.mode, "latency")) {
        /* Key-registration latency: time from a key press to the first strobe
         * edge of its code. Each sample shifts the press by a fraction of the
         * 1 ms scan tick, so the samples cover the full tick phase. */
        const sim_keymap_test_t *km = pick_keymap(a.keymap);
        const sim_event_t *e = 0;
        if (km) {
            for (int i = 0; i < km->num_events && !e; i++)
                if (km->events[i].with_modifier == SIM_MOD_NONE) e = &km->events[i];
        }
        if (!e) {
            fprintf(stderr, "FAIL: no unmodified event for keymap %s\n", a.keymap);
            vcd_end();
            return 1;
        }

        set_dip(km->dip_value);
        if (sim_wait_ms(cpu, km->boot_scan_ticks, io->cpu_frequency_hz) < 0) {
            fprintf(stderr, "FAIL: cpu halted during latency boot wait\n");
            vcd_end();
            return 1;
        }

        const uint64_t cycles_per_ms = io->cpu_frequency_hz / 1000;
        uint64_t min_cycles = UINT64_MAX, max_cycles = 0;
        double min_strobe_us = 1e9, max_strobe_us = 0;

        for (int i = 0; i < SIM_LATENCY_SAMPLES; i++) {
            cap_clear();
            if (sim_run_for(cpu, (cycles_per_ms * i) / SIM_LATENCY_SAMPLES) < 0) {
                fprintf(stderr, "FAIL: cpu halted before latency sample %d\n", i);
                vcd_end();
                return 1;
            }

            uint64_t pressed_at = cpu->cycle;
            matrix_press(e->row, e->col);
            if (sim_run_until(cpu, pred_capture_nonempty, 0,
                              cycles_per_ms * SIM_LATENCY_LIMIT_MS) != 1) {
                fprintf(stderr,
                        "FAIL: %s/%s latency sample %d: no output within %d ms\n",
                        a.target, a.keymap, i, SIM_LATENCY_LIMIT_MS);
                vcd_end();
                return 1;
            }
            asdf_cap_record_t r;
            cap_pop(&r);
            if (r.byte != e->expected) {
                fprintf(stderr, "FAIL: %s/%s latency sample %d: got 0x%02x, expected 0x%02x\n",
                        a.target, a.keymap, i, r.byte, e->expected);
                vcd_end();
                return 1;
            }

            /* strobe width: from this edge to the next */
            if (sim_run_until(cpu, pred_capture_nonempty, 0, cycles_per_ms) == 1) {
                asdf_cap_record_t r2;
                cap_pop(&r2);
                double width_us = (double)(r2.cycle - r.cycle) * 1000.0 / cycles_per_ms;
                if (width_us < SIM_STROBE_MIN_US || width_us > SIM_STROBE_MAX_US) {
                    fprintf(stderr, "FAIL: %s/%s strobe width %.1f us, expected %d-%d us\n",
                            a.target, a.keymap, width_us, SIM_STROBE_MIN_US,
                            SIM_STROBE_MAX_US);
                    vcd_end();
                    return 1;
                }
                if (width_us < min_strobe_us) min_strobe_us = width_us;
                if (width_us > max_strobe_us) max_strobe_us = width_us;
            }

            uint64_t latency = r.cycle - pressed_at;
            if (latency < min_cycles) min_cycles = latency;
            if (latency > max_cycles) max_cycles = latency;

            matrix_release(e->row, e->col);
            sim_wait_ms(cpu, 50, io->cpu_frequency_hz);
        }

        vcd_end();
        if (max_cycles > cycles_per_ms * SIM_LATENCY_MAX_MS) {
            fprintf(stderr, "FAIL: %s/%s key latency max %.2f ms exceeds %d ms\n",
                    a.target, a.keymap, (double)max_cycles / cycles_per_ms,
                    SIM_LATENCY_MAX_MS);
            return 1;
        }
        printf("OK: %s/%s key latency min %.2f ms, max %.2f ms over %d presses; "
               "strobe %.1f-%.1f us\n",
               a.target, a.keymap,
               (double)min_cycles / cycles_per_ms, (double)max_cycles / cycles_per_ms,
               SIM_LATENCY_SAMPLES, min_strobe_us, max_strobe_us);
        return 0;
    }

    if (!strcmp(a.mode, "repeat")) {
        const sim_repeat_test_t *rp = pick_repeat_test(a.keymap);
        if (!rp) {
            fprintf(stderr, "FAIL: no repeat test data for keymap %s\n", a.keymap);
            vcd_end();
            return 1;
        }
        if (rp->num_cols <= 0 || rp->num_cols > SIM_REPEAT_MAX_COLS ||
            rp->minimum_count < 2) {
            fprintf(stderr,
                    "FAIL: invalid repeat data for %s: num_cols=%d, minimum_count=%u\n",
                    a.keymap, rp->num_cols, rp->minimum_count);
            vcd_end();
            return 1;
        }

        set_dip(rp->dip_value);

        if (sim_wait_ms(cpu, rp->boot_scan_ticks, io->cpu_frequency_hz) < 0) {
            fprintf(stderr, "FAIL: cpu halted during repeat boot wait\n");
            vcd_end();
            return 1;
        }

        unsigned counts[SIM_REPEAT_MAX_COLS];

        for (int i = 0; i < rp->num_cols; i++) {
            int col = rp->cols[i];

            cap_clear();
            matrix_press(rp->row, col);
            if (sim_wait_ms(cpu, rp->hold_ms, io->cpu_frequency_hz) < 0) {
                fprintf(stderr, "FAIL: cpu halted holding (%d,%d)\n", rp->row, col);
                vcd_end();
                return 1;
            }
            matrix_release(rp->row, col);

            /* The release can land inside a 10 us output strobe. Let a strobe
             * in progress complete before counting. */
            if (cap_count() & 1u) {
                sim_run_until(cpu, pred_capture_even, 0, io->cpu_frequency_hz / 1000);
            }

            /* Both strobe edges are captured, so the raw record count is
             * twice the number of bytes the firmware actually emitted. */
            size_t captured = cap_count();
            if (captured == ASDF_CAP_RING_SIZE) {
                fprintf(stderr,
                        "FAIL: %s/%s repeat capture for (%d,%d) filled the "
                        "capture ring; results may be truncated\n",
                        a.target, a.keymap, rp->row, col);
                vcd_end();
                return 1;
            }
            if (captured & 1u) {
                fprintf(stderr,
                        "FAIL: %s/%s repeat capture for (%d,%d) has an unmatched "
                        "strobe edge (%zu records)\n",
                        a.target, a.keymap, rp->row, col, captured);
                vcd_end();
                return 1;
            }
            counts[i] = (unsigned)(captured / 2);

            if (sim_wait_ms(cpu, rp->settle_ms, io->cpu_frequency_hz) < 0) {
                fprintf(stderr, "FAIL: cpu halted while settling (%d,%d)\n",
                        rp->row, col);
                vcd_end();
                return 1;
            }
            cap_clear();
        }

        unsigned lo = counts[0], hi = counts[0];
        for (int i = 1; i < rp->num_cols; i++) {
            if (counts[i] < lo) lo = counts[i];
            if (counts[i] > hi) hi = counts[i];
        }

        /* A uniformly broken autorepeat implementation would otherwise pass
         * the spread check with one initial byte from every column. */
        if (lo < rp->minimum_count) {
            fprintf(stderr,
                    "FAIL: %s/%s repeat: minimum emission count %u is below "
                    "required %u; keys must emit and autorepeat\n",
                    a.target, a.keymap, lo, rp->minimum_count);
            for (int i = 0; i < rp->num_cols; i++)
                fprintf(stderr, "  col %d: %u\n", rp->cols[i], counts[i]);
            vcd_end();
            return 1;
        }

        if (hi - lo > rp->tolerance) {
            fprintf(stderr,
                        "FAIL: %s/%s emission rate depends on column position: "
                        "spread %u (min %u, max %u) exceeds tolerance %u\n",
                    a.target, a.keymap, hi - lo, lo, hi, rp->tolerance);
            for (int i = 0; i < rp->num_cols; i++)
                fprintf(stderr, "  row %d col %d: %u emissions in %u ms\n",
                        rp->row, rp->cols[i], counts[i], rp->hold_ms);
            vcd_end();
            return 1;
        }

        vcd_end();
        printf("OK: %s/%s autorepeat uniform across %d columns "
               "(%u-%u emissions, tolerance %u) at cycle %" PRIu64 "\n",
               a.target, a.keymap, rp->num_cols, lo, hi, rp->tolerance,
               (uint64_t)cpu->cycle);
        return 0;
    }

    fprintf(stderr, "FAIL: unreachable\n");
    return 1;
}
