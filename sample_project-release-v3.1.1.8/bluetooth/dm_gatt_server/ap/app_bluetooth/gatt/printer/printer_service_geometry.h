#ifndef PRINTER_SERVICE_GEOMETRY_H
#define PRINTER_SERVICE_GEOMETRY_H
/* User measured the old 7480-step stroke as approximately 40mm.
 * 7480 * 10 / 40 = 1870 steps. This remains an open-loop estimate.
 */
#define PRINTER_SERVICE_BASE_STEPS 7480U
#define PRINTER_SERVICE_MARGIN_STEPS 1870U
#define PRINTER_SERVICE_SPAN_STEPS (PRINTER_SERVICE_BASE_STEPS+2U*PRINTER_SERVICE_MARGIN_STEPS)
#endif
