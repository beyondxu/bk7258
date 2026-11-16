#ifndef PRINTER_SERVICE_GEOMETRY_H
#define PRINTER_SERVICE_GEOMETRY_H
/* User measured the old 7480-step stroke as approximately 40mm.
 * 7480 * 8 / 40 = 1496 steps. This remains an open-loop estimate.
 */
#define PRINTER_SERVICE_BASE_STEPS 7480U
#define PRINTER_SERVICE_MARGIN_STEPS 1496U
#define PRINTER_SERVICE_SPAN_STEPS (PRINTER_SERVICE_BASE_STEPS+2U*PRINTER_SERVICE_MARGIN_STEPS)
/* Only the head-OFF service trial uses these speed parameters. */
#define PRINTER_SERVICE_START_LOW_US 3000U
#define PRINTER_SERVICE_CRUISE_LOW_US 600U
#define PRINTER_SERVICE_RAMP_STEPS 250U
#endif
