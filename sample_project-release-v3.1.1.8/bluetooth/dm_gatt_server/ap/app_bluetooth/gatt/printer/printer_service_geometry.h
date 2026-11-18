#ifndef PRINTER_SERVICE_GEOMETRY_H
#define PRINTER_SERVICE_GEOMETRY_H
/* Add another requested 2mm TOTAL to V16: +1mm at each end.
 * Approximate calibration 7480 steps / 40mm => 187 extra steps per side.
 * LEFT counted origin must correspond to the new left endpoint (1mm farther
 * left). This is not automatic homing; no Hall/limit feedback is active. */
#define PRINTER_SERVICE_BASE_STEPS 7480U
#define PRINTER_SERVICE_MARGIN_STEPS 1964U
#define PRINTER_SERVICE_SPAN_STEPS (PRINTER_SERVICE_BASE_STEPS+2U*PRINTER_SERVICE_MARGIN_STEPS)
/* Only the head-OFF service trial uses these speed parameters. */
#define PRINTER_SERVICE_START_LOW_US 3000U
#define PRINTER_SERVICE_CRUISE_LOW_US 600U
#define PRINTER_SERVICE_RAMP_STEPS 250U
#endif
