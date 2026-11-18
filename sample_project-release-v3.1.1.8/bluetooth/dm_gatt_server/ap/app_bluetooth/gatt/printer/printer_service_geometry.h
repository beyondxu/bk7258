#ifndef PRINTER_SERVICE_GEOMETRY_H
#define PRINTER_SERVICE_GEOMETRY_H
/* User correction: shorten V17 stroke by 2mm TOTAL, 1mm at each end.
 * Approximate 7480 steps / 40mm => remove 187 steps per side.
 * LEFT counted origin moves 1mm inward from V17. No sensor homing yet. */
#define PRINTER_SERVICE_BASE_STEPS 7480U
#define PRINTER_SERVICE_MARGIN_STEPS 1777U
#define PRINTER_SERVICE_SPAN_STEPS (PRINTER_SERVICE_BASE_STEPS+2U*PRINTER_SERVICE_MARGIN_STEPS)
/* Extend maintenance travel ONLY at the wiper side. The print LEFT/RIGHT
 * anchors stay fixed; the wiper endpoint is 374 steps left of print LEFT. */
#define PRINTER_MAINTENANCE_EXTRA_STEPS 374U /* 2mm at 7480 steps / 40mm */
#define PRINTER_MAINTENANCE_SPAN_STEPS (PRINTER_SERVICE_SPAN_STEPS+PRINTER_MAINTENANCE_EXTRA_STEPS)
/* Only the head-OFF service trial uses these speed parameters. */
#define PRINTER_SERVICE_START_LOW_US 3000U
#define PRINTER_SERVICE_CRUISE_LOW_US 600U
#define PRINTER_SERVICE_RAMP_STEPS 250U
#endif
