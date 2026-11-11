#ifndef PRINTER_DIAGNOSTICS_H
#define PRINTER_DIAGNOSTICS_H

#ifndef PRINTER_IDLE_AB
#define PRINTER_IDLE_AB 1 /* frozen-line OFF/ON versus held-power idle comparison */
#endif

#ifndef PRINTER_IDLE_HELD_ONLY
#define PRINTER_IDLE_HELD_ONLY 1 /* one held-power phase, six scans per APP trigger */
#endif
#ifndef PRINTER_IDLE_AB_GAP_MS
#define PRINTER_IDLE_AB_GAP_MS 300000U /* five minutes between active-column ACKs */
#endif

#ifndef PRINTER_DOMAIN_PAIR
#define PRINTER_DOMAIN_PAIR 0 /* fixed-address S2 D1/D2/paired comparison */
#endif

#ifndef PRINTER_CYAN_INVENTORY
#define PRINTER_CYAN_INVENTORY 0 /* previous C-nozzle inventory */
#endif

#ifndef PRINTER_LINE_CONTROL
#define PRINTER_LINE_CONTROL 0 /* previous five-line control */
#endif

#ifndef PRINTER_NOZZLE_CHECK
#define PRINTER_NOZZLE_CHECK 0 /* keep the 600-nozzle short-line test available */
#endif

#ifndef PRINTER_PAGE_ONLY_SWATH
#define PRINTER_PAGE_ONLY_SWATH (-1) /* diagnostic covers all physical rows */
#endif

#ifndef PRINTER_PAGE_REPEAT_SAME_POWER
#define PRINTER_PAGE_REPEAT_SAME_POWER 0 /* one scan per nozzle group */
#endif

#ifndef PRINTER_PAGE_HEAD_SETTLE_MS
#define PRINTER_PAGE_HEAD_SETTLE_MS 2000U /* powered idle, no columns before each diagnostic group */
#endif
#if PRINTER_PAGE_HEAD_SETTLE_MS != 0 && PRINTER_PAGE_HEAD_SETTLE_MS != 2000
#error Page head settle comparison supports 0 or 2000 milliseconds
#endif

#ifndef PRINTER_FULL_PAGE
#define PRINTER_FULL_PAGE 1
#endif
#if PRINTER_IDLE_AB && (!PRINTER_FULL_PAGE || PRINTER_DOMAIN_PAIR || PRINTER_CYAN_INVENTORY || PRINTER_LINE_CONTROL || PRINTER_NOZZLE_CHECK || PRINTER_PAGE_REPEAT_SAME_POWER)
#error Idle A/B requires full-page trigger and exclusive diagnostic mode
#endif
#ifndef PRINTER_PAGE_COLOR_MODE
#define PRINTER_PAGE_COLOR_MODE 1 /* 1=CMY together; 2=C pass then M+Y pass */
#endif
#if PRINTER_PAGE_COLOR_MODE != 1 && PRINTER_PAGE_COLOR_MODE != 2
#error Page color mode must be 1 or 2
#endif
#ifndef PRINTER_PAGE_TRANSFORM
#define PRINTER_PAGE_TRANSFORM 0U /* bit0=mirror X, bit1=flip Y; test TOP/END first */
#endif
#ifndef PRINTER_PAGE_FEED_STEPS
#define PRINTER_PAGE_FEED_STEPS 3170U /* measured: 11700 pulses / 30mm; 3170 pulses ~8.13mm */
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
