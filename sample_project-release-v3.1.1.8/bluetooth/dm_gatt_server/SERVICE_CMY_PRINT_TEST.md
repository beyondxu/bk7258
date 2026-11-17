# Actual APP CMY print with service travel

The active build prints the actual decoded APP C/M/Y page, combined pass,
original nozzle rows, no image transform. Disable the black prefix, yellow-only
mask, yellow S1 remap and all fixed-pattern/idle diagnostic branches. A three-color
source must remain three colors in the rendered column data; no synthetic black
line is inserted. This also removes the old upper-yellow-band layout requirement.

Keep the current 1964-step non-firing margin at each end, 7480-step print
segment, 11408-step forward and return stroke, service speed and 3170-step paper
feeds. Head OFF throughout service travel and return. HC32 unchanged.

After updating BK, confirm APP_SERVICE_CMY_V1 at startup. With the established
left service origin, send the original six bars C,M,Y,Y,M,C from the APP once.
Photograph the full page and retain BK/HC32 logs. Do not infer actual ink output
from successful column transmission. No new wipe or repeated firing is required
by this software test.

Validation: full-page renderer/controller regression, service-window motion
regression, and actual SDK compilation of startup, worker, protocol and motor
objects. Firmware linking and physical color/registration remain target checks.

## Extended service margins

Each end now extends another374 steps (~2mm), from1590 to1964 steps
(~10.5mm per side). Total stroke11408 steps (~61mm). Establish the LEFT service
origin2mm farther left than the previous8.5mm mark to keep the print window in
the same place on paper; verify available clearance. No automatic homing or
limit feedback is present. This change does not establish maintenance contact
or sealing, nor does it prove an overnight no-ink condition is clogging.

Keep feed at3170 steps, actual APP CMY, service/print speeds and HC32 timing
unchanged for this travel comparison. The suggested2975-step seam experiment
has not been applied. Maintenance and return travel continue with head OFF.
