#ifndef PRINTER_SERVICE_GEOMETRY_H
#define PRINTER_SERVICE_GEOMETRY_H
/* Keep the proven print envelope and pixel timing fixed. Boot, preparation
 * and final park distances below are separate calibrated phase parameters. */
#define PRINTER_SERVICE_BASE_STEPS 7480U
#define PRINTER_SERVICE_MARGIN_STEPS 1777U
#define PRINTER_SERVICE_SPAN_STEPS (PRINTER_SERVICE_BASE_STEPS+2U*PRINTER_SERVICE_MARGIN_STEPS)
/* Separate phase budgets. Measured OC3 outward count=11057; the old11408
 * reverse overran by351 pulses. Return11034 leaves23 pulses (~0.12mm) inward.
 * Search caps are NOT physical RIGHT-to-wiper return distances. */
#ifndef PRINTER_OC3_BOOT_SEARCH_STEPS
#define PRINTER_OC3_BOOT_SEARCH_STEPS 11408U
#endif
#ifndef PRINTER_OC3_BOOT_RETURN_STEPS
#define PRINTER_OC3_BOOT_RETURN_STEPS 11034U
#endif
#ifndef PRINTER_OC3_PREP_SEARCH_STEPS
#define PRINTER_OC3_PREP_SEARCH_STEPS 11408U
#endif
#ifndef PRINTER_OC3_PREP_RETURN_STEPS
#define PRINTER_OC3_PREP_RETURN_STEPS 11034U
#endif
#ifndef PRINTER_OC3_FINAL_RETURN_STEPS
#define PRINTER_OC3_FINAL_RETURN_STEPS 11034U
#endif
#define PRINTER_OC3_TRAVEL_LIMIT_STEPS 11408U
#if PRINTER_OC3_BOOT_RETURN_STEPS < 1 || PRINTER_OC3_BOOT_RETURN_STEPS > 11034U || PRINTER_OC3_PREP_RETURN_STEPS < 1 || PRINTER_OC3_PREP_RETURN_STEPS > 11034U || PRINTER_OC3_FINAL_RETURN_STEPS < 1 || PRINTER_OC3_FINAL_RETURN_STEPS > 11034U
#error Return distance exceeds the calibrated safe range
#endif
#if PRINTER_OC3_BOOT_SEARCH_STEPS < 1 || PRINTER_OC3_BOOT_SEARCH_STEPS > PRINTER_OC3_TRAVEL_LIMIT_STEPS || PRINTER_OC3_PREP_SEARCH_STEPS < 1 || PRINTER_OC3_PREP_SEARCH_STEPS > PRINTER_OC3_TRAVEL_LIMIT_STEPS
#error Search distance exceeds the bounded travel budget
#endif
/* Compatibility names denote the return/print envelope, not outward search. */
#define PRINTER_MAINTENANCE_EXTRA_STEPS 0U
#define PRINTER_MAINTENANCE_SPAN_STEPS PRINTER_SERVICE_SPAN_STEPS
/* Only the head-OFF service trial uses these speed parameters. */
#define PRINTER_SERVICE_START_LOW_US 3000U
#define PRINTER_SERVICE_CRUISE_LOW_US 600U
#define PRINTER_SERVICE_RAMP_STEPS 250U
#endif
