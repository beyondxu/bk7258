#ifndef PRINTER_CANVAS_GEOMETRY_H
#define PRINTER_CANVAS_GEOMETRY_H
#include "printer_service_geometry.h"
/* Measured approximately 7480 steps / 40mm. Scale motor and column clocks together to preserve physical pitch;
 * use the configured print stroke independently of maintenance positioning.
 * 1180 transport columns include CMY horizontal offset compensation. At this
 * pitch the actual APP canvas is 1139 columns, approximately 50mm wide.
 * APP white margins are image pixels and must not be cropped. */
/* V30:6.45% faster than V29. Column509us is rounded from985*62/120.
 * Image pitch differs from baseline by0.0164% (~0.008mm/50mm). */
#define PRINTER_CANVAS_STEP_US 62U
#define PRINTER_CANVAS_COLUMN_US 509U
#define PRINTER_CANVAS_X_PITCH_CENTI_UM 4390U
#define PRINTER_CANVAS_COLUMNS 1180U
/* Mirror the first/last timed column around the configured print-stroke midpoint.
 * Round the start to a whole motor step. Direction registration error from
 * this rounding is less than one measured step (approximately 5.35um). */
#define PRINTER_CANVAS_LEAD_STEPS ((PRINTER_SERVICE_SPAN_STEPS*PRINTER_CANVAS_STEP_US-(PRINTER_CANVAS_COLUMNS-1U)*PRINTER_CANVAS_COLUMN_US)/(2U*PRINTER_CANVAS_STEP_US))
/* Shorter outer travel leaves396 lead steps. Limit only image acceleration
 * to346 steps, so the first/last columns retain50 steps of cruise clearance.
 * Head-OFF service motion keeps its original400-step ramps. */
#define PRINTER_CANVAS_RAMP_STEPS ((PRINTER_CANVAS_LEAD_STEPS<450U)?PRINTER_CANVAS_LEAD_STEPS-50U:400U)
#if PRINTER_CANVAS_LEAD_STEPS < 350U
#error Canvas must remain entirely inside the constant-speed stroke
#endif
#endif
