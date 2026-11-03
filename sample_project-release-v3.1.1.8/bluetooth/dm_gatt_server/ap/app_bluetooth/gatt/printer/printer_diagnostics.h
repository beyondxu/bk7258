#ifndef PRINTER_DIAGNOSTICS_H
#define PRINTER_DIAGNOSTICS_H

/* Receive-only investigation: disables ALL startup firing/motion diagnostics. */
#ifndef PRINTER_DATA_AUDIT
#define PRINTER_DATA_AUDIT 1
#endif
/* Mapping audit emits fingerprints only, never SPI or motor commands. */
#ifndef PRINTER_MAP_AUDIT
#define PRINTER_MAP_AUDIT 1
#endif
#if PRINTER_MAP_AUDIT && !PRINTER_DATA_AUDIT
#error Mapping audit requires receive-only mode; do not enable uncalibrated firing
#endif
#ifndef PRINTER_MAP_X_PITCH_CENTI_UM
#define PRINTER_MAP_X_PITCH_CENTI_UM 4233
#endif
#ifndef PRINTER_MAP_X_DIRECTION
#define PRINTER_MAP_X_DIRECTION 1
#endif
#ifndef PRINTER_MAP_FIRST_NOZZLE
#define PRINTER_MAP_FIRST_NOZZLE 0
#endif
#ifndef PRINTER_MAP_REVERSE_Y
#define PRINTER_MAP_REVERSE_Y 0
#endif

#ifndef PRINTER_AUDIT_DUMP_MAX_BYTES
#define PRINTER_AUDIT_DUMP_MAX_BYTES 65536U
#endif

/* Rebuild BK7258 after changing these values. No APK data is used.
 * 0: normal BLE startup, no automatic ink firing
 * 1: 100 blank SPI columns, no carriage/paper movement
 * 2: one selected domain/slot fired once, no movement
 * 3: selected domain/slot draws a horizontal line with the existing sweep
 * 4: hold boost on for 30 seconds for rail measurement; no SPI firing/motion
 */
#ifndef PRINTER_STARTUP_DIAGNOSTIC
#define PRINTER_STARTUP_DIAGNOSTIC 0
#endif
#ifndef PRINTER_DIAG_SLOT
#define PRINTER_DIAG_SLOT 40 /* address index 10, section index 0; not nozzle #10 */
#endif
#ifndef PRINTER_DIAG_DOMAIN
#define PRINTER_DIAG_DOMAIN 0 /* exactly one bit D1; range 0..5 */
#endif

/* Optional A/B override: 0x3F reproduces the original six-domain stimulus.
 * It selects six physical nozzles, not a single black pixel.
 * If not overridden, preserve the existing one-domain diagnostic.
 */
#ifndef PRINTER_DIAG_MASK
#define PRINTER_DIAG_MASK (1U << PRINTER_DIAG_DOMAIN)
#endif
#if PRINTER_DIAG_MASK == 0 || (PRINTER_DIAG_MASK & ~0x3FU) != 0
#error Printer diagnostic mask must select only D1..D6
#endif

#if PRINTER_STARTUP_DIAGNOSTIC < 0 || PRINTER_STARTUP_DIAGNOSTIC > 4
#error Invalid printer diagnostic mode
#endif
#if PRINTER_DIAG_SLOT < 0 || PRINTER_DIAG_SLOT >= 100
#error Invalid printer diagnostic slot
#endif
#if PRINTER_DIAG_DOMAIN < 0 || PRINTER_DIAG_DOMAIN >= 6
#error Invalid printer diagnostic domain
#endif

void printer_run_startup_diagnostic(void);

#endif
