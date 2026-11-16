# Actual APP CMY print with service travel

The active build prints the actual decoded APP C/M/Y page, combined pass,
original nozzle rows, no image transform. Disable the black prefix, yellow-only
mask, yellow S1 remap and all fixed-pattern/idle diagnostic branches. A three-color
source must remain three colors in the rendered column data; no synthetic black
line is inserted. This also removes the old upper-yellow-band layout requirement.

Keep the current 1590-step non-firing margin at each end, 7480-step print
segment, 10660-step forward and return stroke, service speed and 3170-step paper
feeds. Head OFF throughout service travel and return. HC32 unchanged.

After updating BK, confirm APP_SERVICE_CMY_V1 at startup. With the established
left service origin, send the original six bars C,M,Y,Y,M,C from the APP once.
Photograph the full page and retain BK/HC32 logs. Do not infer actual ink output
from successful column transmission. No new wipe or repeated firing is required
by this software test.

Validation: full-page renderer/controller regression, service-window motion
regression, and actual SDK compilation of startup, worker, protocol and motor
objects. Firmware linking and physical color/registration remain target checks.
