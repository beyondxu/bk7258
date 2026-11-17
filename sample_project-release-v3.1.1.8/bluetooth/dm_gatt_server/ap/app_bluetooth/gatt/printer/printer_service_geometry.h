#ifndef PRINTER_SERVICE_GEOMETRY_H
#define PRINTER_SERVICE_GEOMETRY_H
/* User measured the old 7480-step stroke as approximately 40mm.
 * Add 2mm per side to the prior 1590-step (~8.5mm) margin:
 * 1590 + round(7480 * 2 / 40) = 1964 steps (~10.5mm).
 * These distances remain open-loop estimates, not measured end positions.
 */
#define PRINTER_SERVICE_BASE_STEPS 7480U
#define PRINTER_SERVICE_MARGIN_STEPS 1964U
#define PRINTER_SERVICE_SPAN_STEPS (PRINTER_SERVICE_BASE_STEPS+2U*PRINTER_SERVICE_MARGIN_STEPS)
/* Only the head-OFF service trial uses these speed parameters. */
#define PRINTER_SERVICE_START_LOW_US 3000U
#define PRINTER_SERVICE_CRUISE_LOW_US 600U
#define PRINTER_SERVICE_RAMP_STEPS 250U
#endif
