#ifndef PRINTER_DIAGNOSTICS_H
#define PRINTER_DIAGNOSTICS_H

#ifndef PRINTER_SECTION_PAPER
#define PRINTER_SECTION_PAPER 1 /* C/D2: all 25 addresses per section, paper-only comparison */
#endif

#ifndef PRINTER_SECTION_SCOPE
#define PRINTER_SECTION_SCOPE 0 /* four C/D2 nozzles, same address, one section per sweep */
#endif

#ifndef PRINTER_NOZZLE_CHECK
#define PRINTER_NOZZLE_CHECK 1 /* APP-triggered 192-row C/M/Y check, generated pixels */
#endif

#ifndef PRINTER_NOZZLE_TIME_SCALE
#define PRINTER_NOZZLE_TIME_SCALE 1U /* only the active diagnostic forward STEP-low delays */
#endif
#if PRINTER_NOZZLE_TIME_SCALE != 1 && PRINTER_NOZZLE_TIME_SCALE != 4
#error Nozzle timing comparison supports baseline 1 or low-load 4 only
#endif

#ifndef PRINTER_NOZZLE_BATCHES
#define PRINTER_NOZZLE_BATCHES 2U /* interleaved line indices, no paper feed within color */
#endif
#if PRINTER_NOZZLE_BATCHES != 1 && PRINTER_NOZZLE_BATCHES != 2
#error Nozzle check supports 1 or 2 batches
#endif
#if PRINTER_NOZZLE_CHECK && PRINTER_NOZZLE_BATCHES == 2 && PRINTER_NOZZLE_TIME_SCALE != 1
#error Batch comparison requires original carriage speed
#endif

#ifndef PRINTER_FULL_PAGE
#define PRINTER_FULL_PAGE 1
#endif
#if PRINTER_NOZZLE_CHECK && !PRINTER_FULL_PAGE
#error Nozzle check requires the complete-job worker
#endif
#ifndef PRINTER_PAGE_COLOR_MODE
#define PRINTER_PAGE_COLOR_MODE 2 /* 1=CMY together; 2=C pass then M+Y pass */
#endif
#if PRINTER_PAGE_COLOR_MODE != 1 && PRINTER_PAGE_COLOR_MODE != 2
#error Page color mode must be 1 or 2
#endif
#ifndef PRINTER_PAGE_TRANSFORM
#define PRINTER_PAGE_TRANSFORM 0U /* bit0=mirror X, bit1=flip Y; test TOP/END first */
#endif
#ifndef PRINTER_PAGE_FEED_STEPS
#define PRINTER_PAGE_FEED_STEPS 3170U /* measured: 5*2340 pulses moved 30mm; target 40.64mm */
#endif
#ifndef PRINTER_PAGE_FEED_DIRECTION
#define PRINTER_PAGE_FEED_DIRECTION 0 /* original forward-feed pin level LOW */
#endif
#if PRINTER_FULL_PAGE && (PRINTER_PAGE_FEED_STEPS < 1 || PRINTER_PAGE_FEED_STEPS > 5000)
#error Page feed must be 1..5000 pulses per 192 nozzle rows
#endif
/* Receive-only investigation: disables ALL startup firing/motion diagnostics. */
#ifndef PRINTER_DATA_AUDIT
#define PRINTER_DATA_AUDIT 1
#endif
#ifndef PRINTER_CARRIAGE_TEST
#define PRINTER_CARRIAGE_TEST 0
#endif
#ifndef PRINTER_ALIGN_PATTERN
#define PRINTER_ALIGN_PATTERN 2 /* 1=overlaid, 2=separated C/M/Y nozzle rows */
#endif
#ifndef PRINTER_SOLID_LINE
#define PRINTER_SOLID_LINE 1
#endif
/* Cyan-line branch: APP-triggered validated single-color print; no startup motion. */
#ifndef PRINTER_BOUNDED_LINE
#define PRINTER_BOUNDED_LINE 1
#endif
#ifndef PRINTER_BOUNDED_PRINT
#define PRINTER_BOUNDED_PRINT 1
#endif
#if PRINTER_CARRIAGE_TEST && PRINTER_BOUNDED_PRINT
#error Carriage test must not enable bounded printing
#endif
#ifndef PRINTER_BOUNDED_CHANNEL_MASK
#define PRINTER_BOUNDED_CHANNEL_MASK 7 /* 1=C, 2=M, 4=Y; not a D1..D6 mask */
#endif
#if PRINTER_BOUNDED_LINE && !PRINTER_DATA_AUDIT
#error Bounded line requires the audited receive path
#endif
#if PRINTER_BOUNDED_CHANNEL_MASK < 1 || PRINTER_BOUNDED_CHANNEL_MASK > 7
#error Select C/M/Y with mask 1..7
#endif
#if PRINTER_BOUNDED_PRINT && !PRINTER_BOUNDED_LINE
#error Bounded printing requires a validated bounded-line plan
#endif
/* Mapping audit emits fingerprints only, never SPI or motor commands. */
#ifndef PRINTER_MAP_AUDIT
#define PRINTER_MAP_AUDIT 0
#endif
#if PRINTER_MAP_AUDIT && !PRINTER_DATA_AUDIT
#error Mapping audit requires receive-only mode; do not enable uncalibrated firing
#endif
#ifndef PRINTER_MAP_X_PITCH_CENTI_UM
#define PRINTER_MAP_X_PITCH_CENTI_UM 3000
#endif
#ifndef PRINTER_MAP_X_DIRECTION
#define PRINTER_MAP_X_DIRECTION -1
#endif
#ifndef PRINTER_MAP_FIRST_NOZZLE
#define PRINTER_MAP_FIRST_NOZZLE 0
#endif
#ifndef PRINTER_MAP_REVERSE_Y
#define PRINTER_MAP_REVERSE_Y 0
#endif

#ifndef PRINTER_AUDIT_DUMP_MAX_BYTES
#define PRINTER_AUDIT_DUMP_MAX_BYTES 0U
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

#if PRINTER_BOUNDED_PRINT && PRINTER_STARTUP_DIAGNOSTIC != 0
#error Bounded printing requires startup diagnostic 0
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
void printer_run_carriage_diagnostic(void);

#endif
