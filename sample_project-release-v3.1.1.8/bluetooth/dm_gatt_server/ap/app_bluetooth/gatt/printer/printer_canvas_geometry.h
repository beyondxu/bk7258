#ifndef PRINTER_CANVAS_GEOMETRY_H
#define PRINTER_CANVAS_GEOMETRY_H
#include "printer_service_geometry.h"
/* Measured approximately 7480 steps / 40mm. Scale motor and column clocks together to preserve physical pitch;
 * use the configured print stroke independently of maintenance positioning.
 * 1180 transport columns include CMY horizontal offset compensation. At this
 * pitch the actual APP canvas is 1139 columns, approximately 50mm wide.
 * APP white margins are image pixels and must not be cropped. */
/* Preserve the canvas pitch: 632us rounds the previous 673us * 77 / 82. */
#define PRINTER_CANVAS_STEP_US 77U
#define PRINTER_CANVAS_STEP_HIGH_US 27U
#define PRINTER_CANVAS_COLUMN_US 632U
#define PRINTER_CANVAS_X_PITCH_CENTI_UM 4388U
#define PRINTER_CANVAS_COLUMNS 1180U
#if PRINTER_CANVAS_STEP_HIGH_US >= PRINTER_CANVAS_STEP_US
#error STEP high time must leave a low interval
#endif
/* Mirror the first/last timed column around the configured print-stroke midpoint.
 * Round the start to a whole motor step. Direction registration error from
 * this rounding is less than one measured step (approximately 5.35um). */
#define PRINTER_CANVAS_LEAD_STEPS ((PRINTER_SERVICE_SPAN_STEPS*PRINTER_CANVAS_STEP_US-(PRINTER_CANVAS_COLUMNS-1U)*PRINTER_CANVAS_COLUMN_US)/(2U*PRINTER_CANVAS_STEP_US))
/* V32: short64-step endpoint ramps (~0.34mm each) put most of the
 * stroke at cruise. The398-step lead keeps all columns outside the ramp. */
#define PRINTER_CANVAS_RAMP_STEPS 64U
#if PRINTER_CANVAS_LEAD_STEPS < PRINTER_CANVAS_RAMP_STEPS+50U
#error Canvas must remain entirely inside the constant-speed stroke
#endif
#endif
