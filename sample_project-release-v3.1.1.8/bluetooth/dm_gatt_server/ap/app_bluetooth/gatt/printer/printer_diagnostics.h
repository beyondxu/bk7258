#ifndef PRINTER_DIAGNOSTICS_H
#define PRINTER_DIAGNOSTICS_H

#ifndef PRINTER_IDLE_AB
#define PRINTER_IDLE_AB 0 /* frozen-line OFF/ON versus held-power idle comparison */
#endif

/* Actual APP CMY printing with 8.5mm-per-side non-firing service travel. */
#ifndef PRINTER_SERVICE_TRAVEL_TEST
#define PRINTER_SERVICE_TRAVEL_TEST 0
#endif
#ifndef PRINTER_SERVICE_PRINT
#define PRINTER_SERVICE_PRINT 1
#endif
#if PRINTER_SERVICE_TRAVEL_TEST || PRINTER_SERVICE_PRINT
#include "printer_service_geometry.h"
#endif

/* One CMY black line before the unchanged yellow page, no automatic repeats. */
#ifndef PRINTER_BLACK_PREFIX
#define PRINTER_BLACK_PREFIX 0
#endif

/* Move only the known upper yellow band's S1 rows within the same section. */
#ifndef PRINTER_YELLOW_S1_REMAP
#define PRINTER_YELLOW_S1_REMAP 0
#endif

/* Print only actual APP yellow pixels, preserving page/nozzle geometry. */
#ifndef PRINTER_APP_YELLOW_ONLY
#define PRINTER_APP_YELLOW_ONLY 0
#endif

/* Inspect the actual APP BIN; no fixed recipe, head, motion or HC32 link. */
#ifndef PRINTER_APP_PAYLOAD_AUDIT
#define PRINTER_APP_PAYLOAD_AUDIT 0
#endif

/* S1 control: same addresses, data masks, motion and dose as fixed S2. */
#ifndef PRINTER_FIXED_S1_COMPARE
#define PRINTER_FIXED_S1_COMPARE 0
#endif

#ifndef PRINTER_FIXED_S2_DOMAIN_CHECK
#define PRINTER_FIXED_S2_DOMAIN_CHECK 0 /* D1..D6 then D6..D1, one D bit per line */
#endif

#ifndef PRINTER_FIXED_S2_COLOR_CHECK
#define PRINTER_FIXED_S2_COLOR_CHECK 0 /* C,M,Y,Y,M,C, equal ordinary dose per line */
#endif

#ifndef PRINTER_FIXED_S2_BASELINE
#define PRINTER_FIXED_S2_BASELINE 0 /* deterministic S2 recipe; APP pixels do not select nozzles */
#endif

#ifndef PRINTER_STARTUP_RECOVERY
#define PRINTER_STARTUP_RECOVERY 0 /* one normal / per-column x2 / normal comparison */
#endif

#if PRINTER_FIXED_S2_BASELINE && (!PRINTER_STARTUP_RECOVERY || !PRINTER_IDLE_AB)
#error Fixed S2 baseline requires the startup-recovery diagnostic path
#endif

#if PRINTER_FIXED_S2_COLOR_CHECK && !PRINTER_FIXED_S2_BASELINE
#error Fixed S2 color check requires the fixed S2 recipe
#endif

#if PRINTER_FIXED_S2_DOMAIN_CHECK && !PRINTER_FIXED_S2_COLOR_CHECK
#error Individual D check requires the fixed S2 check engine
#endif

#ifndef PRINTER_IDLE_HELD_ONLY
#define PRINTER_IDLE_HELD_ONLY 0 /* previous held-power-only comparison */
#endif
#ifndef PRINTER_IDLE_OFF_ONLY
#define PRINTER_IDLE_OFF_ONLY 0 /* previous five-minute OFF/restart comparison */
#endif
#ifndef PRINTER_IDLE_REFERENCE_ONLY
#define PRINTER_IDLE_REFERENCE_ONLY 0 /* three CMY scans, no long idle or priming */
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

/* Requires matching HC32 LBUFv1 firmware: cache the full row before moving. */
#ifndef PRINTER_BUFFERED_LINE
#define PRINTER_BUFFERED_LINE 1
#endif
/* Alternate actual print direction after each nonempty band. */
#ifndef PRINTER_PAGE_BIDIRECTIONAL
#define PRINTER_PAGE_BIDIRECTIONAL 1
#endif
/* Narrow-band experiment: 24 rows (~1.016mm) per paper advance.
 * PAGE_FEED_STEPS remains the calibration for192 rows, not for one narrow band.
 */
#ifndef PRINTER_PAGE_BAND_ROWS
#define PRINTER_PAGE_BAND_ROWS 24U
#endif
#ifndef PRINTER_PAGE_ONLY_SWATH
#define PRINTER_PAGE_ONLY_SWATH (-1) /* diagnostic covers all physical rows */
#endif

#ifndef PRINTER_PAGE_REPEAT_SAME_POWER
#define PRINTER_PAGE_REPEAT_SAME_POWER 0 /* one scan per nozzle group */
#endif

#ifndef PRINTER_PAGE_HEAD_SETTLE_MS
#define PRINTER_PAGE_HEAD_SETTLE_MS 0U /* powered idle, no columns before each diagnostic group */
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
#if PRINTER_STARTUP_RECOVERY && PRINTER_IDLE_AB && PRINTER_FULL_PAGE && !PRINTER_IDLE_REFERENCE_ONLY
#error Startup recovery requires short reference-only mode
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
/* Independent head-OFF preparation before image row zero. Startup paper
 * travel is an absolute loading offset, NOT a scale or per-band feed. */
#ifndef PRINTER_BOOT_CARRIAGE_PREP
#define PRINTER_BOOT_CARRIAGE_PREP 1 /* once after link self-test, before BLE */
#endif
#ifndef PRINTER_STARTUP_PREP
#define PRINTER_STARTUP_PREP 1
#endif
#ifndef PRINTER_STARTUP_WIPE_STEPS
#define PRINTER_STARTUP_WIPE_STEPS 3740U /* user correction: stage 1 stays 20mm */
#endif
#ifndef PRINTER_STARTUP_WIPE_ROUNDS
#define PRINTER_STARTUP_WIPE_ROUNDS 2U
#endif
#ifndef PRINTER_STARTUP_LOAD_STEPS
#define PRINTER_STARTUP_LOAD_STEPS 702U /* 1.8mm PER SINGLE LEG, 11700 pulses/30mm */
#endif
#ifndef PRINTER_STARTUP_LOAD_ROUNDS
#define PRINTER_STARTUP_LOAD_ROUNDS 3U /* six single legs; 6*1.8mm = 10.8mm */
#endif
#if PRINTER_STARTUP_LOAD_ROUNDS < 1 || PRINTER_STARTUP_LOAD_ROUNDS > 3
#error Startup loading requires one to three bounded round trips
#endif
#if PRINTER_STARTUP_LOAD_STEPS > 5000U
#error Startup loading offset must be bounded to at most 5000 pulses
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
#include "printer_canvas_geometry.h"
#define PRINTER_MAP_X_PITCH_CENTI_UM PRINTER_CANVAS_X_PITCH_CENTI_UM
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
int printer_run_boot_carriage_prepare(void);
void printer_run_carriage_diagnostic(void);

#endif
