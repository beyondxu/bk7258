#ifndef PRINTER_DIAGNOSTICS_H
#define PRINTER_DIAGNOSTICS_H

/* Rebuild BK7258 after changing these values. No APK data is used.
 * 0: normal BLE startup, no automatic ink firing
 * 1: 100 blank SPI columns, no carriage/paper movement
 * 2: one selected domain/slot fired once, no movement
 * 3: selected domain/slot draws a horizontal line with the existing sweep
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

#if PRINTER_STARTUP_DIAGNOSTIC < 0 || PRINTER_STARTUP_DIAGNOSTIC > 3
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
